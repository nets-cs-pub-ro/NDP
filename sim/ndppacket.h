// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
#ifndef NDPPACKET_H
#define NDPPACKET_H

#include <list>
#include "network.h"

// NdpPacket and NdpAck are subclasses of Packet.
// They incorporate a packet database, to reuse packet objects that are no longer needed.
// Note: you never construct a new NdpPacket or NdpAck directly; 
// rather you use the static method newpkt() which knows to reuse old packets from the database.

#define ACKSIZE 64
#define VALUE_NOT_SET -1
#define PULL_MAXPATHS 256 // maximum path ID that can be pulled

class NdpPacket : public Packet {
 public:
    typedef uint64_t seq_t;

    // pseudo-constructor for a routeless packet - routing information
    // must be filled in later
    inline static NdpPacket* newpkt(PacketFlow &flow, 
				    seq_t seqno, seq_t pacerno, int size, 
				    bool retransmitted, 
				    bool last_packet) {
	NdpPacket* p = _packetdb.allocPacket();
	p->set_attrs(flow, size+ACKSIZE, seqno+size-1); // The NDP sequence number is the first byte of the packet; I will ID the packet by its last byte.
	p->_type = NDP;
	p->_is_header = false;
	p->_bounced = false;
	p->_seqno = seqno;
	p->_pacerno = pacerno;
	p->_retransmitted = retransmitted;
	p->_last_packet = last_packet;
	p->_path_len = 0;
	return p;
    }
  
    inline static NdpPacket* newpkt(PacketFlow &flow, const Route &route, 
				    seq_t seqno, seq_t pacerno, int size, 
				    bool retransmitted, int32_t no_of_paths,
				    bool last_packet) {
	NdpPacket* p = _packetdb.allocPacket();
	p->set_route(flow,route,size+ACKSIZE,seqno+size-1); // The NDP sequence number is the first byte of the packet; I will ID the packet by its last byte.
	p->_type = NDP;
	p->_is_header = false;
	p->_bounced = false;
	p->_seqno = seqno;
	p->_pacerno = pacerno;
	p->_retransmitted = retransmitted;
	p->_no_of_paths = no_of_paths;
	p->_last_packet = last_packet;
	p->_path_len = route.size();
	return p;
    }
  
    virtual inline void  strip_payload() {
	Packet::strip_payload(); _size = ACKSIZE;
    };
	
    void free() {_packetdb.freePacket(this);}
    virtual ~NdpPacket(){}
    inline seq_t seqno() const {return _seqno;}
    inline seq_t pacerno() const {return _pacerno;}
    inline void set_pacerno(seq_t pacerno) {_pacerno = pacerno;}
    inline bool retransmitted() const {return _retransmitted;}
    inline bool last_packet() const {return _last_packet;}
    inline simtime_picosec ts() const {return _ts;}
    inline void set_ts(simtime_picosec ts) {_ts = ts;}
    inline int32_t path_id() const {return _route->path_id();}
    inline int32_t no_of_paths() const {return _no_of_paths;}

 protected:
    seq_t _seqno;
    seq_t _pacerno;  // the pacer sequence number from the pull, seq space is common to all flows on that pacer
    simtime_picosec _ts;
    bool _retransmitted;
    int32_t _no_of_paths;  // how many paths are in the sender's
			    // list.  A real implementation would not
			    // send this in every packet, but this is
			    // simulation, and this is easiest to
			    // implement
    bool _last_packet;  // set to true in the last packet in a flow.
    static PacketDB<NdpPacket> _packetdb;
};

class NdpAck : public Packet {
 public:
    typedef NdpPacket::seq_t seq_t;
  
    inline static NdpAck* newpkt(PacketFlow &flow, const Route &route, 
				 seq_t pacerno, seq_t ackno, seq_t cumulative_ack,
				 seq_t pullno, int32_t path_id) {
	NdpAck* p = _packetdb.allocPacket();
	p->set_route(flow,route,ACKSIZE,ackno);
	p->_type = NDPACK;
	p->_is_header = true;
	p->_bounced = false;
	p->_pacerno = pacerno;
	p->_ackno = ackno;
	p->_cumulative_ack = cumulative_ack;
	p->_pull = true;
	p->_pullno = pullno;
	p->_path_id = path_id;
	p->_path_len = 0;
	return p;
    }
  
    void free() {_packetdb.freePacket(this);}
    inline seq_t pacerno() const {return _pacerno;}
    inline void set_pacerno(seq_t pacerno) {_pacerno = pacerno;}
    inline seq_t ackno() const {return _ackno;}
    inline seq_t cumulative_ack() const {return _cumulative_ack;}
    inline simtime_picosec ts() const {return _ts;}
    inline void set_ts(simtime_picosec ts) {_ts = ts;}
    inline bool pull() const {return _pull;}
    inline seq_t pullno() const {return _pullno;}
    int32_t  path_id() const {return _path_id;}
    inline void dont_pull() {_pull = false; _pullno = 0;}
  
    virtual ~NdpAck(){}

 protected:
    seq_t _pacerno;
    seq_t _ackno;
    seq_t _cumulative_ack;
    simtime_picosec _ts;
    bool _pull;
    seq_t _pullno;
    int32_t _path_id; //see comment in NdpPull
    static PacketDB<NdpAck> _packetdb;
};


class NdpNack : public Packet {
 public:
    typedef NdpPacket::seq_t seq_t;
  
    inline static NdpNack* newpkt(PacketFlow &flow, const Route &route, 
				  seq_t pacerno, seq_t ackno, seq_t cumulative_ack,
				  seq_t pullno, int32_t path_id) {
	NdpNack* p = _packetdb.allocPacket();
	p->set_route(flow,route,ACKSIZE,ackno);
	p->_type = NDPNACK;
	p->_is_header = true;
	p->_bounced = false;
	p->_pacerno = pacerno;
	p->_ackno = ackno;
	p->_cumulative_ack = cumulative_ack;
	p->_pull = true;
	p->_pullno = pullno;
	p->_path_id = path_id; // used to indicate which path the data
	                       // packet was trimmed on
	p->_path_len = 0;
	return p;
    }
  
    void free() {_packetdb.freePacket(this);}
    inline seq_t pacerno() const {return _pacerno;}
    inline void set_pacerno(seq_t pacerno) {_pacerno = pacerno;}
    inline seq_t ackno() const {return _ackno;}
    inline seq_t cumulative_ack() const {return _cumulative_ack;}
    inline simtime_picosec ts() const {return _ts;}
    inline void set_ts(simtime_picosec ts) {_ts = ts;}
    inline bool pull() const {return _pull;}
    inline seq_t pullno() const {return _pullno;}
    int32_t path_id() const {return _path_id;}
    inline void dont_pull() {_pull = false; _pullno = 0;}
  
    virtual ~NdpNack(){}

 protected:
    seq_t _pacerno;
    seq_t _ackno;
    seq_t _cumulative_ack;
    simtime_picosec _ts;
    bool _pull;
    seq_t _pullno;
    int32_t _path_id;
    static PacketDB<NdpNack> _packetdb;
};

class NdpPull : public Packet {
 public:
    typedef NdpPacket::seq_t seq_t;
    inline static NdpPull* newpkt(NdpAck* ack) {
	NdpPull* p = _packetdb.allocPacket();
	assert(ack->route());
	p->set_route(ack->flow(), *(ack->route()), ACKSIZE, ack->ackno());

	assert(p->route());
	p->_type = NDPPULL;
	p->_is_header = true;
	p->_bounced = false;
	p->_ackno = ack->ackno();
	p->_cumulative_ack = ack->cumulative_ack();
	p->_pullno = ack->pullno();
	p->_path_len = 0;
	return p;
    }
  
    inline static NdpPull* newpkt(NdpNack* nack) {
	NdpPull* p = _packetdb.allocPacket();
	assert(nack->route());
	p->set_route(nack->flow(), *(nack->route()), ACKSIZE, nack->ackno());

	assert(p->route());

	p->_type = NDPPULL;
	p->_is_header = true;
	p->_bounced = false;
	p->_ackno = nack->ackno();
	p->_cumulative_ack = nack->cumulative_ack();
	p->_pullno = nack->pullno();
	p->_path_len = 0;
	return p;
    }
  
    void free() {_packetdb.freePacket(this);}
    inline seq_t pacerno() const {return _pacerno;}
    inline void set_pacerno(seq_t pacerno) {_pacerno = pacerno;}
    inline seq_t ackno() const {return _ackno;}
    inline seq_t cumulative_ack() const {return _cumulative_ack;}
    inline seq_t pullno() const {return _pullno;}
    int32_t path_id() const {return _path_id;}
  
    virtual ~NdpPull(){}

 protected:
    seq_t _pacerno;
    seq_t _ackno;
    seq_t _cumulative_ack;
    seq_t _pullno;
    int32_t _path_id; // indicates ??
    static PacketDB<NdpPull> _packetdb;
};

#endif
