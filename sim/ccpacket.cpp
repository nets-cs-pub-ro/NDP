// -*- c-basic-offset: 4; indent-tabs-mode: nil -*- 
#include "ccpacket.h"

PacketDB<CCPacket> CCPacket::_packetdb;
PacketDB<CCAck> CCAck::_packetdb;
PacketDB<CCNack> CCNack::_packetdb;

