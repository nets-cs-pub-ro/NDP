// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#ifndef _LOSSLESS_INPUT_QUEUE_H
#define _LOSSLESS_INPUT_QUEUE_H
#include "queue.h"
/*
 * A FIFO queue that supports PAUSE frames and lossless operation
 */

#include <list>
#include "config.h"
#include "eventlist.h"
#include "network.h"
#include "loggertypes.h"
#include "eth_pause_packet.h"

class Switch;

class LosslessInputQueue : public Queue, public VirtualQueue {
 public:
    LosslessInputQueue(EventList &eventlist);
    LosslessInputQueue(EventList &eventlist,Queue* peer);

    void receivePacket(Packet& pkt);

    void sendPause(unsigned int wait);
    void completedService(Packet& pkt);

    virtual void setName(const string& name) {
	Logged::setName(name); 
	_nodename += name;
    }
    virtual string& nodename() { return _nodename; }

    enum {PAUSED,READY,PAUSE_RECEIVED};

 private:
    int _state_recv;

    int _low_threshold;
    int _high_threshold;
};

#endif
