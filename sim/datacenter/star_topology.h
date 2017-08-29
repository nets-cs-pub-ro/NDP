#ifndef STAR
#define STAR
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
#include <ostream>

#define NSRV 576

class StarTopology: public Topology{
 public:
  Pipe * pipe_out_ns[NSRV];
  RandomQueue * queue_out_ns[NSRV];

  Pipe * pipe_in_ns[NSRV];
  RandomQueue * queue_in_ns[NSRV];

  FirstFit * ff;
  Logfile* logfile;
  EventList* eventlist;
  
  StarTopology(Logfile* log,EventList* ev,FirstFit* f);

  void init_network();
  virtual vector<const Route*>* get_paths(int src, int dest);

  void count_queue(RandomQueue*);
  void print_path(std::ofstream& paths, int src, const Route* route);
  vector<int>* get_neighbours(int src);
  int no_of_nodes() const {return _no_of_nodes;}
 private:
  map<RandomQueue*,int> _link_usage;
  int _no_of_nodes;
};

#endif
