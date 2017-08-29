// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#include "pipe.h"
#include <iostream>
#include <sstream>

Pipe::Pipe(simtime_picosec delay, EventList& eventlist)
: EventSource(eventlist,"pipe"), _delay(delay)
{
    stringstream ss;
    ss << "pipe(" << delay/1000000 << "us)";
    _nodename= ss.str();
}

void
Pipe::receivePacket(Packet& pkt)
{
    pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_ARRIVE);
    if (_inflight.empty()){
	/* no packets currently inflight; need to notify the eventlist
	   we've an event pending */
	eventlist().sourceIsPendingRel(*this,_delay);
    }
    _inflight.push_front(make_pair(eventlist().now() + _delay, &pkt));
}

void
Pipe::doNextEvent() {
    if (_inflight.size() == 0) 
	return;

    Packet *pkt = _inflight.back().second;
    _inflight.pop_back();
    pkt->flow().logTraffic(*pkt, *this,TrafficLogger::PKT_DEPART);

    // tell the packet to move itself on to the next hop
    pkt->sendOn();

    if (!_inflight.empty()) {
	// notify the eventlist we've another event pending
	simtime_picosec nexteventtime = _inflight.back().first;
	_eventlist.sourceIsPending(*this, nexteventtime);
    }
}
