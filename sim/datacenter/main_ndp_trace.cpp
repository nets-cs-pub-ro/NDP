// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
#include <sstream>
#include <iostream>
#include <string.h>
#include <math.h>
#include <list>
#include "config.h"
#include "network.h"
#include "subflow_control.h"
#include "pipe.h"
#include "eventlist.h"
#include "logfile.h"
#include "loggers.h"
#include "clock.h"
#include "ndp.h"
#include "ndp_transfer.h"
#include "compositequeue.h"
#include "firstfit.h"
#include "topology.h"
//#include "fat_tree_topology.h"
#include "leafspine_topology.h"

// #include "rack_scale_topology.h"
#include "ndp_pairs.h"

// Simulation params

#define PRINT_PATHS 0

#define PERIODIC 0
#include "main.h"

//int RTT = 10; // this is per link delay; identical RTT microseconds = 0.02 ms
uint32_t RTT = 1; // this is per link delay in us; also check the topology file, where it interprets this value. identical RTT microseconds = 0.02 ms
//double RTT = .2; // this is per link delay in us; identical RTT microseconds = 0.012 ms
#define DEFAULT_NODES 128
#define DEFAULT_QUEUE_SIZE 8

FirstFit* ff = NULL;
unsigned int subflow_count = 1;

string ntoa(double n);
string itoa(uint64_t n);
vector<pair<uint64_t, double> >* mesgSizeDistFromFile(
    string filename, double& lambda);

//#define SWITCH_BUFFER (SERVICE * RTT / 1000)
#define USE_FIRST_FIT 0
#define FIRST_FIT_INTERVAL 100

EventList eventlist;

Logfile* lg;

void exit_error(char* progr) {
    cout << "Usage " << progr << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED|COUPLED_EPSILON] [epsilon][COUPLED_SCALABLE_TCP" << endl;
    exit(1);
}

uint16_t loadFactor = 65; // load factor in percent

int main(int argc, char **argv) {
    // Packet::set_packet_size(1442);
    eventlist.setEndtime(timeFromSec(40.0));
    Clock c(timeFromSec(5 / 100.), eventlist);
    uint32_t no_of_conns = DEFAULT_NODES, cwnd = 32, no_of_nodes = DEFAULT_NODES;
    mem_b queuesize; // = memFromPkt(DEFAULT_QUEUE_SIZE);
    stringstream filename(ios_base::out);
    RouteStrategy route_strategy = NOT_SET;
    stringstream distfile(ios_base::out);
    stringstream flowfile(ios_base::out);
    int new_queue_size = 0;
    int mtu = 1442;

	int is_incast = 0;
    int i = 1,seed = 1;
    filename << "logout.dat";
    distfile << "";

    while (i<argc) {
	if (!strcmp(argv[i],"-o")){
	    filename.str(std::string());
	    filename << argv[i+1];
	    i++;
	} else if (!strcmp(argv[i], "-dist")) {
	    distfile.str(std::string());
	    distfile << argv[i+1];
	    i++;
    } else if (!strcmp(argv[i], "-flow")) {
	    flowfile << argv[i+1];
	    i++;
    } else if (!strcmp(argv[i], "-mtu")) {
	    mtu = atoi(argv[i+1]);
	    i++;
    } else if (!strcmp(argv[i],"-seed")){
	    seed = atoi(argv[i+1]);
	    i++;
    } else if (!strcmp(argv[i],"-lf")){
	    loadFactor = atoi(argv[i+1]);
	    i++;
	} else if (!strcmp(argv[i],"-nodes")){
	    no_of_nodes = atoi(argv[i+1]);
	    i++;
	} else if (!strcmp(argv[i],"-cwnd")){
	    cwnd = atoi(argv[i+1]);
	    i++;
	} else if (!strcmp(argv[i],"-q")){
        new_queue_size = atoi(argv[i+1]);
	    i++;
	} else if (!strcmp(argv[i],"-incast")){
	    is_incast = atoi(argv[i+1]);
	    i++;
	} else if (!strcmp(argv[i],"-strat")){
	    if (!strcmp(argv[i+1], "perm")) {
		route_strategy = SCATTER_PERMUTE;
	    } else if (!strcmp(argv[i+1], "rand")) {
		route_strategy = SCATTER_RANDOM;
	    } else if (!strcmp(argv[i+1], "pull")) {
		route_strategy = PULL_BASED;
	    } else if (!strcmp(argv[i+1], "single")) {
		route_strategy = SINGLE_PATH;
	    }
	    i++;
	} else {
	    exit_error(argv[0]);
	}
	i++;
    }
    srand(seed);
    Packet::set_packet_size(mtu);

    if(new_queue_size != 0){
        queuesize = memFromPkt(new_queue_size);
    }else{
        queuesize = memFromPkt(DEFAULT_QUEUE_SIZE);
    }
    cout << "mtu " << Packet::data_packet_size() << endl;
    if (route_strategy == NOT_SET) {
	fprintf(stderr, "Route Strategy not set.  Use the -strat param.  \nValid values are perm, rand, pull, rg and single\n");
	exit(1);
    }

    if (!strcmp(distfile.str().c_str(), "") && !strcmp(flowfile.str().c_str(), "") ) {
        fprintf(stderr, "Message size distribution file is not set and flow file is not set"
            " Use -dist or -flow to specify the filename ");
	    exit(1);
    }

    cout << "Using subflow count " << subflow_count <<endl;

    cout << "conns " << no_of_conns << endl;
    cout << "requested nodes " << no_of_nodes << endl;
    cout << "cwnd " << cwnd << endl;
    cout << "loadFactor " << loadFactor << "%" << endl;
    cout << "link delays " << RTT << "us" << endl; 

    // prepare the loggers

    cout << "Logging to " << filename.str() << endl;
    //Logfile
    Logfile logfile(filename.str(), eventlist);

#if PRINT_PATHS
    filename << ".paths";
    cout << "Logging path choices to " << filename.str() << endl;
    std::ofstream paths(filename.str().c_str());
    if (!paths){
	cout << "Can't open for writing paths file!"<<endl;
	exit(1);
    }
#endif

    lg = &logfile;
    logfile.setStartTime(timeFromSec(0));
    NdpSinkLoggerSampling sinkLogger = NdpSinkLoggerSampling(timeFromMs(10), eventlist);
    logfile.addLogger(sinkLogger);
    NdpTrafficLogger traffic_logger = NdpTrafficLogger();
    logfile.addLogger(traffic_logger);

    NdpRtxTimerScanner ndpRtxScanner(timeFromMs(10), eventlist);

#if USE_FIRST_FIT
    if (subflow_count==1){
	ff = new FirstFit(timeFromMs(FIRST_FIT_INTERVAL),eventlist);
    }
#endif

#ifdef FAT_TREE
    FatTreeTopology* top = new FatTreeTopology(no_of_nodes, queuesize,
					       &logfile, &eventlist,ff,COMPOSITE,0);
    cout << "topology created" << endl;
#endif


#ifdef RACK_SCALE
	RackScaleTopology* top = new RackScaleTopology(no_of_nodes, memFromPkt(8), &logfile, 
		&eventlist,ff,COMPOSITE,0);
#endif




#ifdef LEAF_SPINE_H
    int srvrsPerTor =32; //32; //16
    int torsPerPod = 16; //16;  //9
    int numPods = 1;
    LeafSpineTopology* top = new LeafSpineTopology(srvrsPerTor, torsPerPod,
        numPods, queuesize, &logfile, &eventlist, ff, COMPOSITE);
    cout << "topology created" << endl;
    no_of_nodes = top->no_of_nodes();
#endif

    cout << "actual nodes " << no_of_nodes << endl;

    vector<const Route*>*** net_paths;
    net_paths = new vector<const Route*>**[no_of_nodes];


    for (uint32_t i=0; i<no_of_nodes; i++){
        net_paths[i] = new vector<const Route*>*[no_of_nodes];
        for (uint32_t j = 0; j<no_of_nodes; j++)
            net_paths[i][j] = NULL;
    }

#if USE_FIRST_FIT
    if (ff)
	ff->net_paths = net_paths;
#endif

    // initialize all sources/sinks
    NdpSrc::setMinRTO(50000); //increase RTO to avoid spurious retransmits
    NdpSrc::setRouteStrategy(route_strategy);
    NdpSink::setRouteStrategy(route_strategy);

    // Record the setup
    int pktsize = Packet::data_packet_size();
    logfile.write("# pktsize=" + ntoa(pktsize) + " bytes");
    logfile.write("# hostnicrate = " + ntoa(HOST_NIC) + "Mbps");
    logfile.write("# corelinkrate = " + ntoa(HOST_NIC*CORE_TO_HOST) + "Mbps");
    double rtt = timeAsSec(timeFromUs(RTT));
    logfile.write("# rtt =" + ntoa(rtt));

    double lambda;

    vector<NdpLoadGen*> loadGens;
    vector<NdpReadTrace*> ReadTraces;
    vector<NdpRecvrAggr*> recvrAggrs;
    vector<NdpPullPacer*> pacers;
    for (size_t node = 0; node < no_of_nodes; node++) {
	    pacers.push_back(new NdpPullPacer(eventlist,1, HOST_NIC));
        recvrAggrs.push_back(new NdpRecvrAggr(eventlist, node, pacers.back()));
    }
	size_t node = 0;


    if (strcmp(flowfile.str().c_str(), ""))
    {
        for (; node < no_of_nodes; node++)
        {
            ReadTraces.push_back(new NdpReadTrace(eventlist, node, &recvrAggrs, top, cwnd,
                                                  &logfile, &ndpRtxScanner, net_paths, flowfile.str()));
        }
    }
    else
    {
        if(is_incast == 1){
            node = 1;	
        }else{
            node = 0;	
        }        
        vector<pair<uint64_t, double> > *wl =
            mesgSizeDistFromFile(distfile.str(), lambda);
        for (; node < no_of_nodes; node++)
        {
            loadGens.push_back(new NdpLoadGen(eventlist, node, &recvrAggrs, top, lambda, cwnd,
                                              wl, &logfile, &ndpRtxScanner, net_paths, is_incast));
        }
    }

    // GO!
    while (eventlist.doNextEvent()) {
    }

    for (size_t node = 0; node < no_of_nodes; node++) {
        delete recvrAggrs[node];
    }
    
    cout << "Simulation completed!" << endl;
    cout << "Done" << endl;

	
    /*
    list <const Route*>::iterator rt_i;
    int counts[10]; int hop;
    for (rt_i = routes.begin(); rt_i != routes.end(); rt_i++) {
	const Route* r = (*rt_i);
	//print_route(*r);
	cout << "Path:" << endl;
	for (int i = 0; i < r->size(); i++) {
	    PacketSink *ps = r->at(i);
	    CompositeQueue *q = dynamic_cast<CompositeQueue*>(ps);
	    if (q == 0) {
		cout << ps->nodename() << endl;
	    } else {
		cout << q->nodename() << " id=" << q->id << " " << q->num_packets() << "pkts "
		     << q->num_headers() << "hdrs " << q->num_acks() << "acks " << q->num_nacks() << "nacks " << q->num_stripped() << "stripped"
		     << endl;
	    }
	}
	cout << endl;
    }
    for (int i = 0; i < 10; i++)
	counts[i] = 0;
    for (rt_i = routes.begin(); rt_i != routes.end(); rt_i++) {
	const Route* r = (*rt_i);
	//print_route(*r);
	hop = 0;
	for (int i = 0; i < r->size(); i++) {
	    PacketSink *ps = r->at(i);
	    CompositeQueue *q = dynamic_cast<CompositeQueue*>(ps);
	    if (q == 0) {
	    } else {
		counts[hop] += q->num_stripped();
		q->_num_stripped = 0;
		hop++;
	    }
	}
	cout << endl;
    }

    for (int i = 0; i < 10; i++)
	cout << "Hop " << i << " Count " << counts[i] << endl;
    list <NdpSrc*>::iterator src_i;
    for (src_i = ndp_srcs.begin(); src_i != ndp_srcs.end(); src_i++) {
	cout << "Src, sent: " << (*src_i)->_packets_sent << "[new: " << (*src_i)->_new_packets_sent << " rtx: " << (*src_i)->_rtx_packets_sent << "] nacks: " << (*src_i)->_nacks_received << " pulls: " << (*src_i)->_pulls_received << " paths: " << (*src_i)->_paths.size() << endl;
    }
    for (src_i = ndp_srcs.begin(); src_i != ndp_srcs.end(); src_i++) {
	(*src_i)->print_stats();
    }
    uint64_t total_rtt = 0;
    cout << "RTT Histogram";
    for (int i = 0; i < 100000; i++) {
	if (NdpSrc::_rtt_hist[i]!= 0) {
	    cout << i << " " << NdpSrc::_rtt_hist[i] << endl;
	    total_rtt += NdpSrc::_rtt_hist[i];
	}
    }
    cout << "RTT CDF";
    uint64_t cum_rtt = 0;
    for (int i = 0; i < 100000; i++) {
	if (NdpSrc::_rtt_hist[i]!= 0) {
	    cum_rtt += NdpSrc::_rtt_hist[i];
	    cout << i << " " << double(cum_rtt)/double(total_rtt) << endl;
	}
    }
    */
}

string ntoa(double n) {
    stringstream s;
    s << n;
    return s.str();
}

string itoa(uint64_t n) {
    stringstream s;
    s << n;
    return s.str();
}

vector<pair<uint64_t, double> >*
mesgSizeDistFromFile(string filename, double& lambda)
{
    auto* wl = new vector<pair<uint64_t, double> >();
    ifstream distFileStream;
    distFileStream.open(filename.c_str());
    std::string avgMsgSizeStr;
    std::string sizeProbStr;

    // The first line of distFileName is the average message size of the
    // distribution.
    getline(distFileStream, avgMsgSizeStr);
    double avgMsgSize;
    sscanf(avgMsgSizeStr.c_str(), "%lf", &avgMsgSize);
    avgMsgSize *= (1500+ACKSIZE); // Avg size of message, in terms of bytes,
                                  // including header bytes

    double avgRate = (loadFactor / 100.0) * speedFromMbps((uint64_t)HOST_NIC); // In bps
    double avgInterArrivalTime = 1.0 * avgMsgSize * 8.0  / avgRate;
    lambda = 1.0 / avgInterArrivalTime;

    // reads msgSize<->probabilty pairs from "distFileName" file
    while(getline(distFileStream, sizeProbStr)) {
        uint64_t msgSize;
        double prob;
        sscanf(sizeProbStr.c_str(), "%llu %lf",
                &msgSize, &prob);
        wl->push_back(std::make_pair(msgSize*1500, prob));
    }
    distFileStream.close();
    return wl;
}

