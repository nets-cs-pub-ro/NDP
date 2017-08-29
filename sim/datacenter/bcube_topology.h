#ifndef BCUBE
#define BCUBE
#include "main.h"
#include "queue.h"
#include "pipe.h"
#include "config.h"
#include "loggers.h"
#include "network.h"
#include "firstfit.h"
#include "topology.h"
#include "logfile.h"
#include "eventlist.h"
#include "matrix.h"
#include <ostream>

//number of levels
//#define K 1
//number of ports per SWITCH
//#define NUM_PORTS 4
//total number of servers
//#define NUM_SRV 16
//switches per level
//=(K+1)*NUM_SRV/NUM_PORTS
//#define NUM_SW 8

#ifndef QT
#define QT
typedef enum {RANDOM, COMPOSITE, COMPOSITE_PRIO} queue_type;
#endif

class BCubeTopology: public Topology{
 public:
  Matrix3d<Pipe*> pipes_srv_switch;
  Matrix3d<Pipe*> pipes_switch_srv;
  Matrix3d<Queue*> queues_srv_switch;
  Matrix3d<Queue*> queues_switch_srv;
  Matrix2d<Queue*> prio_queues_srv;
  Matrix2d<unsigned int> addresses;

  FirstFit * ff;
  Logfile* logfile;
  EventList* eventlist;
  
  BCubeTopology(int no_of_nodes, int ports_per_switch, int no_of_levels, 
		Logfile* log,EventList* ev,FirstFit* f, queue_type q);

  void init_network();
  virtual vector<const Route*>* get_paths(int src, int dest);
 

  void count_queue(Queue*);
  void print_path(std::ofstream& paths,int src,const Route* route);
  void print_paths(std::ofstream& p, int src, vector<const Route*>* paths);
  vector<int>* get_neighbours(int src);
  int no_of_nodes() const {
    cout << "NoN: " << _NUM_SRV << "\n";
    return _NUM_SRV;
  }
 private:
  queue_type qt;
  map<Queue*,int> _link_usage;
  int _K, _NUM_PORTS, _NUM_SRV, _NUM_SW;
  void set_params(int no_of_nodes, int ports_per_switch, int no_of_levels);

  int srv_from_address(unsigned int* address);
  void address_from_srv(int srv);
  int get_neighbour(int src, int level);
  int switch_from_srv(int srv, int level);

  Route* bcube_routing(int src,int dest, int* permutation, int* nic);
  Route* dc_routing(int src,int dest, int i);  
  Route* alt_dc_routing(int src,int dest, int i,int c);  

  Queue* alloc_src_queue(QueueLogger* queueLogger);
  Queue* alloc_queue(QueueLogger* queueLogger);
  Queue* alloc_queue(QueueLogger* queueLogger, uint64_t speed);
};

#endif
