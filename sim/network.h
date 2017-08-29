// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#ifndef NETWORK_H
#define NETWORK_H

#include <vector>
#include <iostream>
#include "config.h"
#include "loggertypes.h"
#include "route.h"

class Packet;
class PacketFlow;
class PacketSink;
typedef uint32_t packetid_t;
void print_route(const Route& route);

class DataReceiver {
 public:
    DataReceiver(){};
    virtual ~DataReceiver(){};
    virtual uint64_t cumulative_ack()=0;
    virtual uint32_t get_id()=0;
    virtual uint32_t drops()=0;
};

class PacketFlow : public Logged {
    friend class Packet;
 public:
    PacketFlow(TrafficLogger* logger);
    virtual ~PacketFlow() {};
    void set_logger(TrafficLogger* logger);
    void logTraffic(Packet& pkt, Logged& location, TrafficLogger::TrafficEvent ev);
    inline uint32_t flow_id() const {return _flow_id;}
    bool log_me() const {return _logger != NULL;}
 protected:
    static uint32_t _max_flow_id;
    uint32_t _flow_id;
    TrafficLogger* _logger;
};


typedef enum {IP, TCP, TCPACK, TCPNACK, NDP, NDPACK, NDPNACK, NDPPULL, NDPLITE, NDPLITEACK, NDPLITEPULL, NDPLITERTS, ETH_PAUSE} packet_type;

class VirtualQueue {
 public:
    VirtualQueue() { }
    virtual ~VirtualQueue() {}
    virtual void completedService(Packet& pkt) =0;
};


// See tcppacket.h to illustrate how Packet is typically used.
class Packet {
    friend class PacketFlow;
 public:
    /* empty constructor; Packet::set must always be called as
       well. It's a separate method, for convenient reuse */
    Packet() {_is_header = false; _bounced = false; _type = IP; _flags = 0;}; 

    /* say "this packet is no longer wanted". (doesn't necessarily
       destroy it, so it can be reused) */
    virtual void free();

    static void set_packet_size(int packet_size) {
	// Use Packet::set_packet_size() to change the default packet
	// size for TCP or NDP data packets.  You MUST call this
	// before the value has been used to initialize anything else.
	// If someone has already read the value of packet size, no
	// longer allow it to be changed, or all hell will break
	// loose.
	assert(_packet_size_fixed == false);
	_data_packet_size = packet_size;
    }

    static int data_packet_size() {
	_packet_size_fixed = true;
	return _data_packet_size;
    }

    virtual PacketSink* sendOn(); // "go on to the next hop along your route"
                                  // returns what that hop is

    virtual PacketSink* sendOn2(VirtualQueue* crtSink);

    uint16_t size() const {return _size;}
    void set_size(int i) {_size = i;}
    int type() const {return _type;};
    bool header_only() const {return _is_header;}
    bool bounced() const {return _bounced;}
    PacketFlow& flow() const {return *_flow;}
    virtual ~Packet() {};
    inline const packetid_t id() const {return _id;}
    inline uint32_t flow_id() const {return _flow->flow_id();}
    const Route* route() const {return _route;}
    const Route* reverse_route() const {return _route->reverse();}

    virtual void strip_payload() { assert(!_is_header); _is_header = true;};
    virtual void bounce();
    virtual void unbounce(uint16_t pktsize);
    inline uint32_t path_len() const {return _path_len;}

    inline uint32_t flags() const {return _flags;}
    inline void set_flags(uint32_t f) {_flags = f;}

    uint32_t nexthop() const {return _nexthop;} // only intended to be used for debugging
    void set_route(const Route &route);
    string str() const;
 protected:
    void set_route(PacketFlow& flow, const Route &route, 
	     int pkt_size, packetid_t id);
    void set_attrs(PacketFlow& flow, int pkt_size, packetid_t id);

    static int _data_packet_size; // default size of a TCP or NDP data packet,
				  // measured in bytes
    static bool _packet_size_fixed; //prevent foot-shooting
    
    packet_type _type;
    
    uint16_t _size;
    bool _is_header;
    bool _bounced; // packet has hit a full queue, and is being bounced back to the sender
    uint32_t _flags; // used for ECN & friends

    // A packet can contain a route or a routegraph, but not both.
    // Eventually switch over entirely to RouteGraph?
    const Route* _route;
    uint32_t _nexthop;

    packetid_t _id;
    PacketFlow* _flow;
    uint32_t _path_len; // length of the path in hops - used in BCube priority routing with NDP
};

class PacketSink {
 public:
    PacketSink() { }
    virtual ~PacketSink() {}
    virtual void receivePacket(Packet& pkt) =0;
    virtual void receivePacket(Packet& pkt,VirtualQueue* previousHop) {
	receivePacket(pkt);
    };
    virtual const string& nodename()=0;
};


// For speed, it may be useful to keep a database of all packets that
// have been allocated -- that way we don't need a malloc for every
// new packet, we can just reuse old packets. Care, though -- the set()
// method will need to be invoked properly for each new/reused packet

template<class P>
class PacketDB {
 public:
    P* allocPacket() {
	if (_freelist.empty()) {
	    return new P();
	} else {
	    P* p = _freelist.back();
	    _freelist.pop_back();
	    return p;
	}
    };
    void freePacket(P* pkt) {
	_freelist.push_back(pkt);
    };

 protected:
    vector<P*> _freelist; // Irek says it's faster with vector than with list
};


#endif
