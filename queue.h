// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#ifndef QUEUE_H
#define QUEUE_H

/*
 * A simple FIFO queue
 */

#include <list>
#include "config.h"
#include "eventlist.h"
#include "network.h"
#include "loggertypes.h"

class Queue : public EventSource, public PacketSink {
 public:
    Queue(linkspeed_bps bitrate, mem_b maxsize, EventList &eventlist, 
	  QueueLogger* logger);
    virtual void receivePacket(Packet& pkt);
    void doNextEvent();
    // should really be private, but loggers want to see
    mem_b _maxsize; 

    inline simtime_picosec drainTime(Packet *pkt) { 
	return (simtime_picosec)(pkt->size() * _ps_per_byte); 
    }
    inline mem_b serviceCapacity(simtime_picosec t) { 
	return (mem_b)(timeAsSec(t) * (double)_bitrate); 
    }
    virtual mem_b queuesize();
    simtime_picosec serviceTime();
    int num_drops() const {return _num_drops;}
    void reset_drops() {_num_drops = 0;}

    virtual void setRemoteEndpoint(Queue* q) {_remoteEndpoint = q;};
    virtual void setRemoteEndpoint2(Queue* q) {_remoteEndpoint = q;q->setRemoteEndpoint(this);};
    Queue* getRemoteEndpoint() {return _remoteEndpoint;}

    virtual void setName(const string& name) {
	Logged::setName(name); 
	_nodename += name;
    }
    virtual void setLogger(QueueLogger* logger) {
	_logger = logger;
    }
    virtual const string& nodename() { return _nodename; }

 protected:
    // Housekeeping
    Queue* _remoteEndpoint;

    QueueLogger* _logger;

    // Mechanism
    // start serving the item at the head of the queue
    virtual void beginService(); 

    // wrap up serving the item at the head of the queue
    virtual void completeService(); 

    linkspeed_bps _bitrate; 
    simtime_picosec _ps_per_byte;  // service time, in picoseconds per byte
    mem_b _queuesize;
    list<Packet*> _enqueued;
    int _num_drops;
    string _nodename;
};

/* implement a 3-level priority queue */
class PriorityQueue : public Queue {
 public:
    typedef enum {Q_LO=0, Q_MID=1, Q_HI=2, Q_NONE=3} queue_priority_t;
    PriorityQueue(linkspeed_bps bitrate, mem_b maxsize, EventList &eventlist, 
		  QueueLogger* logger);
    virtual void receivePacket(Packet& pkt);
    virtual mem_b queuesize();
    simtime_picosec serviceTime(Packet& pkt);

 protected:
    //this is needed for lossless operation!
    

    // start serving the item at the head of the queue
    virtual void beginService(); 

    // wrap up serving the item at the head of the queue
    virtual void completeService(); 
    PriorityQueue::queue_priority_t getPriority(Packet& pkt);
    list <Packet*> _queue[Q_NONE];
    mem_b _queuesize[Q_NONE];
    queue_priority_t _servicing;
    int _state_send;
};

#endif
