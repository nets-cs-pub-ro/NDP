// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <math.h>
using namespace std;

#ifdef __clang__
#include <unordered_map>
#define hashmap unordered_map
#else
#include <ext/hash_map>
#define hashmap __gnu_cxx::hash_map
#endif

#include <vector>

#include "loggers.h"

struct eqint
{
  bool operator()(int s1, int s2) const
  {
    return s1==s2;
  }
};

int main(int argc, char** argv){
    if (argc < 2){
	printf("Usage %s filename [-show|-verbose|-ascii]\n", argv[0]);
	return 1;
    }

    int show = 0, verbose = 0, ascii = 0;

    if ((argc>2 && !strcmp(argv[2], "-show"))
	|| (argc>3 && !strcmp(argv[3], "-show")))
	show = 1;

    if ((argc>2 && !strcmp(argv[2], "-ascii"))
	|| (argc>3 && !strcmp(argv[3], "-ascii")))
	ascii = 1;

    if ((argc>2 && !strcmp(argv[2], "-verbose"))
	|| (argc>3 && !strcmp(argv[3], "-verbose")))
	verbose = 1;

    if (ascii && (show || verbose)) {
	    perror("Use -ascii by itself, not with -show or -verbose!\n");
	    exit(1);
    }

    FILE* logfile;
    hashmap<int, string> object_names;

    logfile = fopen(argv[1], "rbS");
    if (logfile==NULL) {
	cerr << "Failed to open logfile " << argv[1] << endl;
	exit(1);
    }

    //parse preamble first
    char* line = new char[10000];

    int numRecords = 0, transpose = 1;
    while (1){
	if(!fgets(line, 10000, logfile)) {
	    perror("File ended while reading preamble!\n");
	    exit(1);
	}
	if (strstr(line, "# TRACE")) {
	    //we have finished the preamble;
	    if(numRecords<=0) {
		printf("Numrecords is %d after preamble, bailing\n", numRecords);
		exit(1);
	    }
	    break;
	}

	if (strstr(line, "# numrecords=")) {
	    numRecords = atoi(line+13);
	};

	if (strstr(line, "# transpose=")) {
	    transpose = atoi(line+12);
	};

	//
	if (strstr(line, ": ")){
	    //logged names and ids
	    char* split = strstr(line,"=");

	    int id;
	    if (split)
		id = atoi(split+1);
	    
	    split[0]=0;
	    string * name = new string(line+2);

	    object_names[id] = *name;
	}
    }

    //must find the number of records here, and go to #TRACE

    int numread = numRecords;

    double* timeRec = new double[numRecords];
    uint32_t* typeRec = new uint32_t[numRecords];
    uint32_t* idRec = new uint32_t[numRecords];
    uint32_t* evRec = new uint32_t[numRecords];
    double* val1Rec = new double[numRecords];
    double* val2Rec = new double[numRecords];
    double *val3Rec = new double[numRecords];

    if (transpose) {
	/* old-style transposed data */
	fread(timeRec, sizeof(double), numread, logfile);
	fread(typeRec, sizeof(uint32_t), numread, logfile);
	fread(idRec,   sizeof(uint32_t), numread, logfile);
	fread(evRec,   sizeof(uint32_t), numread, logfile);
	fread(val1Rec, sizeof(double), numread, logfile);
	fread(val2Rec, sizeof(double), numread, logfile);
	fread(val3Rec, sizeof(double), numread, logfile);  
    } else {
	/* new-style one record at a time */
	for (int i = 0; i < numRecords; i++) {
	    fread(&timeRec[i], sizeof(double), 1, logfile);
	    fread(&typeRec[i], sizeof(uint32_t), 1, logfile);
	    fread(&idRec[i],   sizeof(uint32_t), 1, logfile);
	    fread(&evRec[i],   sizeof(uint32_t), 1, logfile);
	    fread(&val1Rec[i], sizeof(double), 1, logfile);
	    fread(&val2Rec[i], sizeof(double), 1, logfile);
	    fread(&val3Rec[i], sizeof(double), 1, logfile);  
	}
    }

    //type=mtcp
    //ev=rate
    //group by ID

    //lets compute 
    hashmap<int, double> flow_rates;
    hashmap<int, double> flow_count;

    hashmap<int, double> flow_rates2;
    hashmap<int, double> flow_count2;

    int TYPE = 11, EV = 1100;

    if (argc>2&&!strcmp(argv[2], "-memory")) {
	TYPE = 12; EV = 1204;
    } else if (argc>2&&!strcmp(argv[2], "-tcp")) {
	TYPE = 11; EV = 1100;
    } else if (argc>2&&!strcmp(argv[2], "-tcp_cwnd")) {
	TYPE = 2; EV = 200;
    } else if (argc>2&&!strcmp(argv[2], "-tcp_seq")) {
	TYPE = 2; EV = 201;
    } else if (argc>2&&!strcmp(argv[2], "-ndp")) {
	TYPE = 18; EV = 1800;
    } else if (argc>2&&!strcmp(argv[2], "-mptcp")) {
	TYPE = 12; EV = 1203;
    } else if (argc>2&&!strcmp(argv[2], "-mptcp-cwnd")) {
	TYPE = 12; EV = 1202;
    } else if (argc>2&&!strcmp(argv[2], "-mptcp-alfa")) {
	TYPE = 12; EV = 1200;
    } else if (argc>2&&!strcmp(argv[2], "-queue")) {
	TYPE = 5; EV = 500;
    } else if (argc>2&&!strcmp(argv[2], "-queue-verbose")) {
	TYPE = 0; EV = 0;
    } else if (argc>2&&!strcmp(argv[2], "-all")) {
	TYPE = -1; EV = -1;
    }

    for (int i=0;i<numRecords;i++){
	if (!timeRec[i])
	    continue;

	if (ascii) {
	    RawLogEvent event(timeRec[i], typeRec[i], idRec[i], evRec[i], 
			      val1Rec[i], val2Rec[i], val3Rec[i]);
	    //cout << Logger::event_to_str(event) << endl;
	    switch((Logger::EventType)typeRec[i]) {
	    case Logger::QUEUE_EVENT: //0
		cout << QueueLoggerSimple::event_to_str(event) << endl;
		break;
	    case Logger::TCP_EVENT: //1
	    case Logger::TCP_STATE: //2
		cout << TcpLoggerSimple::event_to_str(event) << endl; 
		break;
	    case Logger::TRAFFIC_EVENT: //3
		cout << TrafficLoggerSimple::event_to_str(event) << endl; 
		break;
	    case Logger::QUEUE_RECORD: //4
	    case Logger::QUEUE_APPROX: //5
		cout << QueueLoggerSampling::event_to_str(event) << endl;
		break;
	    case Logger::TCP_RECORD: //6
		cout << AggregateTcpLogger::event_to_str(event) << endl;
		break;
	    case Logger::QCN_EVENT: //7
	    case Logger::QCNQUEUE_EVENT: //8
		cout << QcnLoggerSimple::event_to_str(event) << endl;
		break;
	    case Logger::TCP_TRAFFIC: //9
		cout << TcpTrafficLogger::event_to_str(event) << endl;
		break;
	    case Logger::NDP_TRAFFIC: //10
		cout << NdpTrafficLogger::event_to_str(event) << endl;
		break;
	    case Logger::TCP_SINK: //11
		cout << TcpSinkLoggerSampling::event_to_str(event) << endl;
		break;
	    case Logger::MTCP: //12
		cout << MultipathTcpLoggerSimple::event_to_str(event) << endl;
		break;
	    case Logger::ENERGY: //13
		// not currently used, so use default logger
		cout << Logger::event_to_str(event) << endl;
		break;
	    case Logger::TCP_MEMORY: //14
		cout << MemoryLoggerSampling::event_to_str(event) << endl;
		break;
	    case Logger::NDP_EVENT: //15
	    case Logger::NDP_STATE: //16
	    case Logger::NDP_RECORD: //17
	    case Logger::NDP_MEMORY: //19
		// not currently used, so use default logger
		cout << Logger::event_to_str(event) << endl;
		break;
	    case Logger::NDP_SINK: //18
		cout << NdpSinkLoggerSampling::event_to_str(event) << endl;
		break;
	    }
	} else {
	    if ((typeRec[i]==(uint32_t)TYPE || TYPE==-1) 
		&& (evRec[i]==(uint32_t)EV || EV==-1)) {
		if (verbose)
		    cout << timeRec[i] << " Type=" << typeRec[i] << " EV=" << evRec[i] 
			 << " ID=" << idRec[i] << " VAL1=" << val1Rec[i] 
			 << " VAL2=" << val2Rec[i] << " VAL3=" << val3Rec[i] << endl;	

		if (!isnan(val3Rec[i])) {
		    if (flow_rates.find(idRec[i]) == flow_rates.end()){
			flow_rates[idRec[i]] = val3Rec[i];
			flow_count[idRec[i]] = 1;
		    } else {
			flow_rates[idRec[i]] += val3Rec[i];
			flow_count[idRec[i]]++;
		    }
		}

		if (!isnan(val2Rec[i])) {
		    if (flow_rates2.find(idRec[i]) == flow_rates2.end()) {
			flow_rates2[idRec[i]] = val2Rec[i];
			flow_count2[idRec[i]] = 1;
		    } else {
			flow_rates2[idRec[i]]+= val2Rec[i];
			flow_count2[idRec[i]]++;
		    }
		}
	    }
	}
    }
    if (ascii) {
	exit(0);
    }

    //now print rates;
    vector<double> rates;

    double mean_rate2 = 0;
    hashmap <int, double> :: iterator it2;
    it2 = flow_rates2.begin ( );
    while (it2!=flow_rates2.end()){
	int id = it2->first;
	double r = it2->second/flow_count2[id];
	mean_rate2 += r;
	it2++;
    }

    hashmap <int, double> :: iterator it;
    it = flow_rates.begin ( );
    double mean_rate = 0;
    while (it!=flow_rates.end()){
	int id = it->first;
	//cout << "Flow with ID " << id << " has mean rate " << it->second/flow_count[id] << endl;
	double r = it->second/flow_count[id];
	//printf("%f %d\n", r, id);
	mean_rate += r;
	rates.push_back(r);

	if (show)
	    printf("%.2f Mbps val %d name %s\n", r*8/1000000,id,object_names[id].c_str());

	it++;
    }

    sort(rates.begin(), rates.end());
  
    double total = 0;
    int cnt = rates.size()/10;
    int nn = rates.size();
    if (cnt<1) cnt = 1;

    for (int i=0;i<nn;i++){
	if (i<cnt)
	    total += rates[i]; 

	if (i>cnt&&!show)
	    break;
    }
    printf("Mean of lower 10pc (%d entries) is %f total mean %f  mean2 %f\n", 
	   cnt, total/cnt, mean_rate/rates.size(), 
	   mean_rate2/flow_rates2.size());
  
    delete[] timeRec;
    delete[] typeRec;
    delete[] idRec;
    delete[] evRec;
    delete[] val1Rec;
    delete[] val2Rec;
    delete[] line;
}
