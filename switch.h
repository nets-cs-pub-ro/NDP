// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        

#ifndef _SWITCH_H
#define _SWITCH_H
#include "queue.h"
/*
 * A switch to group together multiple ports (currently used in the PAUSE implementation)
 */

#include <list>
#include "config.h"
#include "eventlist.h"
#include "network.h"
#include "loggertypes.h"

class LosslessQueue;
class LosslessInputQueue;

class Switch {
 public:
    Switch(){ _name = "none";};
    Switch(string s) { _name= s;}

    void addPort(Queue* q){
	_ports.push_back(q);
    }

    unsigned int portCount(){ return _ports.size();}

    void sendPause(LosslessQueue* problem, unsigned int wait);
    void sendPause(LosslessInputQueue* problem, unsigned int wait);

    void configureLossless();
    void configureLosslessInput();

    string _name;
 private:
    list<Queue*> _ports;
};
#endif
