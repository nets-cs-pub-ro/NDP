#ifndef CBR_PACKET
#define CBR_PACKET

#include <list>
#include "network.h"

class CbrPacket : public Packet {
public:
  static PacketDB<CbrPacket> _packetdb;
  inline static CbrPacket* newpkt(PacketFlow &flow, route_t &route, int id, int size) {
    CbrPacket* p = _packetdb.allocPacket();
    p->set_route(flow,route,size,id);
    return p;
  }

  void free() {_packetdb.freePacket(this);}
  virtual ~CbrPacket(){};
};
#endif
