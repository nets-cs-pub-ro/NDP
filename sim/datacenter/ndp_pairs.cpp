#include <iostream>
#include "ndp_pairs.h"
#include "math.h"

int msgNumber = 0;
uint64_t total_message_size = 0;
map<uint32_t, pair<uint64_t, uint64_t> > sender_tput;


uint64_t NdpSrcPart::inflightMesgs = 0;
int NdpSrcPart::lastLogTime = 0;
int NdpLoadGen::initConn = 30;
//int NdpLoadGen::initConn = 1;

// static int message_generated = 0;
NdpSrcPart::NdpSrcPart(NdpLogger* logger, TrafficLogger* pktLogger,
        EventList &eventlist, NdpLoadGen* owner)
    : NdpSrc(logger,pktLogger,eventlist)
    , mesgSize(0)
    , isActive(false)
    , started(eventlist.now())
    , loadGen(owner)
{
}

void
NdpSrcPart::setMesgSize(uint64_t mesgSize)
{
    this->mesgSize = mesgSize;
    set_flowsize(mesgSize);
}

void
NdpSrcPart::reset(uint64_t newMesgSize, bool shouldRestart)
{
    setMesgSize(newMesgSize);
    isActive = false;
    if (shouldRestart) {
        eventlist().sourceIsPending(*this, eventlist().now());
    }
}

void
NdpSrcPart::connect(route_t& routeout, route_t& routeback, NdpSink& sink,
    simtime_picosec starttime)
{
    reset(0, 0);
    NdpSrc::connect(routeout, routeback, sink, starttime);
}

void
NdpSrcPart::doNextEvent() {
    if (isActive) {
        if (_rtx_timeout_pending) {
            _rtx_timeout_pending = false;
            if (_logger) _logger->logNdp(*this, NdpLogger::NDP_TIMEOUT);
            retransmit_packet();
        }
    } else if (mesgSize) {
        NdpSrcPart::inflightMesgs++;
        isActive = true;
        ((NdpSinkPart*)_sink)->reset();

        started = eventlist().now();
        cout << "Started mesg of size: " << mesgSize << ", from ndp_src: "
            << str() << " at time: " << timeAsUs(started) <<"us." << endl;
        //cout << "startflow " << endl;
        startflow();
    }
}

void
NdpSrcPart::receivePacket(Packet& pkt){
    
	if (isActive){
        NdpSrc::receivePacket(pkt);
        if (_last_acked>=mesgSize){
        //   print_route(*(pkt.route()));
        //   cout << endl << "Mesg from src: " << str() <<" id "<< loadGen->src  << ", size: " <<  mesgSize
        //   << ", started at: " << timeAsUs(started) << "us, finished after: " <<
        //   timeAsUs(eventlist().now()-started) << "us." << " experience " <<(pkt.route()->size() -1) << " us"<< endl;
          //total bytes including headers
          uint64_t total_bytes_per_message = (mesgSize/_mss)*(_mss+ACKSIZE) + (mesgSize%_mss > 0 ? 1: 0)*(mesgSize%_mss + ACKSIZE);
          float ideal_fct = pkt.route()->size() -1 + total_bytes_per_message*8.0/100000;
          double fct = timeAsNs(eventlist().now() - started);
          float slowdown = timeAsUs(eventlist().now()-started)/ideal_fct;
          // sip, dip, sport, dport, size (B), start_time, fct (ns), standalone_fct (ns), qid, appid
          cout << loadGen->src <<" dip sport dport "<< mesgSize <<" "<< timeAsNs(started) <<" "<< fct <<" "<< ideal_fct<< " yle yle" <<endl;
          sender_tput[loadGen->src] = make_pair(sender_tput[loadGen->src].first + mesgSize, sender_tput[loadGen->src].second) ;
          cout << "yle: node "<< loadGen->src  <<" tput: " <<  sender_tput[loadGen->src].first*8.0 /(timeAsMs(eventlist().now()- sender_tput[loadGen->src].second*1000))/1000000 << "Gbps " << endl;
          total_message_size += mesgSize;
          cout<< "current_tput=" << total_message_size*8.0/(timeAsMs(eventlist().now()))/1000000 << "Gbps " << endl;
	  	  NdpSrcPart::inflightMesgs--;
          reset(0, 0);
        }
    }
    else {
      pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_RCVDESTROY);
      pkt.free();
    }

    double timeNow = timeAsMs(eventlist().now());
    if (int(timeNow/10) > NdpSrcPart::lastLogTime) {
        NdpSrcPart::lastLogTime = int(timeNow/10);
        cout << "#inflight messages: " <<  NdpSrcPart::inflightMesgs
        << ", time: " << NdpSrcPart::lastLogTime*10 << "ms" << endl;
    }
}

void
NdpSrcPart::rtx_timer_hook(simtime_picosec now, simtime_picosec period) {
  if (!isActive) {
      return;
  }

  if (now <= _rtx_timeout || _rtx_timeout==timeInf) {
      return;
  }

  if (_highest_sent == 0) {
      return;
  }
  cout << "Transfer timeout: active " << isActive << " mesg size " <<
    mesgSize << " sent " << _last_acked << endl;
  NdpSrc::rtx_timer_hook(now,period);
}

NdpSinkPart::NdpSinkPart(NdpRecvrAggr* recvrAggr)
    : NdpSink(recvrAggr->pacer)
    , recvrAggr(recvrAggr)
    , recvCmplt(false)
{}

void
NdpSinkPart::receivePacket(Packet& pkt)
{
	NdpSrcPart* srcPart = (NdpSrcPart*)_src;
    if (srcPart->isActive && !recvCmplt) {
        recvrAggr->markActive(this);
        switch (pkt.type()) {
            case NDP: {
                recvrAggr->dataBytesRecvd += pkt.size();
                recvrAggr->bytesRecvd += pkt.size();
                break;
            }
            case NDPACK:
            case NDPNACK:
            case NDPPULL:
                recvrAggr->bytesRecvd += pkt.size();
                break;
            default:
                cout << "unrecognized pkt recvd at " << str() << endl;
                break;
        }
    }

    // handle the packet in ndp mechanism
    NdpSink::receivePacket(pkt);

    // have received everything
    recvCmplt = (_last_packet_seqno > 0 &&
        _cumulative_ack == _last_packet_seqno);
    if (recvCmplt) {
        recvrAggr->markInactive(this);
    }
}

void
NdpSinkPart::reset() {
  _cumulative_ack = 0;
  _last_packet_seqno = 0;
  recvCmplt = false;
  _received.clear();
}


NdpLoadGen::NdpLoadGen(EventList& eventlist, int src,
        vector<NdpRecvrAggr*>* recvrAggrs, Topology* top, double lambda,
        int cwnd, vector<pair<uint64_t, double> > *wl, Logfile* logfile,
        NdpRtxTimerScanner* rtx, vector<const Route*>*** allRoutes, int is_incast)
    : EventSource(eventlist, "NdpLoadGen")
    , allNdpPairs(recvrAggrs->size(), NdpPairList())
    , src(src)
    , recvrAggrs(recvrAggrs)
    , topo(top)
    , lambda(lambda)
    , cwnd(cwnd)
    , wl(wl)
    , logfile(logfile)
    , ndpRtxScanner(rtx)
    , allRoutes(allRoutes)
	, is_incast(is_incast)
{
    eventlist.sourceIsPendingRel(*this, timeFromMs(10));
}

NdpLoadGen::NdpLoadGen(EventList& eventlist, int src,
        vector<NdpRecvrAggr*>* recvrAggrs, Topology* top, 
        int cwnd, Logfile* logfile,
        NdpRtxTimerScanner* rtx, vector<const Route*>*** allRoutes)
    : EventSource(eventlist, "NdpLoadGen")
    , allNdpPairs(recvrAggrs->size(), NdpPairList())
    , src(src)
    , recvrAggrs(recvrAggrs)
    , topo(top)
    , cwnd(cwnd)
    , logfile(logfile)
    , ndpRtxScanner(rtx)
    , allRoutes(allRoutes)
{
    // cout << " NdpLoadGen " << src << endl;
    //eventlist.sourceIsPendingRel(*this, timeFromMs(10));
}

uint64_t
NdpLoadGen::generateMesgSize()
{
    double prob = drand();
    size_t mid, high, low;
    high = wl->size() - 1;
    low = 0;
    while(low < high) {
        mid = (high + low) / 2;
        if (prob <= wl->at(mid).second) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    uint64_t msgSize = wl->at(high).first;
	return msgSize;
}

void
NdpLoadGen::doNextEvent()
{
  /*if (msgNumber > 10000) {
    return;
  }*/
  run();
  msgNumber++;
  simtime_picosec nextArrival = timeFromSec(exponential(lambda));
  cout << "next arrival time(ms): " << nextArrival/1e9 << endl;
  eventlist().sourceIsPendingRel(*this, nextArrival);
}

NdpLoadGen::NdpPair&
NdpLoadGen::createConnection(int dest)
{
    cout << "createConnection called at time " << timeAsUs(eventlist().now()) <<"us." << endl;
    NdpPairList& pairList = allNdpPairs[dest];
    if (!allRoutes[src][dest]) {
        vector<const Route*>* paths = topo->get_paths(src, dest);
        allRoutes[src][dest] = paths;
        paths = topo->get_paths(dest, src);
        allRoutes[dest][src] = paths;
    }

    do {
        NdpSrcPart* ndpSrc = new NdpSrcPart(NULL, NULL, eventlist(), this);
        NdpSinkPart* ndpSnk = new NdpSinkPart(recvrAggrs->at(dest));
        ndpSrc->setCwnd(cwnd*Packet::data_packet_size());
        ndpSrc->setName("ndp_" + ntoa(src) + "_" + ntoa(dest)+
            "("+ntoa(pairList.size())+")");
        logfile->writeName(*ndpSrc);

        ndpSnk->setName("ndp_sink_" + ntoa(src) + "_" + ntoa(dest)+
            "("+ntoa(pairList.size())+")");
        logfile->writeName(*ndpSnk);

        ndpRtxScanner->registerNdp(*ndpSrc);

        size_t choice = rand() % allRoutes[src][dest]->size();
        Route* routeout = new Route(*(allRoutes[src][dest]->at(choice)));
        routeout->push_back(ndpSnk);

        choice = rand() % allRoutes[dest][src]->size();
        Route* routein = new Route(*(allRoutes[dest][src]->at(choice)));
        routein->push_back(ndpSrc);

        ndpSrc->connect(*routeout, *routein, *ndpSnk, eventlist().now());
        ndpSrc->set_paths(allRoutes[src][dest]);
        ndpSnk->set_paths(allRoutes[dest][src]);
        pairList.push_back(make_pair(ndpSrc, ndpSnk));
    } while (pairList.size() < initConn);
    return allNdpPairs[dest].back();
}

void
NdpLoadGen::run() {
    //randomly choose a destination and activate a connection to it.
    int dest = src;
    do {
        dest = rand() % recvrAggrs->size();
    } while (src == dest);

	if(is_incast == 1){
		dest = 0;
	}
    //look for inactive connection
    NdpPairList& ndpPairs = allNdpPairs[dest];
    NdpPair ndpPair;
    NdpPairList::iterator it;
    for (it=ndpPairs.begin(); it != ndpPairs.end(); it++) {
        ndpPair =  *it;
        if (!ndpPair.first->isActive) {
            break;
        }
    }

    if (it != ndpPairs.end()) {
        ndpPairs.erase(it);
        ndpPairs.push_back(ndpPair);
    } else {
        ndpPair = createConnection(dest);
    }
    ndpPair.first->reset(generateMesgSize(), true);
}

string
NdpLoadGen::ntoa(double n) {
    stringstream s;
    s << n;
    return s.str();
}

string
NdpLoadGen::itoa(uint64_t n) {
    stringstream s;
    s << n;
    return s.str();
}

void
NdpLoadGen::printAllActive()
{
    stringstream outstr;
    for (auto& pairList : allNdpPairs) {
        for (auto& pair : pairList) {
            if (pair.first->isActive) {
                outstr << pair.first << ", name: " <<  pair.first->str() <<
                ", flight_size: " << pair.first->_flight_size <<
                ", highest_sent: " << pair.first->_highest_sent <<
                ", last_ack: " << pair.first->_last_acked << "\n";
            }
        }
    }
    cout << outstr.str() << endl;
}

//void
//NdpLoadGen::printTopoPaths(vector<const Route*>* paths, int src)
//{
//    for (size_t i = 0; i < paths->size(); i++) {
//        topo->print_path(std::cout, src, paths->at(i));
//    }
//}

NdpReadTrace::NdpReadTrace(EventList& eventlist, int src,
        vector<NdpRecvrAggr*>* recvrAggrs, Topology* top,
        int cwnd, Logfile* logfile,
        NdpRtxTimerScanner* rtx, vector<const Route*>*** allRoutes, string flow_file)
    : NdpLoadGen(eventlist, src, recvrAggrs, top, cwnd, logfile, rtx, allRoutes)
    , flowfile(flow_file)
{
    flowf.open(flow_file.c_str());
    flow_idx = 0;
    flowf >> flow_num;
    // cout << " flow file: " << flow_file.c_str() << " flow_num: " << flow_num << endl;
    eventlist.sourceIsPendingRel(*this, timeFromMs(0));
}

void
NdpReadTrace::doNextEvent()
{
  if (msgNumber > flow_num) {
    return;
  }
  run();

  
}

void
NdpReadTrace::run()
{
    uint32_t dest, pg, dport, messageSize;
    int source = -1;
    uint64_t start_time_ns;
    while(source != src && flow_idx < flow_num){
        flowf >> source >> dest >> pg >> dport >> messageSize >> start_time_ns;
        flow_idx ++;
    }
    if(flow_idx == flow_num && source != src){
        // cout<< " src node " << src << " msgNumber " << msgNumber <<endl;
        return;
    }
    msgNumber++;
    if (sender_tput.find(src) == sender_tput.end()){
        sender_tput[src] = make_pair(0, timeFromNs(start_time_ns));
    }
    simtime_picosec nextArrival =  timeFromNs(start_time_ns) - eventlist().now();
    cout <<"src "<< src <<" source "<< source << " dest " << dest 
        << " msg " << messageSize << " start_time (us) " << start_time_ns 
        <<" nextArrival " << nextArrival
        <<endl;
    //look for inactive connection
    NdpPairList& ndpPairs = allNdpPairs[dest];
    NdpPair ndpPair;
    NdpPairList::iterator it;
    for (it=ndpPairs.begin(); it != ndpPairs.end(); it++) {
        ndpPair =  *it;
        if (!ndpPair.first->isActive) {
            break;
        }
    }

    if (it != ndpPairs.end()) {
        ndpPairs.erase(it);
        ndpPairs.push_back(ndpPair);
    } else {
        ndpPair = createConnection(dest);
    }
    ndpPair.first->reset(messageSize, true);
    if(timeFromNs(start_time_ns) < eventlist().now()){
        cerr << "The next message should start "<< start_time_ns << "ns, but read at " << eventlist().now()/1000
            << "ns \n";
        exit(1);
    }
    
    eventlist().sourceIsPendingRel(*this, nextArrival); 
}

NdpRecvrAggr::NdpRecvrAggr(EventList& eventlist, int dest, NdpPullPacer* pacer)
    : EventSource(eventlist, "NdpRecvrAggr")
    , node(dest)
    , pacer(pacer)
    , activeSinkNodes()
    , inActivePeriod(false)
    , activeStart(0)
    , activeEnd(0)
    , totalActiveTime(0)
    , startTime(0)
    , dataBytesRecvd(0)
    , bytesRecvd(0)
{}

void
NdpRecvrAggr::markActive(NdpSinkPart* sink)
{
    if (startTime == 0)
        startTime = eventlist().now();

    if (!inActivePeriod) {
        assert(activeSinkNodes.empty());
        inActivePeriod = true;
        activeStart = eventlist().now();
    }
    activeSinkNodes.insert(sink);
}

void
NdpRecvrAggr::markInactive(NdpSinkPart* sink)
{
    activeSinkNodes.erase(sink);
    if (activeSinkNodes.empty()) {
        inActivePeriod = false;
        activeEnd = eventlist().now();
        totalActiveTime += (activeEnd - activeStart);
    }
}

NdpRecvrAggr::~NdpRecvrAggr()
{
    if (inActivePeriod) {
        activeEnd = eventlist().now();
        totalActiveTime += (activeEnd - activeStart);
    }
    cout << "RecvrId: " << node << ", active time: " << totalActiveTime <<
        ", total time: " << eventlist().now() - startTime <<
        ", active total bytes: " << bytesRecvd << ", active data bytes: " <<
        dataBytesRecvd << endl;
}
