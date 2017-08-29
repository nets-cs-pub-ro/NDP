// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        


#ifndef NDP_H
#define NDP_H

/*
 * A NDP source and sink
 */

#include <list>
#include <map>
#include "config.h"
#include "network.h"
#include "ndppacket.h"
#include "fairpullqueue.h"
#include "eventlist.h"

#define timeInf 0
#define NDP_PACKET_SCATTER

//#define LOAD_BALANCED_SCATTER

//min RTO bound in us
// *** don't change this default - override it by calling NdpSrc::setMinRTO()
#define DEFAULT_RTO_MIN 5000

#define RECORD_PATH_LENS // used for debugging which paths lengths packets were trimmed on - mostly useful for BCube

enum RouteStrategy {NOT_SET, SINGLE_PATH, SCATTER_PERMUTE, SCATTER_RANDOM, PULL_BASED};

class NdpSink;
class ReceiptEvent {
  public:
    ReceiptEvent()
	: _path_id(-1), _is_header(false) {};
    ReceiptEvent(uint32_t path_id, bool is_header)
	: _path_id(path_id), _is_header(is_header) {}
    inline int32_t path_id() const {return _path_id;}
    inline bool is_header() const {return _is_header;}
    int32_t _path_id;
    bool _is_header;
};

class NdpSrc : public PacketSink, public EventSource {
    friend class NdpSink;
 public:
    NdpSrc(NdpLogger* logger, TrafficLogger* pktlogger, EventList &eventlist);
    uint32_t get_id(){ return id;}
    virtual void connect(Route& routeout, Route& routeback, NdpSink& sink, simtime_picosec startTime);
    void set_traffic_logger(TrafficLogger* pktlogger);
    void startflow();
    void setCwnd(uint32_t cwnd) {_cwnd = cwnd;}
    static void setMinRTO(uint32_t min_rto_in_us) {_min_rto = timeFromUs((uint32_t)min_rto_in_us);}
    static void setRouteStrategy(RouteStrategy strat) {_route_strategy = strat;}
    void set_flowsize(uint64_t flow_size_in_bytes) {
	_flow_size = flow_size_in_bytes;
    }


    virtual void doNextEvent();
    virtual void receivePacket(Packet& pkt);

    virtual void processRTS(NdpPacket& pkt);
    virtual void processAck(const NdpAck& ack);
    virtual void processNack(const NdpNack& nack);

    void replace_route(Route* newroute);

    virtual void rtx_timer_hook(simtime_picosec now,simtime_picosec period);
    void set_paths(vector<const Route*>* rt);

    // should really be private, but loggers want to see:
    uint64_t _highest_sent;  //seqno is in bytes
    uint64_t _packets_sent;
    uint64_t _new_packets_sent;
    uint64_t _rtx_packets_sent;
    uint64_t _acks_received;
    uint64_t _nacks_received;
    uint64_t _pulls_received;
    uint64_t _implicit_pulls;
    uint64_t _bounces_received;
    uint32_t _cwnd;
    uint64_t _last_acked;
    uint32_t _flight_size;
    uint32_t _acked_packets;

    // the following are used with SCATTER_PERMUTE, SCATTER_RANDOM and
    // PULL_BASED route strategies
    uint16_t _crt_path;
    uint16_t _crt_direction;
    vector<const Route*> _paths;
    vector<const Route*> _original_paths; //paths in original permutation order
    vector<int> _path_counts_new; // only used for debugging, can remove later.
    vector<int> _path_counts_rtx; // only used for debugging, can remove later.
    vector<int> _path_counts_rto; // only used for debugging, can remove later.

    vector <int32_t> _path_acks; //keeps path scores
    vector <int32_t> _path_nacks; //keeps path scores
    vector <bool> _bad_path; //keeps path scores
    vector <int32_t> _avoid_ratio; //keeps path scores
    vector <int32_t> _avoid_score; //keeps path scores

    map<NdpPacket::seq_t, simtime_picosec> _sent_times;
    map<NdpPacket::seq_t, simtime_picosec> _first_sent_times;

    void print_stats();

    int _pull_window; // Used to keep track of expected pulls so we
                      // can handle return-to-sender cleanly.
                      // Increase by one for each Ack/Nack received.
                      // Decrease by one for each Pull received.
                      // Indicates how many pulls we expect to
                      // receive, if all currently sent but not yet
                      // acked/nacked packets are lost
                      // or are returned to sender.
    int _first_window_count;

    //round trip time estimate, needed for coupled congestion control
    simtime_picosec _rtt, _rto, _mdev,_base_rtt;

    uint16_t _mss;
 
    uint32_t _drops;

    NdpSink* _sink;
 
    simtime_picosec _rtx_timeout;
    bool _rtx_timeout_pending;
    const Route* _route;

    const Route *choose_route();

    void pull_packets(NdpPull::seq_t pull_no, NdpPull::seq_t pacer_no);
    void send_packet(NdpPull::seq_t pacer_no);

    virtual const string& nodename() { return _nodename; }
    inline uint32_t flow_id() const { return _flow.flow_id();}
 
    //debugging hack
    void log_me();
    bool _log_me;

    static uint32_t _global_rto_count;  // keep track of the total number of timeouts across all srcs
    static simtime_picosec _min_rto;
    static RouteStrategy _route_strategy;
    static int _global_node_count;
    static int _rtt_hist[10000000];
    int _node_num;

 private:
    // Housekeeping
    NdpLogger* _logger;
    TrafficLogger* _pktlogger;
    // Connectivity
    PacketFlow _flow;
    string _nodename;

    enum  FeedbackType {ACK, NACK, BOUNCE, UNKNOWN};
    static const int HIST_LEN=12;
    FeedbackType _feedback_history[HIST_LEN];
    int _feedback_count;

    // Mechanism
    void clear_timer(uint64_t start,uint64_t end);
    void retransmit_packet();
    void permute_paths();
    void update_rtx_time();
    void process_cumulative_ack(NdpPacket::seq_t cum_ackno);
    inline void count_ack(int32_t path_id) {count_feedback(path_id, ACK);}
    inline void count_nack(int32_t path_id) {count_feedback(path_id, NACK);}
    inline void count_bounce(int32_t path_id) {count_feedback(path_id, BOUNCE);}
    void count_feedback(int32_t path_id, FeedbackType fb);
    bool is_bad_path();
    void log_rtt(simtime_picosec sent_time);
    NdpPull::seq_t _last_pull;
    uint64_t _flow_size;  //The flow size in bytes.  Stop sending after this amount.
    list <NdpPacket*> _rtx_queue; //Packets queued for (hopefuly) imminent retransmission
};

class NdpPullPacer;

class NdpSink : public PacketSink, public DataReceiver, public Logged {
    friend class NdpSrc;
 public:
    NdpSink(EventList& ev, double pull_rate_modifier);
    NdpSink(NdpPullPacer* pacer);
 

    uint32_t get_id(){ return id;}
    void receivePacket(Packet& pkt);
    NdpAck::seq_t _cumulative_ack; // the packet we have cumulatively acked
    uint32_t _drops;
    uint64_t cumulative_ack() { return _cumulative_ack + _received.size()*9000;}
    uint64_t total_received() const { return _total_received;}
    uint32_t drops(){ return _src->_drops;}
    virtual const string& nodename() { return _nodename; }
    void increase_window() {_pull_no++;} 
    static void setRouteStrategy(RouteStrategy strat) {_route_strategy = strat;}

    list<NdpAck::seq_t> _received; // list of packets above a hole, that we've received
 
    NdpSrc* _src;

    //debugging hack
    void log_me();
    bool _log_me;

    void set_paths(vector<const Route*>* rt);

#ifdef RECORD_PATH_LENS
#define MAX_PATH_LEN 20
    vector<uint32_t> _path_lens;
    vector<uint32_t> _trimmed_path_lens;
#endif
    static RouteStrategy _route_strategy;

 private:
 
    // Connectivity
    void connect(NdpSrc& src, Route& route);

    inline uint32_t flow_id() const {
	return _src->flow_id();
    };

    // the following are used with SCATTER_PERMUTE, SCATTER_RANDOM,
    // and PULL_BASED route strategies
    uint16_t _crt_path;
    uint16_t _crt_direction;
    vector<const Route*> _paths; //paths in current permutation order
    vector<const Route*> _original_paths; //paths in original permutation order
    const Route* _route;

   string _nodename;
 
    NdpPullPacer* _pacer;
    NdpPull::seq_t _pull_no; // pull sequence number (local to connection)
    NdpPacket::seq_t _last_packet_seqno; //sequence number of the last
                                         //packet in the connection (or 0 if not known)
    uint64_t _total_received;
 
    // Mechanism
    void send_ack(simtime_picosec ts, NdpPacket::seq_t ackno, NdpPacket::seq_t pacer_no);
    void send_nack(simtime_picosec ts, NdpPacket::seq_t ackno, NdpPacket::seq_t pacer_no);
    void permute_paths();
    
    //Path History
    void update_path_history(const NdpPacket& p);
#define HISTORY_PER_PATH 4 //how much history to hold - we hold an
			   //average of HISTORY_PER_PATH entries for
			   //each possible path
    vector<ReceiptEvent> _path_history;  //this is a bit heavyweight,
					 //but it will let us
					 //experiment with different
					 //algorithms
    int _path_hist_index; //index of last entry to be added to _path_history
    int _path_hist_first; //index of oldest entry added to _path_history
    int _no_of_paths;
};

class NdpPullPacer : public EventSource {
 public:
    NdpPullPacer(EventList& ev, double pull_rate_modifier);  
    NdpPullPacer(EventList& ev, char* fn);  
    // pull_rate_modifier is the multiplier of link speed used when
    // determining pull rate.  Generally 1 for FatTree, probable 2 for BCube
    // as there are two distinct paths between each node pair.

    void sendPacket(Packet* p, NdpPacket::seq_t pacerno, NdpSink *receiver);
    virtual void doNextEvent();
    uint32_t get_id(){ return id;}
    void release_pulls(uint32_t flow_id);

    //debugging hack
    void log_me();
    bool _log_me;

    void set_preferred_flow(int id) { _preferred_flow = id;cout << "Preferring flow "<< id << endl;};

 private:
    void set_pacerno(Packet *pkt, NdpPull::seq_t pacer_no);
//#define FIFO_PULL_QUEUE
#ifdef FIFO_PULL_QUEUE
    FifoPullQueue<NdpPull> _pull_queue;
#else
    FairPullQueue<NdpPull> _pull_queue;
#endif
    simtime_picosec _last_pull;
    simtime_picosec _packet_drain_time;
    NdpPull::seq_t _pacer_no; // pull sequence number, shared by all connections on this pacer

    //pull distribution from real life
    static int _pull_spacing_cdf_count;
    static double* _pull_spacing_cdf;

    //debugging
    double _total_excess;
    int _excess_count;
    int _preferred_flow;
};


class NdpRtxTimerScanner : public EventSource {
 public:
    NdpRtxTimerScanner(simtime_picosec scanPeriod, EventList& eventlist);
    void doNextEvent();
    void registerNdp(NdpSrc &tcpsrc);
 private:
    simtime_picosec _scanPeriod;
    typedef list<NdpSrc*> tcps_t;
    tcps_t _tcps;
};

#endif

