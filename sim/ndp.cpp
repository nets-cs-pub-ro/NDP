// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-    
#include <math.h>
#include <iostream>
#include "ndp.h"
#include "queue.h"
#include <stdio.h>

////////////////////////////////////////////////////////////////
//  NDP SOURCE
////////////////////////////////////////////////////////////////

/* When you're debugging, sometimes it's useful to enable debugging on
   a single NDP receiver, rather than on all of them.  Set this to the
   node ID and recompile if you need this; otherwise leave it
   alone. */
//#define LOGSINK 2332
#define LOGSINK 0

/* We experimented with adding extra pulls to cope with scenarios
   where you've got a bad link and pulls get dropped.  Generally you
   don't want to do this though, so best leave RCV_CWND set to
   zero. Lost pulls are well handled by the cumulative pull number. */
//#define RCV_CWND 15
#define RCV_CWND 0

int NdpSrc::_global_node_count = 0;
/* _rtt_hist is used to build a histogram of RTTs.  The index is in
   units of microseconds, and RTT is from when a packet is first sent
   til when it is ACKed, including any retransmissions.  You can read
   this out after the sim has finished if you care about this. */
int NdpSrc::_rtt_hist[10000000] = {0};

/* keep track of RTOs.  Generally, we shouldn't see RTOs if
   return-to-sender is enabled.  Otherwise we'll see them with very
   large incasts. */
uint32_t NdpSrc::_global_rto_count = 0;

/* _min_rto can be tuned using SetMinRTO. Don't change it here.  */
simtime_picosec NdpSrc::_min_rto = timeFromUs((uint32_t)DEFAULT_RTO_MIN);

// You MUST set a route strategy.  The default is to abort without
// running - this is deliberate!
RouteStrategy NdpSrc::_route_strategy = NOT_SET;
RouteStrategy NdpSink::_route_strategy = NOT_SET;

NdpSrc::NdpSrc(NdpLogger* logger, TrafficLogger* pktlogger, EventList &eventlist)
    : EventSource(eventlist,"ndp"),  _logger(logger), _flow(pktlogger)
{
    _mss = Packet::data_packet_size();

    _base_rtt = timeInf;
    _acked_packets = 0;
    _packets_sent = 0;
    _new_packets_sent = 0;
    _rtx_packets_sent = 0;
    _acks_received = 0;
    _nacks_received = 0;
    _pulls_received = 0;
    _implicit_pulls = 0;
    _bounces_received = 0;

    _flight_size = 0;

    _highest_sent = 0; // the last bytes sent
    _last_acked = 0;

    _sink = 0;

    _rtt = 0;
    _rto = timeFromMs(20);
    _cwnd = 15 * Packet::data_packet_size();
    _mdev = 0;
    _drops = 0;
    _flow_size = ((uint64_t)1)<<63;
    _last_pull = 0;
    _pull_window = 0;
  
    _crt_path = 0; // used for SCATTER_PERMUTE route strategy

    _feedback_count = 0;
    for(int i = 0; i < HIST_LEN; i++)
	_feedback_history[i] = UNKNOWN;

    _rtx_timeout_pending = false;
    _rtx_timeout = timeInf;
    _node_num = _global_node_count++;
    _nodename = "ndpsrc" + to_string(_node_num);
	cout << "new Ndpsrc _nodename " << _nodename << endl;

    // debugging hack
    _log_me = false;
}

void NdpSrc::set_traffic_logger(TrafficLogger* pktlogger) {
    _flow.set_logger(pktlogger);
}

void NdpSrc::log_me() {
    // avoid looping
    if (_log_me == true)
	return;
    cout << "Enabling logging on NdpSrc " << _nodename << endl;
    _log_me = true;
    if (_sink)
	_sink->log_me();
}

void NdpSrc::set_paths(vector<const Route*>* rt_list){
    int no_of_paths = rt_list->size();
    switch(_route_strategy) {
    case NOT_SET:
    case SINGLE_PATH:
	// shouldn't call this with these strategies
	abort();
    case SCATTER_PERMUTE:
    case SCATTER_RANDOM:
    case PULL_BASED:
	_paths.resize(no_of_paths);
        _original_paths.resize(no_of_paths);
        _path_acks.resize(no_of_paths);
        _path_nacks.resize(no_of_paths);
        _bad_path.resize(no_of_paths);
        _avoid_ratio.resize(no_of_paths);
        _avoid_score.resize(no_of_paths);
	_path_counts_new.resize(no_of_paths);
	_path_counts_rtx.resize(no_of_paths);
	_path_counts_rto.resize(no_of_paths);

	for (unsigned int i=0; i < no_of_paths; i++){
	    Route* tmp = new Route(*(rt_list->at(i)));
	    tmp->add_endpoints(this, _sink);
	    tmp->set_path_id(i, rt_list->size());
	    _paths[i] = tmp;
	    _original_paths[i] = tmp;
	    _path_counts_new[i] = 0;
	    _path_counts_rtx[i] = 0;
	    _path_counts_rto[i] = 0;
	    _path_acks[i] = 0;
	    _path_nacks[i] = 0;
	    _avoid_ratio[i] = 0;
	    _avoid_score[i] = 0;
	    _bad_path[i] = false;
	}

	_crt_path = 0;
	permute_paths();
	break;
    }
}

void NdpSrc::startflow(){
    _highest_sent = 0;
    _last_acked = 0;
    
    _acked_packets = 0;
    _packets_sent = 0;
    _rtx_timeout_pending = false;
    _rtx_timeout = timeInf;
    _pull_window = 0;
    
    _flight_size = 0;
    _first_window_count = 0;

	//yanfang: reuse the connection
	_rtx_queue.clear();
	_sent_times.clear();
	_first_sent_times.clear();

    while (_flight_size < _cwnd && _flight_size < _flow_size) {
	send_packet(0);
	_first_window_count++;
    }
}

void NdpSrc::connect(Route& routeout, Route& routeback, NdpSink& sink, simtime_picosec starttime) {
    _route = &routeout;
    assert(_route);
    
    _sink = &sink;
    _flow.id = id; // identify the packet flow with the NDP source that generated it
    _flow._name = _name;
    _sink->connect(*this, routeback);
  
    eventlist().sourceIsPending(*this,starttime);

    //debugging hacks
    if (sink.get_id()==LOGSINK) {
	cout << "Found source for " << LOGSINK << "\n";
	_log_me = true;
    }
}

#define ABS(X) ((X)>0?(X):-(X))

void NdpSrc::count_feedback(int32_t path_id, FeedbackType fb) {
    if (_route_strategy == SINGLE_PATH)
	return;

    int32_t sz = _paths.size();
    // keep feedback history in a circular buffer
    _feedback_history[_feedback_count] = fb;
    _feedback_count = (_feedback_count + 1) % HIST_LEN;
    switch (fb) {
    case ACK:
	_path_acks[path_id]++;
	break;
    case NACK:
	_path_nacks[path_id]++;
	break;
    case BOUNCE:
	_path_nacks[path_id]+=3;  //a bounce is kind of a more severe Nack for this purpose
	break;
    case UNKNOWN:
	//not possible, but keep compiler calm
	abort();
    }
    int path_acks_total = 0;
    int path_nacks_total = 0;
    for (int i = 0; i < sz; i++) {
	path_acks_total += _path_acks[i];
	path_nacks_total += _path_nacks[i];
    }
    int path_acks_mean = path_acks_total / sz;
    int path_nacks_mean = path_nacks_total / sz;

    int nack_ratio = 0; 
    if (_path_acks[path_id] > 10)
	nack_ratio = (_path_nacks[path_id]*100)/_path_acks[path_id];
    int mean_nack_ratio = 100;
    if (path_acks_mean > 0) 
	mean_nack_ratio = (path_nacks_mean*100)/path_acks_mean;
    // criteria for a bad path.
    // 1.  nack count > 125% of the mean nack count
    // 2.  nack ratio > 30%
    // 3.  total acks+nacks > 100, so we don't react to noisy startup data
    if ((_path_acks[path_id] + _path_nacks[path_id] > 100) &&
	(nack_ratio > mean_nack_ratio*1.25) &&
	(nack_ratio > 30)) {
	_bad_path[path_id] = true;
	_avoid_ratio[path_id]++;
    } else {
	if (_avoid_ratio[path_id] > 0)
	    _avoid_ratio[path_id]--;
	_bad_path[path_id] = false;
    }
}

bool NdpSrc::is_bad_path() {
    // We've just got a return-to-sender.  Either all paths are
    // congested, in which case the path is not bad, or just this one
    // is.  Look at the immediate history to tell the difference.
    int bounce_count = 0, ack_count = 0, nack_count = 0, total = 0;
    for (int i=0; i< HIST_LEN; i++) {
	switch (_feedback_history[i]) {
	case ACK:
	    ack_count++;
	    break;
	case NACK:
	    nack_count++;
	    break;
	case BOUNCE:
	    bounce_count++;
	    break;
	case UNKNOWN:
	    break;
	}
    }
    total = ack_count + nack_count + bounce_count;
    // If we get a return-to-sender due to incast, all the paths
    // should be very overloaded. When we're just on the threshold for
    // getting an RTS, there's a full queue of headers at that switch,
    // and should be something pretty similar at switches on other
    // paths.  Thus almost all packets being sent are resulting in
    // NACKs.  If our history shows at least 25% ACKs, the net is not
    // really congested on aggregate, so this is likely just a bad
    // path.
    if (ack_count > 0 && total/ack_count <= 3) {
	printf("total: %d ack: %d nack:%d rts: %d, BAD\n", total, ack_count, nack_count, bounce_count);
	return true;
    }
    printf("total: %d ack: %d nack:%d rts: %d, NOT BAD\n", total, ack_count, nack_count, bounce_count);
    return false;
}

/* Process a return-to-sender packet */
void NdpSrc::processRTS(NdpPacket& pkt){
	//Yanfang	
	cout << " processRTS "<< pkt.seqno() <<" at "<< _name  << endl;
	int pkt_size = _mss;
	if(pkt.seqno() + _mss -1 >= _flow_size){
		pkt_size = _flow_size - pkt.seqno() +1;
	}
    assert(pkt.bounced());
    pkt.unbounce(ACKSIZE + pkt_size);
    
    _sent_times.erase(pkt.seqno());
    //resend from front of RTX
    //queue on any other path than the one we tried last time
    pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_CREATE);
    _rtx_queue.push_front(&pkt); 

    count_bounce(pkt.route()->path_id());

    /* When we get a return-to-sender packet, we could immediately
       resend it, but this leads to a larger-than-necessary second
       incast at the receiver.  So generally the best strategy is to
       swallow the RTS packet.  There are two exceptions: 1.  It's the
       only packet left, so the receiver doesn't even know we're
       trying to send.  2.  The packet was sent on a known-bad path.
       In this cases we immediately resend.  Comment out the #define
       below if you want to always resend immediately. */
#define SWALLOW
#ifdef SWALLOW
    if ((_pull_window == 0 && _first_window_count <= 1) || is_bad_path()) {
	//Only immediately resend if we're not expecting any more
	//pulls, or we believe this was a bad path.  Otherwise wait
	//for a pull.  Waiting reduces the effective window by one.
	send_packet(0);
	if (_log_me) {
	    printf("bounce send pw=%d\n", _pull_window);
	} else {
	    //printf("bounce send\n");
	}
    } else {
	if (_log_me) {
	    printf("bounce swallow pw=%d\n", _pull_window);
	} else {
	    //printf("bounce swallow\n");
	}
    }
#else
    send_packet(0);
    //printf("bounce send\n");
#endif
}

/* Process a NACK.  Generally this involves queuing the NACKed packet
   for retransmission, but then waiting for a PULL to actually resend
   it.  However, sometimes the NACK has the PULL bit set, and then we
   resend immediately */
void NdpSrc::processNack(const NdpNack& nack){
    NdpPacket* p;
/*
    if (nack.pull())
	printf("Receive NACK (pull)\n");
    else
	printf("Receive NACK (----)\n");
*/
    
    bool last_packet = (nack.ackno() + _mss - 1) >= _flow_size;
	int pkt_size = _mss;
	if(last_packet){
		pkt_size = _flow_size - nack.ackno() +1;
	}
    _sent_times.erase(nack.ackno());

    count_nack(nack.path_id());

    p = NdpPacket::newpkt(_flow, *_route, nack.ackno(), 0, pkt_size, true,
			  _paths.size()>0?_paths.size():1, last_packet);
    
    // need to add packet to rtx queue
    p->flow().logTraffic(*p,*this,TrafficLogger::PKT_CREATE);
    _rtx_queue.push_back(p);
	if(p->flow_id() == 1684730){
		cout << " processNack " << p->flow_id() << " " << nack.pull() << " " << nack.pullno() << " " << nack.pacerno() << endl;
	}
    if (nack.pull()) {
	_implicit_pulls++;
	pull_packets(nack.pullno(), nack.pacerno());
    }
}

/* Process an ACK.  Mostly just housekeeping, but if the ACK also has
   then PULL bit set, we also send a new packet immediately */
void NdpSrc::processAck(const NdpAck& ack) {
    NdpAck::seq_t ackno = ack.ackno();
    NdpAck::seq_t pacerno = ack.pacerno();
    NdpAck::seq_t pullno = ack.pullno();
    NdpAck::seq_t cum_ackno = ack.cumulative_ack();
	// cout << " processAck "<<eventlist().now() <<" ackno " << ackno << " cum_ackno "<< cum_ackno << endl;

    bool pull = ack.pull();
    if (pull) {
	if (_log_me)
	    cout << "PULLACK\n";
	_pull_window--;
    }
    simtime_picosec ts = ack.ts();
    int32_t path_id = ack.path_id();

    /*
      if (pull)
      printf("Receive ACK (pull): %s\n", ack.pull_bitmap().to_string().c_str());
      else
      printf("Receive ACK (----): %s\n", ack.pull_bitmap().to_string().c_str());
    */
    log_rtt(_first_sent_times[ackno]);
    _first_sent_times.erase(ackno);
    _sent_times.erase(ackno);

    count_ack(path_id);
  
    // Compute rtt.  This comes originally from TCP, and may not be optimal for NDP */
    uint64_t m = eventlist().now()-ts;

    if (m!=0){
	if (_rtt>0){
	    uint64_t abs;
	    if (m>_rtt)
		abs = m - _rtt;
	    else
		abs = _rtt - m;

	    _mdev = 3 * _mdev / 4 + abs/4;
	    _rtt = 7*_rtt/8 + m/8;

	    _rto = _rtt + 4*_mdev;
	} else {
	    _rtt = m;
	    _mdev = m/2;

	    _rto = _rtt + 4*_mdev;
	}
	if (_base_rtt==timeInf || _base_rtt > m)
	    _base_rtt = m;
    }

    if (_rto < _min_rto)
	_rto = _min_rto * ((drand() * 0.5) + 0.75);

    if (cum_ackno > _last_acked) { // a brand new ack    
	// we should probably cancel the rtx timer for any acked by
	// the cumulative ack, but we'll get an ACK or NACK anyway in
	// due course.
	_last_acked = cum_ackno;

    }
    if (_logger) _logger->logNdp(*this, NdpLogger::NDP_RCV);

	int pkt_size = _mss;
	if(ackno+_mss -1 >= _flow_size){
		pkt_size = _flow_size - ackno +1 ;
	}	 
	if(_flight_size < pkt_size){
		cout << "flow_id " <<ack.flow_id()  <<" _flight_size "<<  _flight_size << " pkt_size " << pkt_size <<" _flow_size " << _flow_size << " " << ackno<< endl;

	}
	assert(_flight_size >= pkt_size);
    _flight_size -= pkt_size;
	//yanfang: this assert would never happen, because _flight_size is an uint_32, it is always >=0;
	//Need fix
    assert(_flight_size>=0);

    if (cum_ackno >= _flow_size){
	cout << "Flow " << nodename() << " finished at " << timeAsMs(eventlist().now()) << endl;
    }

    update_rtx_time();

    /* if the PULL bit is set, send some new data packets */
    if (pull) {
	_implicit_pulls++;
	pull_packets(pullno, pacerno);
    }
}

void NdpSrc::receivePacket(Packet& pkt) 
{
    pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_RCVDESTROY);
    switch (pkt.type()) {
    case NDP:
	{
	    _bounces_received++;
	    _first_window_count--;
		if(pkt.flow_id() == 1684730)
		 	cout << " recv_packet "<<eventlist().now() <<" NDP " << " " << ((NdpPacket&)pkt).seqno() <<" " << pkt.flow_id() << endl;
	    processRTS((NdpPacket&)pkt);
	    return;
	}
    case NDPNACK: 
	{
	    _nacks_received++;
	    _pull_window++;
	    _first_window_count--;
	    /*if (_log_me) {
		printf("NACK, pw=%d\n", _pull_window);
	    } else {
		printf("NACK\n");
	    }*/
		if(pkt.flow_id() == 1684730)
			cout << " recv_packet "<<eventlist().now() <<" NDPNACK "  << " " << ((NdpNack&)pkt).ackno() <<" " << pkt.flow_id() << endl;
	    processNack((const NdpNack&)pkt);
	    pkt.free();
	    return;
	} 
    case NDPPULL: 
	{
	    _pulls_received++;
	    _pull_window--;
	    if (_log_me) {
		printf("PULL, pw=%d\n", _pull_window);
	    }
	    NdpPull *p = (NdpPull*)(&pkt);
	    NdpPull::seq_t cum_ackno = p->cumulative_ack();
	    if (cum_ackno > _last_acked) { // a brand new ack    
		// we should probably cancel the rtx timer for any acked by
		// the cumulative ack, but we'll get an ACK or NACK anyway in
		// due course.
		_last_acked = cum_ackno;
	  
	    }
	    //printf("Receive PULL: %s\n", p->pull_bitmap().to_string().c_str());
		if(pkt.flow_id() == 1684730)
			cout << " recv_packet "<<eventlist().now() <<" NDPPULL " << " cumackno " << cum_ackno <<" " << pkt.flow_id() <<" " << p->ackno() <<" " << p->pullno() << " " << p->pacerno()<< endl;
	    pull_packets(p->pullno(), p->pacerno());
	    return;
	}
    case NDPACK:
	{
	    _acks_received++;
	    _pull_window++;
	    _first_window_count--;
	    //	    if (_log_me) {
	    //	printf("ACK, pw=%d\n", _pull_window);
	    //}
		if(pkt.flow_id() == 1684730)
			cout << " recv_packet "<<eventlist().now() <<" NDPACK "<< " ackno " << ((NdpAck&)pkt).ackno() <<" ackum "<<((NdpAck&)pkt).cumulative_ack()  <<" " << pkt.flow_id() << endl;
	    processAck((const NdpAck&)pkt);
	    pkt.free();
	    return;
	}
	default:
	{
		cout << " flow_id " <<   pkt.flow_id() << " uknown " << endl;
	}
    }
}

/* Choose a route for a particular packet */
const Route* NdpSrc::choose_route() {
    const Route * rt;
    switch(_route_strategy) {
    case PULL_BASED:
    {
	/* this case is basically SCATTER_PERMUTE, but avoiding bad paths. */

	assert(_paths.size() > 0);
	if (_paths.size() == 1) {
	    // special case - no choice
	    rt = _original_paths[0];
	    return rt;
	}
	// otherwise we've got a choice
	_crt_path++;
	if (_crt_path == _paths.size()) {
	    permute_paths();
	    _crt_path = 0;
	}
	uint32_t path_id = _paths.at(_crt_path)->path_id();
	_avoid_score[path_id] = _avoid_ratio[path_id];
	int ctr = 0;
	while (_avoid_score[path_id] > 0 /* && ctr < 2*/) {
	    printf("as[%d]: %d\n", path_id, _avoid_score[path_id]);
	    _avoid_score[path_id]--;
	    ctr++;
	    //re-choosing path
	    cout << "re-choosing path " << path_id << endl;
	    _crt_path++;
	    if (_crt_path == _paths.size()) {
		permute_paths();
		_crt_path = 0;
	    }
	    path_id = _paths.at(_crt_path)->path_id();
	    _avoid_score[path_id] = _avoid_ratio[path_id];
	}
	//cout << "AS: " << _avoid_score[path_id] << " AR: " << _avoid_ratio[path_id] << endl;
	assert(_avoid_score[path_id] == 0);
	break;
    }
    case SCATTER_RANDOM:
	//ECMP
	assert(_paths.size() > 0);
	_crt_path = random()%_paths.size();
	break;
    case SCATTER_PERMUTE:
	//Cycle through a permutation.  Generally gets better load balancing than SCATTER_RANDOM.
	_crt_path++;
	assert(_paths.size() > 0);
	if (_crt_path == _paths.size()) {
	    permute_paths();
	    _crt_path = 0;
	}
	break;
    case SINGLE_PATH:
	abort();  //not sure if this can ever happen - if it can, remove this line
	return _route;
    case NOT_SET:
	abort();  // shouldn't be here at all
    }	

    rt = _paths.at(_crt_path);
    return rt;
}

void NdpSrc::pull_packets(NdpPull::seq_t pull_no, NdpPull::seq_t pacer_no) {
    // Pull number is cumulative both to allow for lost pulls and to
    // reduce reverse-path RTT - if one pull is delayed on one path, a
    // pull that gets there faster on another path can supercede it
	if(flow_id() == 1684730)
		cout << " last_pull "<<flow_id() <<" "<<_last_pull <<" pull_no " << pull_no <<" " << pacer_no<< endl;	
	// Here is a bug that I found, a message of two packets, the first packet gets dropped, the second packet arrives at the reciever, 
	//the cut-payloaded packet of the first packet arrives at the receiver first, the second packet arrives at the receiver later. 
	//the receiver generates an Nack for the first packet with pull no =2,
	//the receiver then generates an ACK for the second packet, and a PULL packet for the second packet with pull_no = 3;
	// the PULL packet arrives at the sender ealier than the NACK packet; 
	// void NdpSrc::pull_packets(NdpPull::seq_t pull_no, NdpPull::seq_t pacer_no)
	// The PULL packet would first increase the _last_pull to be 3, because there is not any packet in the retransmission queue, nothing happens.
	// the NACK packet would call pull_packets function, because _last_pull has already increased to 3, the NACK packet would not trigger a retransmission. 
	// At the end, only timeout can help this packet gets retransmitted. 
	if(pull_no !=0 && pull_no <= _last_pull){
		send_packet(pacer_no);
	}else{
		while (_last_pull < pull_no) {
		send_packet(pacer_no);
		_last_pull++;
		}
	}

	// while (_last_pull < pull_no) {
	// send_packet(pacer_no);
	// _last_pull++;
	// }
}

// Note: the data sequence number is the number of Byte1 of the packet, not the last byte.
void NdpSrc::send_packet(NdpPull::seq_t pacer_no) {
    NdpPacket* p;
	//Yanfang: when timeout happens, it retransmits the packet that is not in the _rtx_queue;
	// Add the following code to avoid twice retransmission;
	//Even the packet in the retransmission queue, the packet should 
	while(!_rtx_queue.empty()){
		p = _rtx_queue.front();
		int pkt_size = _mss;
		if (p->seqno() + _mss - 1 > _flow_size) {
			pkt_size = _flow_size - (p->seqno() - 1);
		}
		
		if((p->seqno()+pkt_size - 1) <= _last_acked){
			if(p->flow_id() == 1684730)
				cout << " rtx pop out send_packet "<<eventlist().now() <<" pkt_size " << pkt_size <<" _last_acked "<<_last_acked << " " << p->seqno() <<" " << p->flow_id() << endl;	
			_rtx_queue.pop_front();
			p->free();
		}else{
			break;
		}
	}
    if (!_rtx_queue.empty()) {
	// There are packets in the RTX queue for us to send

	p = _rtx_queue.front();
	_rtx_queue.pop_front();
	p->flow().logTraffic(*p,*this,TrafficLogger::PKT_SEND);
	p->set_ts(eventlist().now());
	p->set_pacerno(pacer_no);

	if (_route_strategy == SINGLE_PATH){
	    p->set_route(*_route);
	} else {
	    const Route *rt = choose_route();
	    p->set_route(*rt);
	    _path_counts_rtx[p->path_id()]++;
	}
	if(p->flow_id() == 1684730)
		cout << " rtx send_packet "<<eventlist().now() <<" pkt_size " << p->size() <<" _flight_size "<<_flight_size << " " << p->seqno() <<" " << p->flow_id() << endl;	
	PacketSink* sink = p->sendOn();
	PriorityQueue *q = dynamic_cast<PriorityQueue*>(sink);
	assert(q);
	//Figure out how long before the feeder queue sends this
	//packet, and add it to the sent time. Packets can spend quite
	//a bit of time in the feeder queue.  It would be better to
	//have the feeder queue update the sent time, because the
	//feeder queue isn't a FIFO but that would be hard to
	//implement in a real system, so this is a rough proxy.
	uint32_t service_time = q->serviceTime(*p);  
	_sent_times[p->seqno()] = eventlist().now() + service_time;
	_packets_sent ++;
	_rtx_packets_sent++;
	update_rtx_time();
	if (_rtx_timeout == timeInf) {
	    _rtx_timeout = eventlist().now() + _rto;
	}
    } else {
	// there are no packets in the RTX queue, so we'll send a new one
	bool last_packet = false;
	int pkt_size = _mss;
	if (_flow_size) {
	    if (_highest_sent >= _flow_size) {
		/* we've sent enough new data. */
		/* xxx should really make the last packet sent be the right size
		 * if _flow_size is not a multiple of _mss */
		return;
	    } 
	    if (_highest_sent + _mss >= _flow_size) {
		last_packet = true;
		pkt_size = _flow_size - _highest_sent;
	    }
	}
	switch (_route_strategy) {
	case SCATTER_PERMUTE:
	case SCATTER_RANDOM:
	case PULL_BASED:
	{
	    /*
	    if (_log_me) {
		cout << "Highest_sent: " << _highest_sent << " Flow_size: " << _flow_size << endl;
	    }
	    */
	    assert(_paths.size() > 0);
	    const Route *rt = choose_route();
	    p = NdpPacket::newpkt(_flow, *rt, _highest_sent+1, pacer_no, pkt_size, false,
				  _paths.size()>0?_paths.size():1, last_packet);
	    _path_counts_new[p->path_id()]++;
	    break;
	}
	case SINGLE_PATH:
	    p = NdpPacket::newpkt(_flow, *_route, _highest_sent+1, pacer_no,
				  pkt_size, false, 1,
				  last_packet);
	    break;
	case NOT_SET:
	    abort();
	}
	p->flow().logTraffic(*p,*this,TrafficLogger::PKT_CREATESEND);
	p->set_ts(eventlist().now());

	_flight_size += pkt_size;
	if(p->flow_id() == 1684730)
		cout << " send_packet "<<eventlist().now() <<" pkt_size " << pkt_size <<" _flight_size "<<_flight_size << " " << p->seqno() <<" " << p->flow_id() << endl;


	// 	if (_log_me) {
	// 	    cout << "Sent " << _highest_sent+1 << " FSz: " << _flight_size << endl;
	// 	}
	// if(p->seqno() == 1){
	// 	print_route(*(p->route()));
	// 	print_route(*(p->route()->reverse()));
	// }
	// cout << "_highest_sent "<< _highest_sent <<" " << p->route()->size() << endl;

	_highest_sent += pkt_size;  //XX beware wrapping
	_packets_sent++;
	_new_packets_sent++;

	PacketSink* sink = p->sendOn();
	PriorityQueue *q = dynamic_cast<PriorityQueue*>(sink);
	assert(q);
	//Figure out how long before the feeder queue sends this
	//packet, and add it to the sent time. Packets can spend quite
	//a bit of time in the feeder queue.  It would be better to
	//have the feeder queue update the sent time, because the
	//feeder queue isn't a FIFO but that would be hard to
	//implement in a real system, so this is a rough proxy.
	uint32_t service_time = q->serviceTime(*p);  
	//cout << "service_time2: " << service_time << endl;
	_sent_times[p->seqno()] = eventlist().now() + service_time;
	_first_sent_times[p->seqno()] = eventlist().now();

	if (_rtx_timeout == timeInf) {
	    _rtx_timeout = eventlist().now() + _rto;
	}
    }
}

void NdpSrc::permute_paths() {
    int len = _paths.size();
    for (int i = 0; i < len; i++) {
	int ix = random() % (len - i);
	const Route* tmppath = _paths[ix];
	_paths[ix] = _paths[len-1-i];
	_paths[len-1-i] = tmppath;
    }
}

void 
NdpSrc::update_rtx_time() {
    //simtime_picosec now = eventlist().now();
    if (_sent_times.empty()) {
	_rtx_timeout = timeInf;
	return;
    }
    map<NdpPacket::seq_t, simtime_picosec>::iterator i;
    simtime_picosec first_senttime = timeInf;
    int c = 0;
    for (i = _sent_times.begin(); i != _sent_times.end(); i++) {
	simtime_picosec sent = i->second;
	if (sent < first_senttime || first_senttime == timeInf) {
	    first_senttime = sent;
	}
	c++;
    }
    _rtx_timeout = first_senttime + _rto;
}
 
void 
NdpSrc::process_cumulative_ack(NdpPacket::seq_t cum_ackno) {
    map<NdpPacket::seq_t, simtime_picosec>::iterator i, i_next;
    i = _sent_times.begin();
    while (i != _sent_times.end()) {
	if (i->first <= cum_ackno) {
	    i_next = i; //juggling to keep i valid
	    i_next++;
	    _sent_times.erase(i);
	    i = i_next;
	} else {
	    return;
	}
    }
    //need to call update_rtx_time right after this!
}

void 
NdpSrc::retransmit_packet() {
    cout << "starting retransmit_packet\n";
    NdpPacket* p;
    map<NdpPacket::seq_t, simtime_picosec>::iterator i, i_next;
    i = _sent_times.begin();
    list <NdpPacket::seq_t> rtx_list;
    // we build a list first because otherwise we're adding to and
    // removing from _sent_times and the iterator gets confused
    while (i != _sent_times.end()) {
	if (i->second + _rto <= eventlist().now()) {
	    cout <<"flow_id "<< flow_id()<< " _sent_time: " << timeAsUs(i->second) << "us rto " << timeAsUs(_rto) << "us now " << timeAsUs(eventlist().now()) << "us, seqo " <<i->first <<"\n";
	    //this one is due for retransmission
	    rtx_list.push_back(i->first);
	    i_next = i; //we're about to invalidate i when we call erase
	    i_next++;
	    _sent_times.erase(i);
	    i = i_next;
	} else {
		cout <<" retransmit flow_id "<< flow_id()<< " _sent_time: " << timeAsUs(i->second) << "us rto " << timeAsUs(_rto) << "us now " << timeAsUs(eventlist().now()) << "us, seqo " <<i->first <<"\n";
	    i++;
	}
    }
    list <NdpPacket::seq_t>::iterator j;
    for (j = rtx_list.begin(); j != rtx_list.end(); j++) {
	NdpPacket::seq_t seqno = *j;
	bool last_packet = (seqno + _mss - 1) >= _flow_size;
	int pkt_size = _mss;
	if(last_packet){
		pkt_size = _flow_size - seqno + 1;
	}
	// if (p->flow_id() == 1684730){
		cout << "flow_id " << p->flow_id() << " pkt_size " << pkt_size << " seq_no " << seqno << endl;
	// }
	switch (_route_strategy) {
	case SCATTER_PERMUTE:
	case SCATTER_RANDOM:
	case PULL_BASED:
	{
	    assert(_paths.size() > 0);
	    const Route* rt = _paths.at(_crt_path);
	    p = NdpPacket::newpkt(_flow, *rt, seqno, 0, pkt_size, true,
				  _paths.size(), last_packet);
	    if (_route_strategy == SCATTER_RANDOM) {
		_crt_path = random() % _paths.size();
	    } else {
		_crt_path++;
		if (_crt_path==_paths.size()){ 
		    permute_paths();
		    _crt_path = 0;
		}
	    }
	    break;
	}
	case SINGLE_PATH:
	// Yanfang: the code does not tested for SINGLE path. 
	    p = NdpPacket::newpkt(_flow, *_route, seqno, 0, pkt_size, true,
				  1, last_packet); //_paths.size()
	    break;
	case NOT_SET:
	    abort();
	}
	
	p->flow().logTraffic(*p,*this,TrafficLogger::PKT_CREATESEND);
	p->set_ts(eventlist().now());
	//_sent_times[seqno] = eventlist().now();
	// 	if (_log_me) {
	//cout << "Sent " << seqno << " RTx" << " flow id " << p->flow().id << endl;
	// 	}
	_global_rto_count++;
	cout << "Total RTOs: " << _global_rto_count << endl;
	if(_route_strategy != SINGLE_PATH)
		_path_counts_rto[p->path_id()]++;
	p->sendOn();
	_packets_sent++;
	_rtx_packets_sent++;
    }
    update_rtx_time();
}

#define RESEND_ON_TIMEOUT

void NdpSrc::rtx_timer_hook(simtime_picosec now, simtime_picosec period) {
#ifndef RESEND_ON_TIMEOUT
    return;  // if we're using RTS, we shouldn't need to also use
	     // timeouts, at least in simulation where we don't see
	     // corrupted packets
#endif

    if (_highest_sent == 0) return;
    if (_rtx_timeout==timeInf || now + period < _rtx_timeout) return;

    cout <<"At " << timeAsUs(now) << "us RTO " << timeAsUs(_rto) << "us MDEV " << timeAsUs(_mdev) << "us RTT "<< timeAsUs(_rtt) << "us SEQ " << _last_acked / _mss << " CWND "<< _cwnd/_mss << " Flow ID " << str()  << endl;
    /*
    if (_log_me) {
	cout << "Flow " << LOGSINK << "scheduled for RTX\n";
    }
    */

    // here we can run into phase effects because the timer is checked
    // only periodically for ALL flows but if we keep the difference
    // between scanning time and real timeout time when restarting the
    // flows we should minimize them !
    if(!_rtx_timeout_pending) {
	_rtx_timeout_pending = true;

	
	// check the timer difference between the event and the real value
	simtime_picosec too_early = _rtx_timeout - now;
	if (now > _rtx_timeout) {
	    // this shouldn't happen
	    cout << "late_rtx_timeout: " << _rtx_timeout << " now: " << now << " now+rto: " << now + _rto << " rto: " << _rto << endl;
	    too_early = 0;
	}
	eventlist().sourceIsPendingRel(*this, too_early);
    }
}

void NdpSrc::log_rtt(simtime_picosec sent_time) {
    int64_t rtt = eventlist().now() - sent_time;
    if (rtt >= 0) 
	_rtt_hist[(int)timeAsUs(rtt)]++;
    else
	cout << "Negative RTT: " << rtt << endl;
}

void NdpSrc::doNextEvent() {
    if (_rtx_timeout_pending) {
	cout << "NdpSrc::doNextEvent " << flow_id() << endl;
	_rtx_timeout_pending = false;
    
	if (_logger) _logger->logNdp(*this, NdpLogger::NDP_TIMEOUT);

	retransmit_packet();
    } else {
	cout << "Starting flow" << endl;
	startflow();
    }
}

void NdpSrc::print_stats() {
    cout << _nodename << "\n";
    int total_new = 0, total_rtx = 0, total_rto = 0;
    for (int i = 0; i < _paths.size(); i++) {
	cout << _path_counts_new[i] << "/" << _path_counts_rtx[i] << "/" << _path_counts_rto[i] << " ";
	total_new += _path_counts_new[i];
	total_rtx += _path_counts_rtx[i];
	total_rto += _path_counts_rto[i];
    }
    cout << "\n";
    cout << "New: " << total_new << "  RTX: " << total_rtx << "  RTO " << total_rto << "\n";
}

////////////////////////////////////////////////////////////////
//  NDP SINK
////////////////////////////////////////////////////////////////

/* Only use this constructor when there is only one for to this receiver */
NdpSink::NdpSink(EventList& event, double pull_rate_modifier)
    : Logged("ndp_sink"),_cumulative_ack(0) , _total_received(0) 
{
    _src = 0;
    _pacer = new NdpPullPacer(event, pull_rate_modifier);
    //_pacer = new NdpPullPacer(event, "/Users/localadmin/poli/new-datacenter-protocol/data/1500.recv.cdf.pretty");
    
    _nodename = "ndpsink";
    _pull_no = 0;
    _last_packet_seqno = 0;
    _log_me = false;
    _total_received = 0;
    _path_hist_index = -1;
    _path_hist_first = -1;
#ifdef RECORD_PATH_LENS
    _path_lens.resize(MAX_PATH_LEN+1);
    _trimmed_path_lens.resize(MAX_PATH_LEN+1);
    for (int i = 0; i < MAX_PATH_LEN+1; i++) {
	_path_lens[i]=0;
	_trimmed_path_lens[i]=0;
    }
#endif
}

/* Use this constructor when there are multiple flows to one receiver
   - all the flows to one receiver need to share the same
   NdpPullPacer */
NdpSink::NdpSink(NdpPullPacer* pacer) : Logged("ndp_sink"),_cumulative_ack(0) , _total_received(0) 
{
    _src = 0;
    _pacer = pacer;
    _nodename = "ndpsink";
    _pull_no = 0;
    _last_packet_seqno = 0;
    _log_me = false;
    _total_received = 0;
    _path_hist_index = -1;
    _path_hist_first = -1;
#ifdef RECORD_PATH_LENS
    _path_lens.resize(MAX_PATH_LEN+1);
    _trimmed_path_lens.resize(MAX_PATH_LEN+1);
    for (int i = 0; i < MAX_PATH_LEN+1; i++) {
	_path_lens[i]=0;
	_trimmed_path_lens[i]=0;
    }
#endif
}

void NdpSink::log_me() {
    // avoid looping
    if (_log_me == true)
	return;

    _log_me = true;
    if (_src)
	_src->log_me();
    _pacer->log_me();
    
}

/* Connect a src to this sink.  We normally won't use this route if
   we're sending across multiple paths - call set_paths() after
   connect to configure the set of paths to be used. */
void NdpSink::connect(NdpSrc& src, Route& route)
{
    _src = &src;
    switch (_route_strategy) {
    case SINGLE_PATH:
	_route = &route;
	break;
    default:
	// do nothing we shouldn't be using this route - call
	// set_paths() to set routing information
	_route = NULL;
	break;
    }
	
    _cumulative_ack = 0;
    _drops = 0;

    // debugging hack
    if (get_id() == LOGSINK) {
	cout << "Found sink for " << LOGSINK << "\n";
	_log_me = true;
	_pacer->log_me();
    }
}

/* sets the set of paths to be used when sending from this NdpSink back to the NdpSrc */
void NdpSink::set_paths(vector<const Route*>* rt_list){
    switch (_route_strategy) {
    case SCATTER_PERMUTE:
    case SCATTER_RANDOM:
    case PULL_BASED:
	assert(_paths.size() == 0);
	_paths.resize(rt_list->size());
	for (unsigned int i=0;i<rt_list->size();i++){
	    Route* t = new Route(*(rt_list->at(i)));
	    t->add_endpoints(this, _src);
	    _paths[i]=t;
	}
	_crt_path = 0;
	permute_paths();
	break;
    case SINGLE_PATH:
    case NOT_SET:
	abort();
    }
}

// Receive a packet.
// Note: _cumulative_ack is the last byte we've ACKed.
// seqno is the first byte of the new packet.
void NdpSink::receivePacket(Packet& pkt) {
    NdpPacket *p = (NdpPacket*)(&pkt);
    NdpPacket::seq_t seqno = p->seqno();
    NdpPacket::seq_t pacer_no = p->pacerno();
    simtime_picosec ts = p->ts();
    bool last_packet = ((NdpPacket*)&pkt)->last_packet();
    switch (pkt.type()) {
    case NDP:
	/*
	if (_log_me) {
	    cout << "Recv " << seqno;
	    if (pkt.header_only()) 
		cout << " (hdr)";
	    else 
		cout << " (full)";
	    cout << endl;
	}
	*/
	break;
    case NDPACK:
    case NDPNACK:
    case NDPPULL:
	// Is there anything we should do here?  Generally won't happen unless the topolgy is very asymmetric.
	assert(pkt.bounced());
	cout << "Got bounced feedback packet!\n";
	p->free();
	return;
    }
	

    update_path_history(*p);
    if (pkt.header_only()){
	send_nack(ts,((NdpPacket*)&pkt)->seqno(), pacer_no);	 
	if(flow_id() == 1684730){
		cout << " 1684730 NdpSink::receivePacket nack " << ((NdpPacket*)&pkt)->seqno() << " "<< _cumulative_ack <<" pull_no " << _pull_no << endl;
	} 
	pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_RCVDESTROY);
#ifdef RECORD_PATH_LENS
	_trimmed_path_lens[pkt.path_len()]++;
#endif
	p->free();
	return;
    }

#ifdef RECORD_PATH_LENS
    _path_lens[pkt.path_len()]++;
#endif

    int size = p->size()-ACKSIZE; // Yanfang: fix the todo that the following code assumes all packets are the same size

    if (last_packet) {
	// we've seen the last packet of this flow, but may not have
	// seen all the preceding packets
	_last_packet_seqno = p->seqno() + size - 1;
    }

    pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_RCVDESTROY);
    p->free();
  
    _total_received+=size;

    if (seqno == _cumulative_ack+1) { // it's the next expected seq no
	_cumulative_ack = seqno + size - 1;
	// are there any additional received packets we can now ack?
	while (!_received.empty() && (_received.front().first == _cumulative_ack+1) ) {
	    _cumulative_ack+= _received.front().second;
		_received.pop_front();
		if(flow_id() == 1684730){
			cout << " 1684730 advance cumack " << seqno << " "<< _cumulative_ack << endl;
	}
	}
    } else if (seqno < _cumulative_ack+1) {
	//must have been a bad retransmit
    } else { // it's not the next expected sequence number
	if (_received.empty()) {
	    _received.push_front(make_pair(seqno, size));
		if(flow_id() == 1684730){
			cout << " 1684730 insert_received " << seqno << " "<< _cumulative_ack << endl;
	}
	    //it's a drop in this simulator there are no reorderings.
	    _drops += (size + seqno-_cumulative_ack-1)/size;
	} else if (seqno > _received.back().first) { // likely case
	    _received.push_back(make_pair(seqno, size));
		if(flow_id() == 1684730){
			cout << " 1684730 insert_received_larger " << seqno << " "<< _cumulative_ack <<" " <<_received.back().first << endl;
		}
	} 
	else { // uncommon case - it fills a hole
	    list<pair<uint64_t, int> >::iterator i;
	    for (i = _received.begin(); i != _received.end(); i++) {
		if (seqno == i->first) break; // it's a bad retransmit
		if (seqno < i->first) {
		    _received.insert(i, make_pair(seqno, size));
			if(flow_id() == 1684730){
				cout << " 1684730 insert_received_hole " << seqno << " "<< _cumulative_ack  << " " << i->first<< endl;
			}			
		    break;
		}
	    }
	}
    }

    send_ack(ts, seqno, pacer_no);
	if(flow_id() == 1684730){
		cout << " 1684730 NdpSink::receivePacket ack " << seqno << " "<< _cumulative_ack <<" pull_no " << _pull_no<<  endl;
	}
    // have we seen everything yet?
    if (_last_packet_seqno > 0 && _cumulative_ack == _last_packet_seqno) {
	_pacer->release_pulls(flow_id());
    }
}

/* _path_history was an experiment with allowing the receiver to tell
   the sender which path to use for the next data packet.  It's no
   longer used for that, but might still be useful for debugging */
void NdpSink::update_path_history(const NdpPacket& p) {
    assert(p.path_id() >= 0 && p.path_id() < 10000);
    if (_path_hist_index == -1) {
	//first received packet.
	_no_of_paths = p.no_of_paths();
	assert(_no_of_paths <= PULL_MAXPATHS); //ensure we've space in the pull bitfield
	_path_history.reserve(_no_of_paths * HISTORY_PER_PATH);
	_path_hist_index = 0;
	_path_hist_first = 0;
	_path_history[_path_hist_index] = ReceiptEvent(p.path_id(), p.header_only());
    } else {
		if(p.flow_id()== 1684730 ){
			cout << "_no_of_paths " << _no_of_paths << endl;
			cout << " p.no_of_paths() " << p.no_of_paths() << endl;
		}
	assert(_no_of_paths == p.no_of_paths());
	_path_hist_index = (_path_hist_index + 1) % _no_of_paths * HISTORY_PER_PATH;
	if (_path_hist_first == _path_hist_index) {
	    _path_hist_first = (_path_hist_first + 1) % _no_of_paths * HISTORY_PER_PATH;
	}
	_path_history[_path_hist_index] = ReceiptEvent(p.path_id(), p.header_only());
    }
}

void NdpSink::send_ack(simtime_picosec ts, NdpPacket::seq_t ackno, NdpPacket::seq_t pacer_no) {
    NdpAck *ack;
    _pull_no++;
    
    switch (_route_strategy) {
    case SCATTER_PERMUTE:
    case SCATTER_RANDOM:
    case PULL_BASED:
	assert(_paths.size() > 0);
	ack = NdpAck::newpkt(_src->_flow, *(_paths.at(_crt_path)), 0, ackno, 
			     _cumulative_ack, _pull_no, 
			     _path_history[_path_hist_index].path_id());
	if (_route_strategy == SCATTER_RANDOM) {
	    _crt_path = random()%_paths.size();
	} else {
	    _crt_path++;
	    if (_crt_path == _paths.size()) {
		permute_paths();
		_crt_path = 0;
	    }
	}
	break;
    case SINGLE_PATH:	
	ack = NdpAck::newpkt(_src->_flow, *_route, 0, ackno, _cumulative_ack,
			     _pull_no, _path_history[_path_hist_index].path_id());
	break;
    case NOT_SET:
	abort();
    }

    ack->flow().logTraffic(*ack,*this,TrafficLogger::PKT_CREATE);
    ack->set_ts(ts);

    _pacer->sendPacket(ack, pacer_no, this);
}

void NdpSink::send_nack(simtime_picosec ts, NdpPacket::seq_t ackno, NdpPacket::seq_t pacer_no) {
    NdpNack *nack;
    _pull_no++;
    switch (_route_strategy) {
    case SCATTER_PERMUTE:
    case SCATTER_RANDOM:
    case PULL_BASED:
	assert(_paths.size() > 0);
	nack = NdpNack::newpkt(_src->_flow, *(_paths.at(_crt_path)), 0, ackno, 
			       _cumulative_ack, _pull_no,
			       _path_history[_path_hist_index].path_id());
	if (_route_strategy == SCATTER_RANDOM) {
	    _crt_path = random()%_paths.size();
	} else {
	    _crt_path++;
	    if (_crt_path == _paths.size()) {
		permute_paths();
		_crt_path = 0;
	    }
	}
	break;
    case SINGLE_PATH:
	nack = NdpNack::newpkt(_src->_flow, *_route, 0, ackno, _cumulative_ack,
			       _pull_no, _path_history[_path_hist_index].path_id());
	break;
    case NOT_SET:
	abort();
    }
    nack->flow().logTraffic(*nack,*this,TrafficLogger::PKT_CREATE);
    nack->set_ts(ts);
    _pacer->sendPacket(nack, pacer_no, this);
}


void NdpSink::permute_paths() {
    int len = _paths.size();
    for (int i = 0; i < len; i++) {
	int ix = random() % (len - i);
	const Route* tmppath = _paths[ix];
	_paths[ix] = _paths[len-1-i];
	_paths[len-1-i] = tmppath;
    }
}


double* NdpPullPacer::_pull_spacing_cdf = NULL;
int NdpPullPacer::_pull_spacing_cdf_count = 0;


/* Every NdpSink needs an NdpPullPacer to pace out it's PULL packets.
   Multiple incoming flows at the same receiving node much share a
   single pacer */
NdpPullPacer::NdpPullPacer(EventList& event, double pull_rate_modifier, uint64_t bwMbps) : 
    EventSource(event, "ndp_pacer"), _last_pull(0)
{
    _packet_drain_time = (simtime_picosec)(Packet::data_packet_size() * (pow(10.0,12.0) * 8) / speedFromMbps((uint64_t)bwMbps))/pull_rate_modifier;
	cout << "NdpPullPacer _packet_drain_time  " << _packet_drain_time << " ps " << Packet::data_packet_size()*8.0*1000/(bwMbps/1000) << " ps" << endl;
    _log_me = false;
    _pacer_no = 0;
}


NdpPullPacer::NdpPullPacer(EventList& event, double pull_rate_modifier)  : 
    EventSource(event, "ndp_pacer"), _last_pull(0)
{
	//Yanfang: assume 10Gpbs receiver
    _packet_drain_time = (simtime_picosec)(Packet::data_packet_size() * (pow(10.0,12.0) * 8) / speedFromMbps((uint64_t)10000))/pull_rate_modifier;
	cout << "_packet_drain_time  " << _packet_drain_time << " ps " << Packet::data_packet_size()*8.0*1000/10 << endl;
    _log_me = false;
    _pacer_no = 0;
}

NdpPullPacer::NdpPullPacer(EventList& event, char* filename)  : 
    EventSource(event, "ndp_pacer"), _last_pull(0)
{
    int t;
    _packet_drain_time = 0;

    if (!_pull_spacing_cdf){
	FILE* f = fopen(filename,"r");
	fscanf(f,"%d\n",&_pull_spacing_cdf_count);
	cout << "Generating pull spacing from CDF; reading " << _pull_spacing_cdf_count << " entries from CDF file " << filename << endl;
	_pull_spacing_cdf = new double[_pull_spacing_cdf_count];

	for (int i=0;i<_pull_spacing_cdf_count;i++){
	    fscanf(f,"%d %lf\n",&t,&_pull_spacing_cdf[i]);
	    //assert(t==i);
	    //cout << " Pos " << i << " " << _pull_spacing_cdf[i]<<endl;
	}
    }
    
    _log_me = false;
    _pacer_no = 0;
}

void NdpPullPacer::log_me() {
    // avoid looping
    if (_log_me == true)
	return;

    _log_me = true;
    _total_excess = 0;
    _excess_count = 0;
}

void NdpPullPacer::set_pacerno(Packet *pkt, NdpPull::seq_t pacer_no) {
    if (pkt->type() == NDPACK) {
	((NdpAck*)pkt)->set_pacerno(pacer_no);
    } else if (pkt->type() == NDPNACK) {
	((NdpNack*)pkt)->set_pacerno(pacer_no);
    } else if (pkt->type() == NDPPULL) {
	((NdpPull*)pkt)->set_pacerno(pacer_no);
    } else {
	abort();
    }
}

void NdpPullPacer::sendPacket(Packet* ack, NdpPacket::seq_t rcvd_pacer_no, NdpSink* receiver) {
    /*
    if (_log_me) {
	cout << "pacerno diff: " << _pacer_no - rcvd_pacer_no << endl;
    }
    */

    if (rcvd_pacer_no != 0 && _pacer_no - rcvd_pacer_no < RCV_CWND) {
	// we need to increase the number of packets in flight from this flow
	if (_log_me)
	    cout << "increase_window\n";
	receiver->increase_window();
    }

    simtime_picosec drain_time;

    if (_packet_drain_time>0)
	drain_time = _packet_drain_time;
    else {
	int t = (int)(drand()*_pull_spacing_cdf_count);
	drain_time = 10*timeFromNs(_pull_spacing_cdf[t])/20;
	//cout << "Drain time is " << timeAsUs(drain_time);
    }
	    

    if (_pull_queue.empty()){
	simtime_picosec delta = eventlist().now()-_last_pull;
    
	if (delta >= drain_time){
	    //send out as long as last NACK/ACK was sent more than packetDrain time ago.
	    ack->flow().logTraffic(*ack,*this,TrafficLogger::PKT_SEND);
	    if (_log_me) {
		double excess = (delta - drain_time)/(double)drain_time;
		_total_excess += excess;
		_excess_count++;
		/*		cout << "Mean excess: " << _total_excess / _excess_count << endl;
		if (ack->type() == NDPACK) {
		    cout << "Ack " <<  (((NdpAck*)ack)->ackno()-1)/9000 << " excess " << excess << " (no queue)\n";
		} else if (ack->type() == NDPNACK) {
		    cout << "Nack " << (((NdpNack*)ack)->ackno()-1)/9000 << " excess " << excess << " (no queue)\n";
		} else {
		    cout << "WTF\n";
		    }*/
	    }
	    set_pacerno(ack, _pacer_no++);
		if(ack->flow_id() == 1684730){
			cout << "NdpPullPacer::sendPacket immediate  "<< eventlist().now() << " " <<  ((NdpAck*)ack)->ackno() <<" " <<((NdpAck*)ack)->cumulative_ack() 
				<<" " << ack->flow_id() <<" pull_no "<< ((NdpAck*)ack)->pullno() <<" pacer_no "<<((NdpAck*)ack)->pacerno() << "\n";
		}
	    ack->sendOn();
	    _last_pull = eventlist().now();
	    return;
	} else {
	    eventlist().sourceIsPendingRel(*this,drain_time - delta);
	}
    }

    if (_log_me) {
	_excess_count++;
	cout << "Mean excess: " << _total_excess / _excess_count << endl;
	if (ack->type() == NDPACK) {
	    cout << "Ack " <<  (((NdpAck*)ack)->ackno()-1)/9000 << " (queue)\n";
	} else if (ack->type() == NDPNACK) {
	    cout << "Nack " << (((NdpNack*)ack)->ackno()-1)/9000 << " (queue)\n";
	} else {
	    cout << "WTF\n";
	}
    }

    //Create a pull packet and stick it in the queue.
    //Send on the ack/nack, but with pull cleared.
    NdpPull *pull_pkt = NULL;
    if (ack->type() == NDPACK) {
	pull_pkt = NdpPull::newpkt((NdpAck*)ack);
	((NdpAck*)ack)->dont_pull();
    } else if (ack->type() == NDPNACK) {
	pull_pkt = NdpPull::newpkt((NdpNack*)ack);
	((NdpNack*)ack)->dont_pull();
    }

	if(ack->flow_id() == 1684730){
		cout << " pull_pkt " << pull_pkt->pullno() << " " ;
		if (ack->type() == NDPACK) {
			cout << "NdpPullPacer::sendPacket ack "<<eventlist().now() <<" "<< ack->flow_id() <<" " <<  ((NdpAck*)ack)->ackno() <<" " <<((NdpAck*)ack)->cumulative_ack()  << "\n";
		} else if (ack->type() == NDPNACK) {
			cout << "NdpPullPacer::sendPacket nack "<< eventlist().now() <<" "<< ack->flow_id()<< " "<<  ((NdpNack*)ack)->ackno() <<" " <<((NdpNack*)ack)->cumulative_ack()  << "\n";
		}
	}
    pull_pkt->flow().logTraffic(*pull_pkt,*this,TrafficLogger::PKT_CREATE);

    _pull_queue.enqueue(*pull_pkt);

    ack->flow().logTraffic(*ack,*this,TrafficLogger::PKT_SEND);
    ack->sendOn();
    
    //   if (_log_me) {
    //       list <Packet*>::iterator i = _waiting_pulls.begin();
    //       cout << "Queue: ";
    //       while (i != _waiting_pulls.end()) {
    // 	  Packet* p = *i;
    // 	  if (p->type() == NDPNACK) {
    // 	      cout << "Nack(" << ((NdpNack*)p)->ackno() << ") ";
    // 	  } else if (p->type() == NDPACK) {
    // 	      cout << "Ack(" << ((NdpAck*)p)->ackno() << ") ";
    // 	  } 
    // 	  i++;
    //       }
    //       cout << endl;
    //   }
    //cout << "Qsize = " << _waiting_pulls.size() << endl;
}


// when we're reached the last packet of a connection, we can release
// all the queued acks for that connection because we know they won't
// generate any more data packets.  This will move the nacks up the
// queue too, causing any retransmitted packets from the tail of the
// file to be received earlier
void NdpPullPacer::release_pulls(uint32_t flow_id) {
    _pull_queue.flush_flow(flow_id);
}


void NdpPullPacer::doNextEvent(){
    if (_pull_queue.empty()) {
	// this can happen if we released all the acks at the end of
	// the connection.  we didn't cancel the timer, so we end up
	// here.
	return;
    }


    Packet *pkt = _pull_queue.dequeue();


    //   cout << "Sending NACK for packet " << nack->ackno() << endl;
    pkt->flow().logTraffic(*pkt,*this,TrafficLogger::PKT_SEND);
    if (pkt->flow().log_me()) {
	if (pkt->type() == NDPACK) {
	    abort(); //we now only pace pulls
	} else if (pkt->type() == NDPNACK) {
	    abort(); //we now only pace pulls
	} if (pkt->type() == NDPPULL) {
	    //cout << "Pull (queued) " << ((NdpNack*)pkt)->ackno() << "\n";
	} else {
	    abort(); //we now only pace pulls
	}
    }
    set_pacerno(pkt, _pacer_no++);
	if(pkt->flow_id() == 1684730){
	// if(pkt->type() == NDPPULL)
		cout << "NdpPullPacer::doNextEvent  "<< eventlist().now() <<" flow_id " << pkt->flow_id()<< " " <<  ((NdpPull*)pkt)->ackno() 
			<<" " <<((NdpPull*)pkt)->cumulative_ack() <<" pull_no  "<< ((NdpNack*)pkt)->pullno() 	<<" pacer_no "<<((NdpAck*)pkt)->pacerno()<< "\n";
    }

    pkt->sendOn();   

    _last_pull = eventlist().now();
   
    simtime_picosec drain_time;

    if (_packet_drain_time>0)
	drain_time = _packet_drain_time;
    else {
	int t = (int)(drand()*_pull_spacing_cdf_count);
	drain_time = 10*timeFromNs(_pull_spacing_cdf[t])/20;
	//cout << "Drain time is " << timeAsUs(drain_time);
    }

    if (!_pull_queue.empty()){
	eventlist().sourceIsPendingRel(*this,drain_time);//*(0.5+drand()));
    }
    else {
	//    cout << "Empty pacer queue at " << timeAsMs(eventlist().now()) << endl; 
    }
}


////////////////////////////////////////////////////////////////
//  NDP RETRANSMISSION TIMER
////////////////////////////////////////////////////////////////

NdpRtxTimerScanner::NdpRtxTimerScanner(simtime_picosec scanPeriod, EventList& eventlist)
  : EventSource(eventlist,"RtxScanner"), 
    _scanPeriod(scanPeriod)
{
    eventlist.sourceIsPendingRel(*this, 0);
}

void 
NdpRtxTimerScanner::registerNdp(NdpSrc &tcpsrc)
{
    _tcps.push_back(&tcpsrc);
}

void
NdpRtxTimerScanner::doNextEvent() 
{
    simtime_picosec now = eventlist().now();
    tcps_t::iterator i;
    for (i = _tcps.begin(); i!=_tcps.end(); i++) {
	(*i)->rtx_timer_hook(now,_scanPeriod);
    }
    eventlist().sourceIsPendingRel(*this, _scanPeriod);
}
