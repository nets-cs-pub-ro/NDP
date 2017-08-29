// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#include "queue_lossless_input.h"
#include <math.h>
#include <iostream>
#include <sstream>
#include "switch.h"

LosslessInputQueue::LosslessInputQueue(EventList& eventlist)
    : Queue(0,Packet::data_packet_size()*20,eventlist,NULL),
      VirtualQueue(),
      _state_recv(READY)
{
    _high_threshold = Packet::data_packet_size()*15;
    _low_threshold = Packet::data_packet_size()*12;

    assert(_high_threshold>0);
    assert(_high_threshold > _low_threshold);

}

LosslessInputQueue::LosslessInputQueue(EventList& eventlist,Queue* peer)
    : Queue(0,Packet::data_packet_size()*20,eventlist,NULL),
      VirtualQueue(),
      _state_recv(READY)
{
    _high_threshold = Packet::data_packet_size()*15;
    _low_threshold = Packet::data_packet_size()*12;

    assert(_high_threshold>0);
    assert(_high_threshold > _low_threshold);

    stringstream ss;
    ss << "VirtualQueue("<< peer->_name<< ")";
    _nodename = ss.str();
    _remoteEndpoint = peer;

    peer->setRemoteEndpoint(this);
}

void
LosslessInputQueue::receivePacket(Packet& pkt) 
{
    /* normal packet, enqueue it */
    _queuesize += pkt.size();

    //send PAUSE notifications if that is the case!
    if (_queuesize > _high_threshold && _state_recv!=PAUSED){
	_state_recv = PAUSED;
	sendPause(1000);
    }

    //if (_state_recv==PAUSED)
    //cout << timeAsMs(eventlist().now()) << " queue " << _name << " switch (" << _switch->_name << ") "<< " recv when paused pkt " << pkt.type() << " sz " << _queuesize << endl;	

    if (_queuesize > _maxsize){
	cout << " Queue " << _name << " LOSSLESS not working! I should have dropped this packet" << endl;
    }

    //tell the output queue we're here!
    pkt.sendOn2(this);
}

void LosslessInputQueue::completedService(Packet& pkt){
    _queuesize -= pkt.size();

    //unblock if that is the case
    if (_queuesize < _low_threshold && _state_recv == PAUSED) {
	sendPause(0);
	_state_recv = READY;
    }
}

void LosslessInputQueue::sendPause(unsigned int wait){
    cout << "Ingress link " << getRemoteEndpoint()->_name << " PAUSE " << wait << endl;
    //cout << "Informing " << getRemoteEndpoint()->_name << endl;
    EthPausePacket* pkt = EthPausePacket::newpkt(wait);
    getRemoteEndpoint()->receivePacket(*pkt);
};

