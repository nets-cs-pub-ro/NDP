#ifndef connection_matrix
#define connection_matrix

#include "main.h"
#include "tcp.h"
#include "topology.h"
#include "randomqueue.h"
#include "eventlist.h"
#include <list>
#include <map>

struct connection{
  int src, dst;
};

class ConnectionMatrix{
 public:
  ConnectionMatrix(int );
  void addConnection(int src, int dest);
  void setPermutation(int conn);
  void setPermutation(int conn, int rack_size);
  void setPermutation();
  void setRandom(int conns);
  void setStride(int many);
  void setLocalTraffic(Topology* top);
  void setStaggeredRandom(Topology* top,int conns,double local);
  void setStaggeredPermutation(Topology* top,double local);
  void setVL2();
  void setHotspot(int hosts_per_spot,int count);
  void setIncast(int hosts_per_hotspot, int center);
  void setOutcast(int hosts_per_hotspot, int center);
  void setManytoMany(int hosts);

  vector<connection*>* getAllConnections();

  map<int,vector<int>*> connections;
  int N;
};

#endif
