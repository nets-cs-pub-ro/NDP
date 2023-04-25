// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#include "config.h"
#include <sstream>
#include <string.h>
#include <strstream>
#include <iostream>
#include <math.h>
#include "network.h"
#include "pipe.h"
#include "eventlist.h"
#include "logfile.h"
#include "loggers.h"
#include "tcp_transfer.h"
#include "clock.h"
#include "cc.h"
#include "compositequeue.h"

string ntoa(double n);
string itoa(uint64_t n);

// Simulation params

int main(int argc, char **argv) {
    EventList eventlist;
    Clock c(timeFromSec(50/100.), eventlist);

    srand(time(NULL));

    Packet::set_packet_size(1500);    
    linkspeed_bps SERVICE1 = speedFromMbps((uint64_t)1000);
    simtime_picosec latency=timeFromMs((int)1);
    mem_b queuesize=300;
    int no_of_conns = 2;
    stringstream filename(ios_base::out);
    filename << "logout.dat";
    int end_time = 5000;
    int startdelta = 0;
    double log_interval = 1;
    
    int i = 1;
    while (i<argc) {
        if (!strcmp(argv[i],"-o")) {
            filename.str(std::string());
            filename << argv[i+1];
            i++;
        } else if (!strcmp(argv[i],"-conns")) {
            no_of_conns = atoi(argv[i+1]);
            cout << "no_of_conns "<<no_of_conns << endl;
            i++;
        } else if (!strcmp(argv[i],"-end")) {
            end_time = atoi(argv[i+1]);
            cout << "endtime(ms) "<< end_time << endl;
            i++;
	} else if (!strcmp(argv[i],"-q")){
            queuesize = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-latency")){
            latency = timeFromMs(atof(argv[i+1]));
            cout << "Latency set to " << timeAsMs(latency) << endl;
            i++;
        } else if (!strcmp(argv[i],"-bandwidth")){
            SERVICE1 = speedFromMbps((uint64_t)atoi(argv[i+1]));
            cout << "Bandwidth set to " << atoi(argv[i+1]) << "Mbps"<<endl;
            i++;
        } else if (!strcmp(argv[i],"-startdelta")){
            startdelta = atoi(argv[i+1]);
            cout << "Start delta to " << atoi(argv[i+1]) << "ms"<<endl;
            i++;
        } else if (!strcmp(argv[i],"-log")){
            log_interval = atof(argv[i+1]);
            cout << "Log interval set to " << log_interval << "ms"<<endl;
            i++;
        }
	else {
	    cout << "Unknown parameter "<< argv[i] << endl;
	    cout << "Usage " << argv[0] << "\t-o logfile\n\t-conns number of connections\n\t-end simtime in ms\n\t-q queuesize in packets \n\t-latency latency in ms\n\t-bandwidth speed in Mbps" << endl; 
	    exit(1);
	}
	i++;
    }

    queuesize = memFromPkt(queuesize);
    
    cout << "Outputting to " << filename.str() << endl;
    Logfile logfile(filename.str(),eventlist);
    eventlist.setEndtime(timeFromMs((int)end_time));
  
    logfile.setStartTime(timeFromSec(0.0));

    CCSinkLoggerSampling sinkLogger = CCSinkLoggerSampling(timeFromMs(log_interval),eventlist);
    QueueLoggerSampling queueLogger = QueueLoggerSampling(timeFromMs(log_interval),eventlist);
    logfile.addLogger(sinkLogger);
    logfile.addLogger(queueLogger);

    Pipe pipe1(latency, eventlist); pipe1.setName("pipe1"); logfile.writeName(pipe1);
    Pipe pipe2(latency, eventlist); pipe2.setName("pipe2"); logfile.writeName(pipe2);

    CompositeQueue queue(SERVICE1, queuesize, eventlist,&queueLogger); queue.setName("Queue1"); logfile.writeName(queue);
    CompositeQueue queue2(SERVICE1, queuesize, eventlist,NULL); queue2.setName("Queue2"); logfile.writeName(queue2);

    Queue queue3(SERVICE1, queuesize*20, eventlist,NULL); queue3.setName("Queue3"); logfile.writeName(queue3);
    
    CCSrc* ccSrc;
    CCSink* ccSnk;

    route_t* routeout;
    route_t* routein;

    for (int i=0;i<no_of_conns;i++){
	ccSrc = new CCSrc(eventlist);
	logfile.writeName(*ccSrc);

	ccSnk = new CCSink(); 
	logfile.writeName(*ccSnk);
	
	// tell it the route
	routeout = new route_t(); 
	routeout->push_back(new Queue(SERVICE1*2, memFromPkt(1000),eventlist, NULL));
	routeout->push_back(&queue); 
	routeout->push_back(&pipe1);
	routeout->push_back(ccSnk);
	
	routein  = new route_t();
	routeout->push_back(&queue2); 
	routein->push_back(&pipe1);
	routein->push_back(ccSrc); 

	ccSrc->connect(routeout, routein, *ccSnk,timeFromMs(i*startdelta));
	sinkLogger.monitorSink(ccSnk);
    }

    // Record the setup
    int pktsize = Packet::data_packet_size();
    logfile.write("# pktsize="+ntoa(pktsize)+" bytes");
    //	logfile.write("# buffer2="+ntoa((double)(queue2._maxsize)/((double)pktsize))+" pkt");
    double rtt = timeAsSec(latency);
    logfile.write("# rtt="+ntoa(rtt));

    // GO!
    while (eventlist.doNextEvent()) {}
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
