// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#include "fairpullqueue.h"
#include "ndppacket.h"

template<class PullPkt>
BasePullQueue<PullPkt>::BasePullQueue() : _pull_count(0), _preferred_flow(-1) {
}

template<class PullPkt>
BasePullQueue<PullPkt>::~BasePullQueue() {
}


template<class PullPkt>
FifoPullQueue<PullPkt>::FifoPullQueue() {
}

template<class PullPkt>
void
FifoPullQueue<PullPkt>::enqueue(PullPkt& pkt) {
    if (this->_preferred_flow>=0 && pkt.flow_id() == this->_preferred_flow){
	//cout << "Got a pkt from the preffered flow " << pull_pkt->flow_id()<<endl;
	typename list<PullPkt*>::iterator it = _pull_queue.begin();

	while (it!=_pull_queue.end()&& ((PullPkt*)(*it))->flow_id()==this->_preferred_flow)
	    it++;

	_pull_queue.insert(it, &pkt);
    } else {
	printf("Enqueue");
	_pull_queue.push_front(&pkt);
    }
    this->_pull_count++;
    assert(this->_pull_count == _pull_queue.size());
}

template<class PullPkt>
PullPkt*
FifoPullQueue<PullPkt>::dequeue() {
    if (this->_pull_count == 0) {
	printf("Dequeue empty");
	return 0;
    }
    printf("Dequeue");
    PullPkt* packet = _pull_queue.back();
    _pull_queue.pop_back();
    this->_pull_count--;
    assert(this->_pull_count == _pull_queue.size());
    return packet;
}

template<class PullPkt>
void
FifoPullQueue<PullPkt>::flush_flow(int32_t flow_id) {
    typename list<PullPkt*>::iterator it = _pull_queue.begin();
    while (it != _pull_queue.end()) {
	PullPkt* pull = *it;
	if (pull->flow_id() == flow_id && pull->type() == NDPPULL) {
	    pull->free();
	    it = _pull_queue.erase(it);
	    this->_pull_count--;
	} else {
	    it++;
	}
    }
    assert(this->_pull_count == _pull_queue.size());
}

template<class PullPkt>
FairPullQueue<PullPkt>::FairPullQueue() {
    _current_queue = _queue_map.begin();
}


template<class PullPkt>
void
FairPullQueue<PullPkt>::enqueue(PullPkt& pkt) {
    list <PullPkt*>* pull_queue;
    if (queue_exists(pkt)) {
	pull_queue = find_queue(pkt);
    }  else {
	pull_queue = create_queue(pkt);
    }
    //we add packets to the front,remove them from the back
    pull_queue->push_front(&pkt);
    this->_pull_count++;
}

template<class PullPkt>
PullPkt* 
FairPullQueue<PullPkt>::dequeue() {
    if (this->_pull_count == 0)
	return 0;
    while (1) {
	if (_current_queue == _queue_map.end())
	    _current_queue = _queue_map.begin();
	list <PullPkt*>* pull_queue = _current_queue->second;
	_current_queue++;
	if (!pull_queue->empty()) {
	    //we add packets to the front,remove them from the back
	    PullPkt* packet = pull_queue->back();
	    pull_queue->pop_back();
	    this->_pull_count--;
	    return packet;
	}
	// there are packets queued, so we'll eventually find a queue
	// that lets this terminate
    }
}

template<class PullPkt>
void
FairPullQueue<PullPkt>::flush_flow(int32_t flow_id) {
    typename map <int32_t, list<PullPkt*>*>::iterator i;
    i = _queue_map.find(flow_id);
    if (i == _queue_map.end())
	return;
    list<PullPkt*>* pull_queue = i->second;
    while (!pull_queue->empty()) {
	    PullPkt* packet = pull_queue->back();
	    pull_queue->pop_back();
	    packet->free();
	    this->_pull_count--;
    }
    if (_current_queue == i)
	_current_queue++;
    _queue_map.erase(i);
}

template<class PullPkt>
bool 
FairPullQueue<PullPkt>::queue_exists(const PullPkt& pkt) {
    typename map <int32_t, list<PullPkt*>*>::iterator i;
    i = _queue_map.find(pkt.flow_id());
    if (i == _queue_map.end())
	return false;
    return true;
}

template<class PullPkt>
list<PullPkt*>* 
FairPullQueue<PullPkt>::find_queue(const PullPkt& pkt) {
    typename map <int32_t, list<PullPkt*>*>::iterator i;
    i = _queue_map.find(pkt.flow_id());
    if (i == _queue_map.end())
	return 0;
    return i->second;
}

template<class PullPkt>
list<PullPkt*>* 
FairPullQueue<PullPkt>::create_queue(const PullPkt& pkt) {
    list<PullPkt*>* new_queue = new(list<PullPkt*>);
    _queue_map.insert(pair<int32_t, list<PullPkt*>*>(pkt.flow_id(), new_queue));
    return new_queue;
}

template class FifoPullQueue<NdpPull>;
template class FairPullQueue<NdpPull>;



