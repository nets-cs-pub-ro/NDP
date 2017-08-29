// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#ifndef ROUTE_H
#define ROUTE_H

/*
 * A Route, carried by packets, to allow routing
 */

#include "config.h"
#include <list>
#include <vector>

class PacketSink;
class Route {
  public:
    Route() : _reverse(NULL) {};
    inline PacketSink* at(size_t n) const {return _sinklist.at(n);}
    void push_back(PacketSink* sink) {_sinklist.push_back(sink);}
    void push_front(PacketSink* sink) {_sinklist.insert(_sinklist.begin(), sink);}
    void add_endpoints(PacketSink *src, PacketSink* dst);
    inline size_t size() const {return _sinklist.size();}
    typedef vector<PacketSink*>::const_iterator const_iterator;
    //typedef vector<PacketSink*>::iterator iterator;
    inline const_iterator begin() const {return _sinklist.begin();}
    inline const_iterator end() const {return _sinklist.end();}
    void set_reverse(Route* reverse) {_reverse = reverse;}
    inline const Route* reverse() const {return _reverse;}
    void set_path_id(int path_id, int no_of_paths) {
	_path_id = path_id;
	_no_of_paths = no_of_paths;
    }
    inline int path_id() const {return _path_id;}
    inline int no_of_paths() const {return _no_of_paths;}
 private:
    vector<PacketSink*> _sinklist;
    Route* _reverse;
    int _path_id; //path identifier for this path
    int _no_of_paths; //total number of paths sender is using
};
//typedef vector<PacketSink*> route_t;
typedef Route route_t;
typedef vector<route_t*> routes_t;

#endif
