// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
#include "fat_tree_topology.h"
#include <vector>
#include "string.h"
#include <sstream>
#include <strstream>
#include <iostream>
#include "main.h"
#include "queue.h"
#include "switch.h"
#include "compositequeue.h"
#include "prioqueue.h"
#include "queue_lossless.h"
#include "queue_lossless_input.h"
#include "queue_lossless_output.h"
#include "ecnqueue.h"

extern uint32_t RTT;

string ntoa(double n);
string itoa(uint64_t n);

//extern int N;

FatTreeTopology::FatTreeTopology(int no_of_nodes, mem_b queuesize, Logfile* lg, 
				 EventList* ev,FirstFit * fit,queue_type q){
    _queuesize = queuesize;
    logfile = lg;
    eventlist = ev;
    ff = fit;
    qt = q;
    failed_links = 0;
 
    set_params(no_of_nodes);

    init_network();
}

FatTreeTopology::FatTreeTopology(int no_of_nodes, mem_b queuesize, Logfile* lg, 
				 EventList* ev,FirstFit * fit, queue_type q, int fail){
    _queuesize = queuesize;
    logfile = lg;
    qt = q;

    eventlist = ev;
    ff = fit;

    failed_links = fail;
  
    set_params(no_of_nodes);

    init_network();
}

void FatTreeTopology::set_params(int no_of_nodes) {
    cout << "Set params " << no_of_nodes << endl;
    _no_of_nodes = 0;
    K = 0;
    while (_no_of_nodes < no_of_nodes) {
	K++;
	_no_of_nodes = K * K * K /4;
    }
    if (_no_of_nodes > no_of_nodes) {
	cerr << "Topology Error: can't have a FatTree with " << no_of_nodes
	     << " nodes\n";
	exit(1);
    }
    NK = (K*K/2);
    NC = (K*K/4);
    NSRV = (K*K*K/4);
    cout << "_no_of_nodes " << _no_of_nodes << endl;
    cout << "K " << K << endl;
    cout << "Queue type " << qt << endl;

    switches_lp.resize(NK,NULL);
    switches_up.resize(NK,NULL);
    switches_c.resize(NC,NULL);

    pipes_nc_nup.resize(NC, vector<Pipe*>(NK));
    pipes_nup_nlp.resize(NK, vector<Pipe*>(NK));
    pipes_nlp_ns.resize(NK, vector<Pipe*>(NSRV));
    queues_nc_nup.resize(NC, vector<Queue*>(NK));
    queues_nup_nlp.resize(NK, vector<Queue*>(NK));
    queues_nlp_ns.resize(NK, vector<Queue*>(NSRV));

    pipes_nup_nc.resize(NK, vector<Pipe*>(NC));
    pipes_nlp_nup.resize(NK, vector<Pipe*>(NK));
    pipes_ns_nlp.resize(NSRV, vector<Pipe*>(NK));
    queues_nup_nc.resize(NK, vector<Queue*>(NC));
    queues_nlp_nup.resize(NK, vector<Queue*>(NK));
    queues_ns_nlp.resize(NSRV, vector<Queue*>(NK));
}

Queue* FatTreeTopology::alloc_src_queue(QueueLogger* queueLogger){
    return  new PriorityQueue(speedFromMbps((uint64_t)HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, queueLogger);
}

Queue* FatTreeTopology::alloc_queue(QueueLogger* queueLogger, mem_b queuesize){
    return alloc_queue(queueLogger, HOST_NIC, queuesize);
}

Queue* FatTreeTopology::alloc_queue(QueueLogger* queueLogger, uint64_t speed, mem_b queuesize){
    if (qt==RANDOM)
	return new RandomQueue(speedFromMbps(speed), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
    else if (qt==COMPOSITE)
	return new CompositeQueue(speedFromMbps(speed), queuesize, *eventlist, queueLogger);
    else if (qt==CTRL_PRIO)
	return new CtrlPrioQueue(speedFromMbps(speed), queuesize, *eventlist, queueLogger);
    else if (qt==ECN)
	return new ECNQueue(speedFromMbps(speed), memFromPkt(2*SWITCH_BUFFER), *eventlist, queueLogger, memFromPkt(15));
    else if (qt==LOSSLESS)
	return new LosslessQueue(speedFromMbps(speed), memFromPkt(50), *eventlist, queueLogger, NULL);
    else if (qt==LOSSLESS_INPUT)
	return new LosslessOutputQueue(speedFromMbps(speed), memFromPkt(200), *eventlist, queueLogger);    
    else if (qt==LOSSLESS_INPUT_ECN)
	return new LosslessOutputQueue(speedFromMbps(speed), memFromPkt(10000), *eventlist, queueLogger,1,memFromPkt(16));
    assert(0);
}

void FatTreeTopology::init_network(){
  QueueLoggerSampling* queueLogger;

  for (int j=0;j<NC;j++)
    for (int k=0;k<NK;k++){
      queues_nc_nup[j][k] = NULL;
      pipes_nc_nup[j][k] = NULL;
      queues_nup_nc[k][j] = NULL;
      pipes_nup_nc[k][j] = NULL;
    }

  for (int j=0;j<NK;j++)
    for (int k=0;k<NK;k++){
      queues_nup_nlp[j][k] = NULL;
      pipes_nup_nlp[j][k] = NULL;
      queues_nlp_nup[k][j] = NULL;
      pipes_nlp_nup[k][j] = NULL;
    }
  
  for (int j=0;j<NK;j++)
    for (int k=0;k<NSRV;k++){
      queues_nlp_ns[j][k] = NULL;
      pipes_nlp_ns[j][k] = NULL;
      queues_ns_nlp[k][j] = NULL;
      pipes_ns_nlp[k][j] = NULL;
    }

  //create switches if we have lossless operation
  if (qt==LOSSLESS)
      for (int j=0;j<NK;j++){
	  switches_lp[j] = new Switch("Switch_LowerPod_"+ntoa(j));
	  switches_up[j] = new Switch("Switch_UpperPod_"+ntoa(j));
	  if (j<NC)
	      switches_c[j] = new Switch("Switch_Core_"+ntoa(j));
      }
      
  // links from lower layer pod switch to server
  for (int j = 0; j < NK; j++) {
      for (int l = 0; l < K/2; l++) {
	  int k = j * K/2 + l;
	  // Downlink
	  queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	  //queueLogger = NULL;
	  logfile->addLogger(*queueLogger);
	  
	  queues_nlp_ns[j][k] = alloc_queue(queueLogger, _queuesize);
	  queues_nlp_ns[j][k]->setName("LS" + ntoa(j) + "->DST" +ntoa(k));
	  logfile->writeName(*(queues_nlp_ns[j][k]));

	  pipes_nlp_ns[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
	  pipes_nlp_ns[j][k]->setName("Pipe-LS" + ntoa(j)  + "->DST" + ntoa(k));
	  logfile->writeName(*(pipes_nlp_ns[j][k]));
	  
	  // Uplink
	  queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	  logfile->addLogger(*queueLogger);
	  queues_ns_nlp[k][j] = alloc_src_queue(queueLogger);
	  queues_ns_nlp[k][j]->setName("SRC" + ntoa(k) + "->LS" +ntoa(j));
	  logfile->writeName(*(queues_ns_nlp[k][j]));

	  if (qt==LOSSLESS){
	      switches_lp[j]->addPort(queues_nlp_ns[j][k]);
	      ((LosslessQueue*)queues_nlp_ns[j][k])->setRemoteEndpoint(queues_ns_nlp[k][j]);
	  }else if (qt==LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN){
	      //no virtual queue needed at server
	      new LosslessInputQueue(*eventlist,queues_ns_nlp[k][j]);
	  }
	  
	  pipes_ns_nlp[k][j] = new Pipe(timeFromUs(RTT), *eventlist);
	  pipes_ns_nlp[k][j]->setName("Pipe-SRC" + ntoa(k) + "->LS" + ntoa(j));
	  logfile->writeName(*(pipes_ns_nlp[k][j]));
	  
	  if (ff){
	    ff->add_queue(queues_nlp_ns[j][k]);
	    ff->add_queue(queues_ns_nlp[k][j]);
	  }
        }
    }

    /*    for (int i = 0;i<NSRV;i++){
      for (int j = 0;j<NK;j++){
	printf("%p/%p ",queues_ns_nlp[i][j], queues_nlp_ns[j][i]);
      }
      printf("\n");
      }*/
    
    //Lower layer in pod to upper layer in pod!
    for (int j = 0; j < NK; j++) {
      int podid = 2*j/K;
      //Connect the lower layer switch to the upper layer switches in the same pod
      for (int k=MIN_POD_ID(podid); k<=MAX_POD_ID(podid);k++){
	// Downlink
	queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	logfile->addLogger(*queueLogger);
	queues_nup_nlp[k][j] = alloc_queue(queueLogger, _queuesize);
	queues_nup_nlp[k][j]->setName("US" + ntoa(k) + "->LS_" + ntoa(j));
	logfile->writeName(*(queues_nup_nlp[k][j]));
	
	pipes_nup_nlp[k][j] = new Pipe(timeFromUs(RTT), *eventlist);
	pipes_nup_nlp[k][j]->setName("Pipe-US" + ntoa(k) + "->LS" + ntoa(j));
	logfile->writeName(*(pipes_nup_nlp[k][j]));
	
	// Uplink
	queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	logfile->addLogger(*queueLogger);
	queues_nlp_nup[j][k] = alloc_queue(queueLogger, _queuesize);
	queues_nlp_nup[j][k]->setName("LS" + ntoa(j) + "->US" + ntoa(k));
	logfile->writeName(*(queues_nlp_nup[j][k]));

	if (qt==LOSSLESS){
	    switches_lp[j]->addPort(queues_nlp_nup[j][k]);
	    ((LosslessQueue*)queues_nlp_nup[j][k])->setRemoteEndpoint(queues_nup_nlp[k][j]);
	    switches_up[k]->addPort(queues_nup_nlp[k][j]);
	    ((LosslessQueue*)queues_nup_nlp[k][j])->setRemoteEndpoint(queues_nlp_nup[j][k]);
	}else if (qt==LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN){	    
	    new LosslessInputQueue(*eventlist, queues_nlp_nup[j][k]);
	    new LosslessInputQueue(*eventlist, queues_nup_nlp[k][j]);
	}
	
	pipes_nlp_nup[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
	pipes_nlp_nup[j][k]->setName("Pipe-LS" + ntoa(j) + "->US" + ntoa(k));
	logfile->writeName(*(pipes_nlp_nup[j][k]));
	
	if (ff){
	  ff->add_queue(queues_nlp_nup[j][k]);
	  ff->add_queue(queues_nup_nlp[k][j]);
	}
      }
    }

    /*for (int i = 0;i<NK;i++){
      for (int j = 0;j<NK;j++){
	printf("%p/%p ",queues_nlp_nup[i][j], queues_nup_nlp[j][i]);
      }
      printf("\n");
      }*/
    
    // Upper layer in pod to core!
    for (int j = 0; j < NK; j++) {
      int podpos = j%(K/2);
      for (int l = 0; l < K/2; l++) {
	int k = podpos * K/2 + l;
	  // Downlink
	queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	logfile->addLogger(*queueLogger);

	queues_nup_nc[j][k] = alloc_queue(queueLogger, _queuesize);
	queues_nup_nc[j][k]->setName("US" + ntoa(j) + "->CS" + ntoa(k));
	logfile->writeName(*(queues_nup_nc[j][k]));
	
	pipes_nup_nc[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
	pipes_nup_nc[j][k]->setName("Pipe-US" + ntoa(j) + "->CS" + ntoa(k));
	logfile->writeName(*(pipes_nup_nc[j][k]));
	
	// Uplink
	
	queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	logfile->addLogger(*queueLogger);
	
	if ((l+j*K/2)<failed_links){
	    queues_nc_nup[k][j] = alloc_queue(queueLogger,HOST_NIC/10, _queuesize);
	  cout << "Adding link failure for j" << ntoa(j) << " l " << ntoa(l) << endl;
	}
 	else
	    queues_nc_nup[k][j] = alloc_queue(queueLogger, _queuesize);
	
	queues_nc_nup[k][j]->setName("CS" + ntoa(k) + "->US" + ntoa(j));


	if (qt==LOSSLESS){
	    switches_up[j]->addPort(queues_nup_nc[j][k]);
	    ((LosslessQueue*)queues_nup_nc[j][k])->setRemoteEndpoint(queues_nc_nup[k][j]);
	    switches_c[k]->addPort(queues_nc_nup[k][j]);
	    ((LosslessQueue*)queues_nc_nup[k][j])->setRemoteEndpoint(queues_nup_nc[j][k]);
	}
	else if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN){
	    new LosslessInputQueue(*eventlist, queues_nup_nc[j][k]);
	    new LosslessInputQueue(*eventlist, queues_nc_nup[k][j]);
	}

	logfile->writeName(*(queues_nc_nup[k][j]));
	
	pipes_nc_nup[k][j] = new Pipe(timeFromUs(RTT), *eventlist);
	pipes_nc_nup[k][j]->setName("Pipe-CS" + ntoa(k) + "->US" + ntoa(j));
	logfile->writeName(*(pipes_nc_nup[k][j]));
	
	if (ff){
	  ff->add_queue(queues_nup_nc[j][k]);
	  ff->add_queue(queues_nc_nup[k][j]);
	}
      }
    }

    /*    for (int i = 0;i<NK;i++){
      for (int j = 0;j<NC;j++){
	printf("%p/%p ",queues_nup_nc[i][j], queues_nc_nup[j][i]);
      }
      printf("\n");
      }*/
    
    //init thresholds for lossless operation
    if (qt==LOSSLESS)
	for (int j=0;j<NK;j++){
	    switches_lp[j]->configureLossless();
	    switches_up[j]->configureLossless();
	    if (j<NC)
		switches_c[j]->configureLossless();
	}
}

void check_non_null(Route* rt){
  int fail = 0;
  for (unsigned int i=1;i<rt->size()-1;i+=2)
    if (rt->at(i)==NULL){
      fail = 1;
      break;
    }
  
  if (fail){
    //    cout <<"Null queue in route"<<endl;
    for (unsigned int i=1;i<rt->size()-1;i+=2)
      printf("%p ",rt->at(i));

    cout<<endl;
    assert(0);
  }
}

vector<const Route*>* FatTreeTopology::get_paths(int src, int dest){
  vector<const Route*>* paths = new vector<const Route*>();

  route_t *routeout, *routeback;
  //QueueLoggerSimple *simplequeuelogger = new QueueLoggerSimple();
  //QueueLoggerSimple *simplequeuelogger = 0;
  //logfile->addLogger(*simplequeuelogger);
  //Queue* pqueue = new Queue(speedFromMbps((uint64_t)HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, simplequeuelogger);
  //pqueue->setName("PQueue_" + ntoa(src) + "_" + ntoa(dest));
  //logfile->writeName(*pqueue);
  if (HOST_POD_SWITCH(src)==HOST_POD_SWITCH(dest)){
  
    // forward path
    routeout = new Route();
    //routeout->push_back(pqueue);
    routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);
    routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

    if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]->getRemoteEndpoint());

    routeout->push_back(queues_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
    routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

    // reverse path for RTS packets
    routeback = new Route();
    routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)]);
    routeback->push_back(pipes_ns_nlp[dest][HOST_POD_SWITCH(dest)]);

    if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)]->getRemoteEndpoint());

    routeback->push_back(queues_nlp_ns[HOST_POD_SWITCH(src)][src]);
    routeback->push_back(pipes_nlp_ns[HOST_POD_SWITCH(src)][src]);

    routeout->set_reverse(routeback);
    routeback->set_reverse(routeout);

    //print_route(*routeout);
    paths->push_back(routeout);

    check_non_null(routeout);
    return paths;
  }
  else if (HOST_POD(src)==HOST_POD(dest)){
    //don't go up the hierarchy, stay in the pod only.

    int pod = HOST_POD(src);
    //there are K/2 paths between the source and the destination
    for (int upper = MIN_POD_ID(pod);upper <= MAX_POD_ID(pod); upper++){
      //upper is nup
      
      routeout = new Route();
      //routeout->push_back(pqueue);
      
      routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);
      routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

      if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	  routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]->getRemoteEndpoint());

      routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper]);
      routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][upper]);

      if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	  routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper]->getRemoteEndpoint());

      routeout->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(dest)]);
      routeout->push_back(pipes_nup_nlp[upper][HOST_POD_SWITCH(dest)]);

      if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	  routeout->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(dest)]->getRemoteEndpoint());

      routeout->push_back(queues_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
      routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

      // reverse path for RTS packets
      routeback = new Route();
      
      routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)]);
      routeback->push_back(pipes_ns_nlp[dest][HOST_POD_SWITCH(dest)]);

      if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	  routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)]->getRemoteEndpoint());

      routeback->push_back(queues_nlp_nup[HOST_POD_SWITCH(dest)][upper]);
      routeback->push_back(pipes_nlp_nup[HOST_POD_SWITCH(dest)][upper]);

      if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	  routeback->push_back(queues_nlp_nup[HOST_POD_SWITCH(dest)][upper]->getRemoteEndpoint());

      routeback->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(src)]);
      routeback->push_back(pipes_nup_nlp[upper][HOST_POD_SWITCH(src)]);

      if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	  routeback->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(src)]->getRemoteEndpoint());
      
      routeback->push_back(queues_nlp_ns[HOST_POD_SWITCH(src)][src]);
      routeback->push_back(pipes_nlp_ns[HOST_POD_SWITCH(src)][src]);

      routeout->set_reverse(routeback);
      routeback->set_reverse(routeout);
      
      //print_route(*routeout);
      paths->push_back(routeout);
      check_non_null(routeout);
    }
    return paths;
  }
  else {
    int pod = HOST_POD(src);

    for (int upper = MIN_POD_ID(pod);upper <= MAX_POD_ID(pod); upper++)
      for (int core = (upper%(K/2)) * K / 2; core < ((upper % (K/2)) + 1)*K/2; core++){
	//upper is nup
	
	routeout = new Route();
	//routeout->push_back(pqueue);
	
	routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);
	routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

	if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	    routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]->getRemoteEndpoint());
	
	routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper]);
	routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][upper]);

	if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	    routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper]->getRemoteEndpoint());
	
	routeout->push_back(queues_nup_nc[upper][core]);
	routeout->push_back(pipes_nup_nc[upper][core]);

	if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	    routeout->push_back(queues_nup_nc[upper][core]->getRemoteEndpoint());
	
	//now take the only link down to the destination server!
	
	int upper2 = HOST_POD(dest)*K/2 + 2 * core / K;
	//printf("K %d HOST_POD(%d) %d core %d upper2 %d\n",K,dest,HOST_POD(dest),core, upper2);
	
	routeout->push_back(queues_nc_nup[core][upper2]);
	routeout->push_back(pipes_nc_nup[core][upper2]);

	if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	    routeout->push_back(queues_nc_nup[core][upper2]->getRemoteEndpoint());	

	routeout->push_back(queues_nup_nlp[upper2][HOST_POD_SWITCH(dest)]);
	routeout->push_back(pipes_nup_nlp[upper2][HOST_POD_SWITCH(dest)]);

	if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	    routeout->push_back(queues_nup_nlp[upper2][HOST_POD_SWITCH(dest)]->getRemoteEndpoint());
	
	routeout->push_back(queues_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
	routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

	// reverse path for RTS packets
	routeback = new Route();
	
	routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)]);
	routeback->push_back(pipes_ns_nlp[dest][HOST_POD_SWITCH(dest)]);

	if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	    routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)]->getRemoteEndpoint());
	
	routeback->push_back(queues_nlp_nup[HOST_POD_SWITCH(dest)][upper2]);
	routeback->push_back(pipes_nlp_nup[HOST_POD_SWITCH(dest)][upper2]);

	if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	    routeback->push_back(queues_nlp_nup[HOST_POD_SWITCH(dest)][upper2]->getRemoteEndpoint());
	
	routeback->push_back(queues_nup_nc[upper2][core]);
	routeback->push_back(pipes_nup_nc[upper2][core]);

	if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	    routeback->push_back(queues_nup_nc[upper2][core]->getRemoteEndpoint());
	
	//now take the only link back down to the src server!
	
	routeback->push_back(queues_nc_nup[core][upper]);
	routeback->push_back(pipes_nc_nup[core][upper]);

	if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	    routeback->push_back(queues_nc_nup[core][upper]->getRemoteEndpoint());
	
	routeback->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(src)]);
	routeback->push_back(pipes_nup_nlp[upper][HOST_POD_SWITCH(src)]);

	if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
	    routeback->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(src)]->getRemoteEndpoint());
	
	routeback->push_back(queues_nlp_ns[HOST_POD_SWITCH(src)][src]);
	routeback->push_back(pipes_nlp_ns[HOST_POD_SWITCH(src)][src]);


	routeout->set_reverse(routeback);
	routeback->set_reverse(routeout);
	
	//print_route(*routeout);
	paths->push_back(routeout);
	check_non_null(routeout);
      }
    return paths;
  }
}

void FatTreeTopology::count_queue(Queue* queue){
  if (_link_usage.find(queue)==_link_usage.end()){
    _link_usage[queue] = 0;
  }

  _link_usage[queue] = _link_usage[queue] + 1;
}

int FatTreeTopology::find_lp_switch(Queue* queue){
  //first check ns_nlp
  for (int i=0;i<NSRV;i++)
    for (int j = 0;j<NK;j++)
      if (queues_ns_nlp[i][j]==queue)
	return j;

  //only count nup to nlp
  count_queue(queue);

  for (int i=0;i<NK;i++)
    for (int j = 0;j<NK;j++)
      if (queues_nup_nlp[i][j]==queue)
	return j;

  return -1;
}

int FatTreeTopology::find_up_switch(Queue* queue){
  count_queue(queue);
  //first check nc_nup
  for (int i=0;i<NC;i++)
    for (int j = 0;j<NK;j++)
      if (queues_nc_nup[i][j]==queue)
	return j;

  //check nlp_nup
  for (int i=0;i<NK;i++)
    for (int j = 0;j<NK;j++)
      if (queues_nlp_nup[i][j]==queue)
	return j;

  return -1;
}

int FatTreeTopology::find_core_switch(Queue* queue){
  count_queue(queue);
  //first check nup_nc
  for (int i=0;i<NK;i++)
    for (int j = 0;j<NC;j++)
      if (queues_nup_nc[i][j]==queue)
	return j;

  return -1;
}

int FatTreeTopology::find_destination(Queue* queue){
  //first check nlp_ns
  for (int i=0;i<NK;i++)
    for (int j = 0;j<NSRV;j++)
      if (queues_nlp_ns[i][j]==queue)
	return j;

  return -1;
}

void FatTreeTopology::print_path(std::ofstream &paths,int src,const Route* route){
  paths << "SRC_" << src << " ";
  
  if (route->size()/2==2){
    paths << "LS_" << find_lp_switch((Queue*)route->at(1)) << " ";
    paths << "DST_" << find_destination((Queue*)route->at(3)) << " ";
  } else if (route->size()/2==4){
    paths << "LS_" << find_lp_switch((Queue*)route->at(1)) << " ";
    paths << "US_" << find_up_switch((Queue*)route->at(3)) << " ";
    paths << "LS_" << find_lp_switch((Queue*)route->at(5)) << " ";
    paths << "DST_" << find_destination((Queue*)route->at(7)) << " ";
  } else if (route->size()/2==6){
    paths << "LS_" << find_lp_switch((Queue*)route->at(1)) << " ";
    paths << "US_" << find_up_switch((Queue*)route->at(3)) << " ";
    paths << "CS_" << find_core_switch((Queue*)route->at(5)) << " ";
    paths << "US_" << find_up_switch((Queue*)route->at(7)) << " ";
    paths << "LS_" << find_lp_switch((Queue*)route->at(9)) << " ";
    paths << "DST_" << find_destination((Queue*)route->at(11)) << " ";
  } else {
    paths << "Wrong hop count " << ntoa(route->size()/2);
  }
  
  paths << endl;
}
