// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#include "bcube_topology.h"
#include <vector>
#include "string.h"
#include <sstream>
#include <strstream>
#include <iostream>
#include "randomqueue.h"
#include "compositequeue.h"
#include "compositeprioqueue.h"
#include "main.h"

extern uint32_t RTT;

string ntoa(double n);
string itoa(uint64_t n);

/*
void push_front(Route* rt, Queue* q){
  rt->insert(rt->begin(),q);
}
*/

BCubeTopology::BCubeTopology(int no_of_nodes, int ports_per_switch, int no_of_levels, 
			     Logfile* lg, EventList* ev,FirstFit * fit, queue_type qt){
    set_params(no_of_nodes, ports_per_switch, no_of_levels);
    logfile = lg;
    eventlist = ev;
    ff = fit;
    this->qt = qt;
    init_network();
}

void 
BCubeTopology::set_params(int no_of_nodes, int ports_per_switch, int no_of_levels) {
    if (no_of_nodes % ports_per_switch != 0) {
	cerr << "Incorrect Bcube parameters\n";
	exit(1);
    }
    _K = no_of_levels;
    _NUM_PORTS = ports_per_switch;
    _NUM_SRV = no_of_nodes;
    _NUM_SW =(_K + 1) * _NUM_SRV / _NUM_PORTS;

    //Pipe * pipes_srv_switch[NUM_SRV][NUM_SW][K+1];
    //Pipe * pipes_switch_srv[NUM_SW][NUM_SRV][K+1];
    //Queue * queues_srv_switch[NUM_SRV][NUM_SW][K+1];
    //Queue * queues_switch_srv[NUM_SW][NUM_SRV][K+1];
    //Queue * prio_queues_srv[NUM_SRV][K+1];
    //unsigned int addresses[NUM_SRV][K+1];
    pipes_srv_switch.set_size(_NUM_SRV, _NUM_SW, _K+1);
    pipes_switch_srv.set_size(_NUM_SW, _NUM_SRV, _K+1);
    queues_srv_switch.set_size(_NUM_SRV, _NUM_SW, _K+1);
    queues_switch_srv.set_size(_NUM_SW, _NUM_SRV, _K+1);
    prio_queues_srv.set_size(_NUM_SRV, _K+1);
    addresses.set_size(_NUM_SRV,_K+1);
}

#define SWITCH_ID(srv,level) switch_from_srv(srv,level)
//(level==0?srv/_NUM_PORTS:srv%(int)pow(_NUM_PORTS,level+1))
#define ADDRESS(srv,level) (level==0?srv%_NUM_PORTS:srv/(int)pow(_NUM_PORTS,level))

Queue* BCubeTopology::alloc_src_queue(QueueLogger* queueLogger){
    return  new PriorityQueue(speedFromMbps((uint64_t)HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, queueLogger);
}

Queue* BCubeTopology::alloc_queue(QueueLogger* queueLogger){
    return alloc_queue(queueLogger, HOST_NIC);
}

Queue* BCubeTopology::alloc_queue(QueueLogger* queueLogger, uint64_t speed){
    if (qt==RANDOM) {
	return new RandomQueue(speedFromMbps(speed), 
			       memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), 
			       *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
    } else if (qt==COMPOSITE) {
	return new CompositeQueue(speedFromMbps(speed), memFromPkt(8), 
				  *eventlist, queueLogger);
    } else if (qt==COMPOSITE_PRIO) {
	return new CompositePrioQueue(speedFromMbps(speed), memFromPkt(8), 
				      *eventlist, queueLogger);
    } 
    
    assert(0);
}

int BCubeTopology::switch_from_srv(int srv, int level){
    //switch address base is given by level
    int switch_addr = 0;//level * _NUM_SRV/_NUM_PORTS;

    int crt_n = (int)pow(_NUM_PORTS,_K-1);
    for (int i=_K;i>=0;i--){
	if (i==level)
	    continue;

	switch_addr += crt_n * addresses(srv,i);
	crt_n /= _NUM_PORTS;
    }

    return switch_addr;
}

int BCubeTopology::srv_from_address(unsigned int* address){
    int addr = 0,crt_n = 0;
  
    crt_n = (int)pow(_NUM_PORTS,_K);
    //printf("Computing addr from:");
    for (int i=_K;i>=0;i--){
	//    printf("%d ",address[i]);
	addr += crt_n * address[i];
	crt_n /= _NUM_PORTS;
    }
    //printf("\n");

    return addr;
}

void BCubeTopology::address_from_srv(int srv){
    int addr = srv,crt_n = 0;
  
    crt_n = (int)pow(_NUM_PORTS,_K);
    for (int i=_K;i>=0;i--){
	addresses(srv,i) = addr / crt_n;
	addr = addr % crt_n;
	crt_n /= _NUM_PORTS;
    }
}

vector<int>* BCubeTopology::get_neighbours(int src){
    unsigned int addr[_K+1],i;

    vector<int>* ns = new vector<int>();

    for (int level=0;level<=_K;level++){
	for (i=0;i<=_K;i++)
	    addr[i] = addresses(src,i);

	for (int i=0;i<_NUM_PORTS;i++){
	    addr[level] = i;
	    if (addr[level]==addresses(src,level))
		continue;
      
	    ns->push_back(srv_from_address(addr));
	}
    }
    return ns;
}

int BCubeTopology::get_neighbour(int src, int level){
    unsigned int addr[_K+1],i;

    for (i=0;i<=_K;i++)
	addr[i] = addresses(src,i);

    //find neighbour at required level
    //_NUM_PORTS can't be 1 otherwise we get infinite loop;
  
    while ((addr[level] = rand()%_NUM_PORTS)==addresses(src,level));

    return srv_from_address(addr);
}

void BCubeTopology::init_network(){
    QueueLoggerSampling* queueLogger;

    assert(_NUM_PORTS>0);
    assert(_K>=0);
    assert(_NUM_SRV==(int)pow(_NUM_PORTS,_K+1));

    for (int i=0;i<_NUM_SRV;i++){
	address_from_srv(i);  //XXX
	for (int k=0;k<=_K;k++){
	    for (int j=0;j<_NUM_SW;j++){
		pipes_srv_switch(i,j,k) = NULL;
		queues_srv_switch(i,j,k) = NULL;
		pipes_switch_srv(j,i,k) = NULL;
		queues_switch_srv(j,i,k) = NULL;
	    }

	    queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	    logfile->addLogger(*queueLogger);
	    prio_queues_srv(i,k) = alloc_src_queue(queueLogger);
	    prio_queues_srv(i,k)->setName("PRIO_SRV_" + ntoa(i) + "(" + ntoa(k) + ")");
	}
    }

    //  addresses[i][k] = ADDRESS(i,k);

    for (int k=0;k<=_K;k++){
	//create links for level _K
	for (int i=0;i<_NUM_SRV;i++){
	    int j;

	    j = SWITCH_ID(i,k);

	    //printf("SWITCH ID for server %d level %d is %d\n",i,k,j);

	    //index k of the address
      
	    queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	    //queueLogger = NULL;
	    logfile->addLogger(*queueLogger);
      
	    queues_srv_switch(i,j,k) = alloc_queue(queueLogger);
	    queues_srv_switch(i,j,k)->setName("SRV_" + ntoa(i) + "(level_" + ntoa(k)+"))_SW_" +ntoa(j));
	    logfile->writeName(*(queues_srv_switch(i,j,k)));
      
	    pipes_srv_switch(i,j,k) = new Pipe(timeFromUs(RTT), *eventlist);
	    pipes_srv_switch(i,j,k)->setName("Pipe-SRV_" + ntoa(i) + "(level_" + ntoa(k)+")-SW_" +ntoa(j));
	    logfile->writeName(*(pipes_srv_switch(i,j,k)));

	    queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	    //queueLogger = NULL;
	    logfile->addLogger(*queueLogger);
      
	    queues_switch_srv(j,i,k) = alloc_queue(queueLogger);
	    queues_switch_srv(j,i,k)->setName("SW_" + ntoa(j) + "(level_" + ntoa(k)+")-SRV_" +ntoa(i));
	    logfile->writeName(*(queues_switch_srv(j,i,k)));
      
	    pipes_switch_srv(j,i,k) = new Pipe(timeFromUs(RTT), *eventlist);
	    pipes_switch_srv(j,i,k)->setName("Pipe-SW_" + ntoa(j) + "(level_" + ntoa(k)+")-SRV_" +ntoa(i));
	    logfile->writeName(*(pipes_switch_srv(j,i,k)));
	}
    }
}

/*void check_non_null(Route* rt){
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
  }*/


Route* BCubeTopology::bcube_routing(int src,int dest, int* permutation, int* nic = NULL){
    Route* routeout = new Route();

    unsigned int crt_addr[_K+1],crt, level,nsrv;
    int i;
    crt = src;

    int first_hop = 1;

    for (i=0;i<=_K;i++)
	crt_addr[i] = addresses(src,i);

    int aaa = srv_from_address(crt_addr);
    //printf("CRT is %d, SRV FROM ADDRESS %d\n",crt,aaa);
    assert(crt==aaa);

    for (i=_K;i>=0;i--){
	level = permutation[i];
	nsrv = (int)pow(_NUM_PORTS,level+1);

	if (addresses(src,level)!=addresses(dest,level)){
	    //add hop from crt_addr by progressing on id level
	    //go upto switch in level and correct digit
	    //switch id

	    if (first_hop){
		first_hop = 0;

		if (nic!=NULL)
		    *nic = level;
	    }

	    routeout->push_back(queues_srv_switch(crt,SWITCH_ID(crt,level),level));
	    routeout->push_back(pipes_srv_switch(crt,SWITCH_ID(crt,level),level));

	    //now correct digit
	    crt_addr[level] = addresses(dest,level);
	    crt = srv_from_address(crt_addr);

	    assert(srv_from_address(crt_addr)==crt);

	    routeout->push_back(queues_switch_srv(SWITCH_ID(crt,level), crt, level));
	    routeout->push_back(pipes_switch_srv(SWITCH_ID(crt,level), crt, level));
	}
    }

    return routeout;
}

//i is the level
Route* BCubeTopology::dc_routing(int src,int dest, int i){
    int permutation[_K+1];
    int m = _K,j;

    for (j = i;j>=i-_K;j--){
	permutation[m] = (j+_K+1) % (_K+1); m--;
    }

    int nic = -1;

    Route* routeout = bcube_routing(src,dest,permutation,&nic);
    assert(nic>=0&&nic<=_K);

    routeout->push_front(prio_queues_srv(src,nic));

    return routeout;
}

Route* BCubeTopology::alt_dc_routing(int src,int dest, int i,int c){
    int permutation[_K+1];
    int m = _K,j;

    for (int ti=0;ti<=_K;ti++)
	permutation[ti] = ti;

    int nic = -1;

    Route* path = bcube_routing(src,c,permutation,&nic);

    assert(nic>=0&&nic<=_K);
    path->push_front(prio_queues_srv(src,nic));

    //beware negative!!!
    for (j = i-1;j>=i-1-_K;j--){
	permutation[m] = (j+_K+1) % (_K+1); m--;
    }

    Route* path2 = bcube_routing(c,dest,permutation);

    //stitch the two paths
    for (unsigned int i=0;i<path2->size();i++)
	path->push_back(path2->at(i));

    delete path2;

    return path;
}

vector<const Route*>* BCubeTopology::get_paths(int src, int dest){
    vector<const Route*>* paths = new vector<const Route*>(); 
 
    for (int i=_K;i>=0;i--){
	if (addresses(src,i)!=addresses(dest,i))
	    paths->push_back(dc_routing(src,dest,i));
	else 
	    paths->push_back(alt_dc_routing(src,dest,i,get_neighbour(src,i)));
    }
    return paths;
}

void BCubeTopology::print_paths(std::ofstream & p,int src,vector<const Route*>* paths){
    for (unsigned int i=0;i<paths->size();i++)
	print_path(p, src, paths->at(i));
}

void BCubeTopology::print_path(std::ofstream &paths,int src,const Route* route){
    paths << "SRC_" << src << " ";

    for (unsigned int i=1;i<route->size()-1;i+=2){
	if (route->at(i)!=NULL)
	    paths << ((RandomQueue*)route->at(i))->str() << " ";
	else 
	    paths << "NULL ";
    }
    paths << endl;
}
