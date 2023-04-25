// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-
#ifndef CCPACKET_H
#define CCPACKET_H

#include <list>
#include "network.h"

// CCPacket and CCAck are subclasses of Packet.
// They incorporate a packet database, to reuse packet objects that are no longer needed.
// Note: you never construct a new NdpPacket or NdpAck directly; 
// rather you use the static method newpkt() which knows to reuse old packets from the database.

#define ACKSIZE 64
#define VALUE_NOT_SET -1
//#define PULL_MAXPATHS 256 // maximum path ID that can be pulled

class CCPacket : public Packet {
public:
    typedef uint64_t seq_t;

    // pseudo-constructor for a routeless packet - routing information
    // must be filled in later
    inline static CCPacket* newpkt(const Route &route,PacketFlow &flow,seq_t seqno, int size,simtime_picosec ts) {
        CCPacket* p = _packetdb.allocPacket();
        p->set_attrs(flow, size+ACKSIZE, seqno+size-1);
        p->set_route(route);
        p->_type = CC;
        p->_is_header = false;
        p->_seqno = seqno;
        p->_ts = ts;
        return p;
    }
  
    inline seq_t seqno() const {return _seqno;}
    inline simtime_picosec ts() const {return _ts;}
    inline void set_ts(simtime_picosec ts) {_ts = ts;}

protected:
    seq_t _seqno;
    simtime_picosec _ts;

    //area to aggregate switch INT information
    static PacketDB<CCPacket> _packetdb;
};

class CCAck : public Packet {
public:
    typedef CCPacket::seq_t seq_t;
  
    inline static CCAck* newpkt(PacketFlow &flow, const Route &route, seq_t ackno, simtime_picosec ts) {
        CCAck* p = _packetdb.allocPacket();
        p->set_route(flow,route,ACKSIZE,ackno);
        p->_type = CCACK;
        p->_is_header = true;
        p->_ackno = ackno;
        p->_ts = ts;
        return p;
    }

    void free() {_packetdb.freePacket(this);}
    inline seq_t ackno() const {return _ackno;}
    inline simtime_picosec ts() const {return _ts;}
    inline void set_ts(simtime_picosec ts) {_ts = ts;}
    
    virtual ~CCAck(){}

protected:
    seq_t _ackno;
    simtime_picosec _ts;
    static PacketDB<CCAck> _packetdb;
};


class CCNack : public Packet {
public:
    typedef CCPacket::seq_t seq_t;
  
    inline static CCNack* newpkt(PacketFlow &flow, const Route &route, 
                                   seq_t ackno,simtime_picosec ts) {
        CCNack* p = _packetdb.allocPacket();
        p->set_route(flow,route,ACKSIZE,ackno);
        p->_type = CCNACK;
        p->_is_header = true;
        p->_ackno = ackno;
        p->_ts = ts;
        return p;
    }
  
    void free() {_packetdb.freePacket(this);}
    inline seq_t ackno() const {return _ackno;}
    inline simtime_picosec ts() const {return _ts;}
    inline void set_ts(simtime_picosec ts) {_ts = ts;}
  
    virtual ~CCNack(){}

protected:
    seq_t _ackno;
    simtime_picosec _ts;
    static PacketDB<CCNack> _packetdb;
};


#endif
