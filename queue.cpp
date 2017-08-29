// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#include <sstream>
#include <math.h>
#include "queue.h"
#include "ndppacket.h"
#include "queue_lossless.h"

Queue::Queue(linkspeed_bps bitrate, mem_b maxsize, EventList& eventlist, 
	     QueueLogger* logger)
  : EventSource(eventlist,"queue"), 
    _maxsize(maxsize), _logger(logger), _bitrate(bitrate), _num_drops(0)
{
    _queuesize = 0;
    _ps_per_byte = (simtime_picosec)((pow(10.0, 12.0) * 8) / _bitrate);
    stringstream ss;
    ss << "queue(" << bitrate/1000000 << "Mb/s," << maxsize << "bytes)";
    _nodename = ss.str();
}


void
Queue::beginService()
{
    /* schedule the next dequeue event */
    assert(!_enqueued.empty());
    eventlist().sourceIsPendingRel(*this, drainTime(_enqueued.back()));
}

void
Queue::completeService()
{
    /* dequeue the packet */
    assert(!_enqueued.empty());
    Packet* pkt = _enqueued.back();
    _enqueued.pop_back();
    _queuesize -= pkt->size();
    pkt->flow().logTraffic(*pkt, *this, TrafficLogger::PKT_DEPART);
    if (_logger) _logger->logQueue(*this, QueueLogger::PKT_SERVICE, *pkt);

    /* tell the packet to move on to the next pipe */
    pkt->sendOn();

    if (!_enqueued.empty()) {
	/* schedule the next dequeue event */
	beginService();
    }
}

void
Queue::doNextEvent() 
{
    completeService();
}


void
Queue::receivePacket(Packet& pkt) 
{
    if (_queuesize+pkt.size() > _maxsize) {
	/* if the packet doesn't fit in the queue, drop it */
	if (_logger) 
	    _logger->logQueue(*this, QueueLogger::PKT_DROP, pkt);
	pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_DROP);
	pkt.free();
	_num_drops++;
	return;
    }
    pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_ARRIVE);

    /* enqueue the packet */
    bool queueWasEmpty = _enqueued.empty();
    _enqueued.push_front(&pkt);
    _queuesize += pkt.size();
    if (_logger) _logger->logQueue(*this, QueueLogger::PKT_ENQUEUE, pkt);

    if (queueWasEmpty) {
	/* schedule the dequeue event */
	assert(_enqueued.size() == 1);
	beginService();
    }
}

mem_b 
Queue::queuesize() {
    return _queuesize;
}

simtime_picosec
Queue::serviceTime() {
    return _queuesize * _ps_per_byte;
}


PriorityQueue::PriorityQueue(linkspeed_bps bitrate, mem_b maxsize, 
			     EventList& eventlist, QueueLogger* logger)
    : Queue(bitrate, maxsize, eventlist, logger) 
{
    _queuesize[Q_LO] = 0;
    _queuesize[Q_MID] = 0;
    _queuesize[Q_HI] = 0;
    _servicing = Q_NONE;
    _state_send = LosslessQueue::READY;
}

PriorityQueue::queue_priority_t 
PriorityQueue::getPriority(Packet& pkt) {
    queue_priority_t prio = Q_LO;
    switch (pkt.type()) {
    case TCPACK:
    case NDPACK:
    case NDPNACK:
    case NDPPULL:
    case NDPLITEACK:
    case NDPLITERTS:
    case NDPLITEPULL:
	prio = Q_HI;
	break;
    case NDP:
	if (pkt.header_only()) {
	    prio = Q_HI;
	} else {
	    NdpPacket* np = (NdpPacket*)(&pkt);
	    if (np->retransmitted()) {
		prio = Q_MID;
	    } else {
		prio = Q_LO;
	    }
	}
	break;
    case TCP:
    case IP:
    case NDPLITE:
	prio = Q_LO;
	break;
    default:
	abort();
    }
    return prio;
}

simtime_picosec
PriorityQueue::serviceTime(Packet& pkt) {
    queue_priority_t prio = getPriority(pkt);
    switch (prio) {
    case Q_LO:
	//cout << "q_lo: " << _queuesize[Q_HI] + _queuesize[Q_MID] + _queuesize[Q_LO] << " ";
	return (_queuesize[Q_HI] + _queuesize[Q_MID] + _queuesize[Q_LO]) * _ps_per_byte;
    case Q_MID:
	//cout << "q_mid: " << _queuesize[Q_MID] + _queuesize[Q_LO] << " ";
	return (_queuesize[Q_HI] + _queuesize[Q_MID]) * _ps_per_byte;
    case Q_HI:
	//cout << "q_hi: " << _queuesize[Q_LO] << " ";
	return _queuesize[Q_HI] * _ps_per_byte;
    default:
	abort();
    }
}

void
PriorityQueue::receivePacket(Packet& pkt) 
{
    //is this a PAUSE packet?
    if (pkt.type()==ETH_PAUSE){
	EthPausePacket* p = (EthPausePacket*)&pkt;

	if (p->sleepTime()>0){
	    //remote end is telling us to shut up.
	    //assert(_state_send == LosslessQueue::READY);
	    if (queuesize()>0)
		//we have a packet in flight
		_state_send = LosslessQueue::PAUSE_RECEIVED;
	    else
		_state_send = LosslessQueue::PAUSED;
	    
	    //cout << timeAsMs(eventlist().now()) << " " << _name << " PAUSED "<<endl;
	}
	else {
	    //we are allowed to send!
	    _state_send = LosslessQueue::READY;
	    //cout << timeAsMs(eventlist().now()) << " " << _name << " GO "<<endl;

	    //start transmission if we have packets to send!
	    if(queuesize()>0)
		beginService();
	}
	
	pkt.free();
	return;
    }

    queue_priority_t prio = getPriority(pkt);
    pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_ARRIVE);

    /* enqueue the packet */
    bool queueWasEmpty = false;
    if (queuesize() == 0)
	queueWasEmpty = true;

    _queuesize[prio] += pkt.size();
    _queue[prio].push_front(&pkt);

    if (_logger) _logger->logQueue(*this, QueueLogger::PKT_ENQUEUE, pkt);

    if (queueWasEmpty && _state_send==LosslessQueue::READY) {
	/* schedule the dequeue event */
	assert(_queue[Q_LO].size() + _queue[Q_MID].size() + _queue[Q_HI].size() == 1);
	beginService();
    }
}

void
PriorityQueue::beginService()
{
    assert(_state_send == LosslessQueue::READY);

    /* schedule the next dequeue event */
    for (int prio = Q_HI; prio >= Q_LO; --prio) {
	if (_queuesize[prio] > 0) {
	    eventlist().sourceIsPendingRel(*this, drainTime(_queue[prio].back()));
	    _servicing = (queue_priority_t)prio;
	    return;
	}
    }
}

void
PriorityQueue::completeService()
{
    /* dequeue the packet */
    assert(!_queue[_servicing].empty());
    assert(_servicing != Q_NONE);
    Packet* pkt = _queue[_servicing].back();
    _queue[_servicing].pop_back();
    _queuesize[_servicing] -= pkt->size();
    pkt->flow().logTraffic(*pkt, *this, TrafficLogger::PKT_DEPART);
    if (_logger) _logger->logQueue(*this, QueueLogger::PKT_SERVICE, *pkt);

    /* tell the packet to move on to the next pipe */
    pkt->sendOn();

    if (_state_send==LosslessQueue::PAUSE_RECEIVED)
	_state_send = LosslessQueue::PAUSED;

    //if (_state_send!=LosslessQueue::READY){
    //cout << eventlist().now() << " queue " << _name << " not ready but sending " << endl;
    //}

    if (queuesize() > 0) {
	if (_state_send==LosslessQueue::READY)
	    /* schedule the next dequeue event */
	    beginService();
	else {
	    //we've received pause or are already paused and will do nothing until the other end unblocks us
	}
    } else {
	_servicing = Q_NONE;
    }
}

mem_b
PriorityQueue::queuesize() {
    return _queuesize[Q_LO] + _queuesize[Q_MID] + _queuesize[Q_HI];
}
