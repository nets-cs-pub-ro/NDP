// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
#include "rack_scale_topology.h"
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

RackScaleTopology::RackScaleTopology(int no_of_nodes, mem_b queuesize, Logfile* lg, 
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

RackScaleTopology::RackScaleTopology(int no_of_nodes, mem_b queuesize, Logfile* lg, 
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

void RackScaleTopology::set_params(int no_of_nodes) {
    cout << "Set params " << no_of_nodes << endl;
    cout << "Queue type " << qt << endl;

	NK=1;
    NSRV = no_of_nodes;
	switches_c.resize(NK,NULL);

    pipes_nlp_ns.resize(NK, vector<Pipe*>(NSRV));
    queues_nlp_ns.resize(NK, vector<Queue*>(NSRV));
    pipes_ns_nlp.resize(NSRV, vector<Pipe*>(NK));
	queues_ns_nlp.resize(NSRV, vector<Queue*>(NK));
}

Queue* RackScaleTopology::alloc_src_queue(QueueLogger* queueLogger){
   if (qt == GAZELLE) 
    return  new PriorityQueue(speedFromMbps((uint64_t)HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, queueLogger);
	else
	return  new PriorityQueue(speedFromMbps((uint64_t)HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, queueLogger);
}

Queue* RackScaleTopology::alloc_queue(QueueLogger* queueLogger, mem_b queuesize){
    return alloc_queue(queueLogger, HOST_NIC, queuesize);
}

Queue* RackScaleTopology::alloc_queue(QueueLogger* queueLogger, uint64_t speed, mem_b queuesize){
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
   	else if (qt == GAZELLE) 
   	return  new PriorityQueue(speedFromMbps(speed), memFromPkt(FEEDER_BUFFER), *eventlist, queueLogger);
	assert(0);
}

void RackScaleTopology::init_network(){
  QueueLoggerSampling* queueLogger;

  for (int j=0;j<NK;j++)
    for (int k=0;k<NSRV;k++){
      queues_nlp_ns[j][k] = NULL;
      pipes_nlp_ns[j][k] = NULL;
      queues_ns_nlp[k][j] = NULL;
      pipes_ns_nlp[k][j] = NULL;
    }

  //create switches if we have lossless operation
  if (qt==LOSSLESS || qt == GAZELLE)
      for (int j=0;j<NK;j++){
	      switches_c[j] = new Switch("Switch_Core_"+ntoa(j));
      }
  cout<< "NK="<< NK << " NSRV= " << NSRV <<endl;
  // links from lower layer pod switch to server
  for (int j = 0; j < NK; j++) {
      for (int k = 0; k<NSRV; k++) {
	  // Downlink
	  queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	  //queueLogger = NULL;
	  logfile->addLogger(*queueLogger);
	  
	  queues_nlp_ns[j][k] = alloc_queue(queueLogger, _queuesize);
	  queues_nlp_ns[j][k]->setName("LS" + ntoa(j) + "->DST" +ntoa(k));
	  logfile->writeName(*(queues_nlp_ns[j][k]));

	  pipes_nlp_ns[j][k] = new Pipe(timeFromNs(RTT), *eventlist);
	  pipes_nlp_ns[j][k]->setName("Pipe-LS" + ntoa(j)  + "->DST" + ntoa(k));
	  logfile->writeName(*(pipes_nlp_ns[j][k]));
	  
	  // Uplink
	  queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	  logfile->addLogger(*queueLogger);
	  queues_ns_nlp[k][j] = alloc_src_queue(queueLogger);
	  queues_ns_nlp[k][j]->setName("SRC" + ntoa(k) + "->LS" +ntoa(j));
	  logfile->writeName(*(queues_ns_nlp[k][j]));

	  if (qt==LOSSLESS || qt== GAZELLE){
	      switches_c[j]->addPort(queues_nlp_ns[j][k]);
	      ((LosslessQueue*)queues_nlp_ns[j][k])->setRemoteEndpoint(queues_ns_nlp[k][j]);
	  }else if (qt==LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN){
	      //no virtual queue needed at server
	      new LosslessInputQueue(*eventlist,queues_ns_nlp[k][j]);
	  }
	  pipes_ns_nlp[k][j] = new Pipe(timeFromNs(RTT), *eventlist);
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
       
    //init thresholds for lossless operation
    if (qt==LOSSLESS || qt == GAZELLE)
	for (int j=0;j<NK;j++){
		//yanfang
		//switches_c[j]->configureConnected();
	}
}

#if 0
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
#endif

vector<const Route*>* RackScaleTopology::get_paths(int src, int dest){
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

    print_route(*routeout);
	paths->push_back(routeout);

    //check_non_null(routeout);
    return paths;
  }
}

void RackScaleTopology::count_queue(Queue* queue){
  if (_link_usage.find(queue)==_link_usage.end()){
    _link_usage[queue] = 0;
  }

  _link_usage[queue] = _link_usage[queue] + 1;
}

int RackScaleTopology::find_lp_switch(Queue* queue){
  //first check ns_nlp
  for (int i=0;i<NSRV;i++)
    for (int j = 0;j<NK;j++)
      if (queues_ns_nlp[i][j]==queue)
	return j;

#if 0
  //only count nup to nlp
  count_queue(queue);

  for (int i=0;i<NK;i++)
    for (int j = 0;j<NK;j++)
      if (queues_nup_nlp[i][j]==queue)
	return j;
#endif
  return -1;
}


int RackScaleTopology::find_up_switch(Queue* queue){
	return -1;
}
int RackScaleTopology::find_core_switch(Queue* queue){
  count_queue(queue);
  //first check nup_nc
#if 0
  for (int i=0;i<NK;i++)
    for (int j = 0;j<NC;j++)
      if (queues_nup_nc[i][j]==queue)
	return j;

#endif
  return -1;
}

int RackScaleTopology::find_destination(Queue* queue){
  //first check nlp_ns
  for (int i=0;i<NK;i++)
    for (int j = 0;j<NSRV;j++)
      if (queues_nlp_ns[i][j]==queue)
	return j;

  return -1;
}

void RackScaleTopology::print_path(std::ofstream &paths,int src,const Route* route){
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
