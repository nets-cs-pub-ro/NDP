// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-  
#include <climits>
#include "route.h"
#include "network.h"
#include "queue.h"


#define MAXQUEUES 10

void
Route::add_endpoints(PacketSink *src, PacketSink* dst) {
    _sinklist.push_back(dst);
    if (_reverse) {
	_reverse->push_back(src);
    }
}


