// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#ifndef FAIRQUEUE_H
#define FAIRQUEUE_H

/*
 * A fair queue for pull packets
 */

#include <list>
#include "config.h"
#include "eventlist.h"
#include "network.h"
//#include "ndplitepacket.h"


template<class PullPkt>
class BasePullQueue {
 public:
    BasePullQueue();
    virtual ~BasePullQueue();
    virtual void enqueue(PullPkt& pkt) = 0;
    virtual PullPkt* dequeue() = 0;
    virtual void flush_flow(int32_t flow_id) = 0;
    virtual void set_preferred_flow(int32_t preferred_flow) {
	_preferred_flow = preferred_flow;
    }
    inline int32_t pull_count() const {return _pull_count;}
    inline bool empty() const {return _pull_count == 0;}
 protected:
    int32_t _pull_count;
    int32_t _preferred_flow;
};

template<class PullPkt>
class FifoPullQueue : public BasePullQueue<PullPkt>{
 public:
    FifoPullQueue();
    virtual void enqueue(PullPkt& pkt);
    virtual PullPkt* dequeue();
    virtual void flush_flow(int32_t flow_id);
 protected:
    list<PullPkt*> _pull_queue;
};

template<class PullPkt>
class FairPullQueue : public BasePullQueue<PullPkt>{
 public:
    FairPullQueue();
    virtual void enqueue(PullPkt& pkt);
    virtual PullPkt* dequeue();
    virtual void flush_flow(int32_t flow_id);
 protected:
    map<int32_t, list<PullPkt*>*> _queue_map;  // map flow id to pull queue
    bool queue_exists(const PullPkt& pkt);
    list<PullPkt*>* find_queue(const PullPkt& pkt);
    list<PullPkt*>* create_queue(const PullPkt& pkt);
    typename map<int32_t, list<PullPkt*>*>::iterator _current_queue;
};

#endif
