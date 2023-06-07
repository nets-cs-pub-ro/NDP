#ifndef NDP_PAIRS_H
#define NDP_PAIRS_H

#include <list>
#include <map>
#include <unordered_set>
#include <vector>
#include <sstream>
#include <iostream>

#include "config.h"
#include "main.h"
#include "eventlist.h"
#include "network.h"
#include "logfile.h"
//#include "randomqueue.h"
#include "connection_matrix.h"
#include "ndp.h"

class NdpSinkPart;
class NdpRecvrAggr;
class NdpLoadGen;
struct FlowInput{
	uint32_t src, dst, pg, appid, message_size, port, dport;
	uint64_t start_time;
	uint32_t idx;
};

class NdpSrcPart : public NdpSrc {
  public:
	NdpSrcPart(NdpLogger* logger, TrafficLogger* pktLogger,
        EventList &eventlist, NdpLoadGen* owner);
	void connect(route_t& routeout, route_t& routeback, NdpSink& sink,
        simtime_picosec starttime);
	virtual void rtx_timer_hook(simtime_picosec now,simtime_picosec period);
	virtual void receivePacket(Packet& pkt);
    void setMesgSize(uint64_t mesgSize);
	void reset(uint64_t newMesgSize, bool shouldRestart);
	virtual void doNextEvent();

	uint64_t mesgSize;
	bool isActive;
  simtime_picosec last_active_time;
	simtime_picosec started;
    NdpLoadGen* loadGen; // backpointer to the NdpLoadGen that owns this source
	vector<route_t*>* paths;
    static uint64_t inflightMesgs;
    static int lastLogTime; // multiples of 10ms
};

class NdpSinkPart : public NdpSink {
  public:
    NdpSinkPart(NdpRecvrAggr* recvrAggr);
    virtual void receivePacket(Packet& pkt);
    void reset();
    NdpRecvrAggr* recvrAggr; //back pointer to NdpRecvrAggr owning this ndpsink
    bool recvCmplt;
    simtime_picosec first_receive_time; 
};

/**
 * This class keeps information about all NdpSinkPart instances that belong to
 * the same destination. Kind of an aggregator class for connections to the same
 * destination.
 */
class NdpRecvrAggr : public EventSource {
  public:
    NdpRecvrAggr(EventList& eventlist, int dest, NdpPullPacer* pacer);
    ~NdpRecvrAggr();
    virtual void doNextEvent(){}
    void markActive(NdpSinkPart* sink);
    void markInactive(NdpSinkPart* sink);

    int node; // the id of this node
    NdpPullPacer* pacer; // the pull-pacer associated with this receiver
    std::unordered_set<NdpSinkPart*> activeSinkNodes;// ndp sinks currently
                                                     // receiving messages
    bool inActivePeriod; // when there is at least one active sink
    simtime_picosec activeStart; // last time active started for this node
    simtime_picosec activeEnd; // last time active ended for this receiver
    simtime_picosec totalActiveTime; // sum of all the active periods so far
    simtime_picosec startTime;
    uint64_t dataBytesRecvd; // data received through all sinks while active
    uint64_t bytesRecvd; // all bytes received when active
}; //end class NdpRecvrAggr



/**
 * Collection of all ndp connection-pairs, from a certain source node i to
 * different sink nodes. This class generates and manages traffice from the one
 * sender to different receivers.
 */
class NdpLoadGen : public EventSource {
  public:
    typedef pair<NdpSrcPart*, NdpSinkPart*> NdpPair;
    typedef list<pair<NdpSrcPart*, NdpSinkPart*> > NdpPairList;

    NdpLoadGen(EventList& eventlist, int src, vector<NdpRecvrAggr*> *recvrAggrs,
        Topology* top, double lambda, int cwnd,
        vector<pair<uint64_t, double> > *wl, Logfile* logfile,
        NdpRtxTimerScanner* rtx, vector<const Route*>*** allRoutes, int is_incast);
    NdpLoadGen(EventList& eventlist, int src,
        vector<NdpRecvrAggr*>* recvrAggrs, Topology* top, 
        int cwnd, Logfile* logfile,
        NdpRtxTimerScanner* rtx, vector<const Route*>*** allRoutes);
    ~NdpLoadGen(){}
    virtual void doNextEvent();
    void run();
    uint64_t generateMesgSize();
    NdpPair& createConnection(int dest);
    string ntoa(double n);
    string itoa(uint64_t n);
    void printAllActive();
    //void printTopoPaths(vector<const Route*>* paths, int src);
    void copyRoute(const Route *route_src, route_t *route_dest);

    std::vector<NdpPairList> allNdpPairs;
    int src;
    vector<NdpRecvrAggr*> *recvrAggrs;
    Topology* topo;
    double lambda;
    int cwnd;
    vector<pair<uint64_t, double> > *wl;
    Logfile* logfile;
    NdpRtxTimerScanner* ndpRtxScanner;
    vector<const Route*>*** allRoutes;
    static int initConn;
	int is_incast;
}; //class NdpLoadGen

class NdpReadTrace : public NdpLoadGen {
  public:
    NdpReadTrace(EventList& eventlist, int src, vector<NdpRecvrAggr*> *recvrAggrs,
        Topology* top, int cwnd, Logfile* logfile,
        NdpRtxTimerScanner* rtx, vector<const Route*>*** allRoutes, string flowfile);
    ~NdpReadTrace(){}
    virtual void doNextEvent();
    void scheduleInput();
    void ReadFlowInput();

    string flowfile;
    uint32_t flow_num;
    ifstream flowf;
    FlowInput flow_input; 

}; //class NdpReadTrace

#endif //END NDP_PAIRS_H
