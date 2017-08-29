#ifndef VL2
#define VL2
#include "main.h"
#include "randomqueue.h"
#include "pipe.h"
#include "config.h"
#include "loggers.h"
#include "network.h"
#include "firstfit.h"
#include "topology.h"
#include "logfile.h"
#include "eventlist.h"

class VL2Topology: public Topology{
 public:
  Pipe * pipes_ni_na[NI][NA];
  Pipe * pipes_na_nt[NA][NT];
  Pipe * pipes_nt_ns[NT][NS];
  RandomQueue * queues_ni_na[NI][NA];
  RandomQueue * queues_na_nt[NA][NT];
  RandomQueue * queues_nt_ns[NT][NS];
  Pipe * pipes_na_ni[NA][NI];
  Pipe * pipes_nt_na[NT][NA];
  Pipe * pipes_ns_nt[NS][NT];
  RandomQueue * queues_na_ni[NA][NI];
  RandomQueue * queues_nt_na[NT][NA];
  RandomQueue * queues_ns_nt[NS][NT];

  FirstFit * ff;
  Logfile* logfile;
  EventList* eventlist;

  VL2Topology(Logfile* log,EventList* ev,FirstFit* f);

  void init_network();
  virtual vector<const Route*>* get_paths(int src, int dest);
  vector<int>* get_neighbours(int src) {return NULL;};
  int no_of_nodes() const {return _no_of_nodes;}
private:
  int _no_of_nodes;
};

#endif
