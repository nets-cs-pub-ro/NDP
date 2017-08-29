// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        

#include "eventlist.h"
//#include <iostream>

EventList::EventList()
    : _endtime(0),
      _lasteventtime(0)
{
}

void
EventList::setEndtime(simtime_picosec endtime)
{
    _endtime = endtime;
}

bool
EventList::doNextEvent() 
{
    if (_pendingsources.empty())
	return false;
    simtime_picosec nexteventtime = _pendingsources.begin()->first;
    EventSource* nextsource = _pendingsources.begin()->second;
    _pendingsources.erase(_pendingsources.begin());
    assert(nexteventtime >= _lasteventtime);
    _lasteventtime = nexteventtime; // set this before calling doNextEvent, so that this::now() is accurate
    nextsource->doNextEvent();
    return true;
}


void 
EventList::sourceIsPending(EventSource &src, simtime_picosec when) 
{
    /*
    pendingsources_t::iterator i = _pendingsources.begin();
    while (i != _pendingsources.end()) {
	if (i->second == &src)
	    abort();
	i++;
    }
    */
    
    assert(when>=now());
    if (_endtime==0 || when<_endtime)
	_pendingsources.insert(make_pair(when,&src));
}

void 
EventList::cancelPendingSource(EventSource &src) {
    pendingsources_t::iterator i = _pendingsources.begin();
    while (i != _pendingsources.end()) {
	if (i->second == &src) {
	    _pendingsources.erase(i);
	    return;
	}
	i++;
    }
}

void 
EventList::reschedulePendingSource(EventSource &src, simtime_picosec when) {
    cancelPendingSource(src);
    sourceIsPending(src, when);
}
