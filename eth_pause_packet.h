// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
#ifndef ETHPACKET_H
#define ETHPACKET_H

#include <list>
#include <bitset>
#include "network.h"

// ETHPAUSE is a subclass of Packet
// They incorporate a packet database, to reuse packet objects that are no longer needed.
// Note: you never construct a new EthPause packet directly; 
// rather you use the static method newpkt() which knows to reuse old packets from the database.

#define PAUSESIZE 64

class EthPausePacket : public Packet {
 public:
    //do not implement PFC; one priority class alone

    inline static EthPausePacket* newpkt(unsigned int sleep){
	EthPausePacket* p = _packetdb.allocPacket();
	p->_type = ETH_PAUSE;
	p->_sleepTime = sleep;
	p->_size = PAUSESIZE;
	return p;
    }
  
    void free() {_packetdb.freePacket(this);}
    virtual ~EthPausePacket(){}

    inline unsigned int sleepTime() const {return _sleepTime;}

 protected:
    unsigned int _sleepTime;
    static PacketDB<EthPausePacket> _packetdb;
};

#endif
