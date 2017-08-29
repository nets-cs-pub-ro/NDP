#ifndef FAT_TREE
#define FAT_TREE
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
#include "switch.h"
#include <ostream>

//#define N K*K*K/4

#define HOST_POD_SWITCH(src) (2*src/K)
//#define HOST_POD_ID(src) src%NSRV
#define HOST_POD(src) (src/NC)

#define MIN_POD_ID(pod_id) (pod_id*K/2)
#define MAX_POD_ID(pod_id) ((pod_id+1)*K/2-1)

#ifndef QT
#define QT
typedef enum {RANDOM, ECN, COMPOSITE, CTRL_PRIO, LOSSLESS, LOSSLESS_INPUT, LOSSLESS_INPUT_ECN} queue_type;
#endif

class FatTreeTopology: public Topology{
 public:
/*	
  Pipe * pipes_nc_nup[NC][NK];
  Pipe * pipes_nup_nlp[NK][NK];
  Pipe * pipes_nlp_ns[NK][NSRV];
  Queue * queues_nc_nup[NC][NK];
  Queue * queues_nup_nlp[NK][NK];
  Queue * queues_nlp_ns[NK][NSRV];

  Pipe * pipes_nup_nc[NK][NC];
  Pipe * pipes_nlp_nup[NK][NK];
  Pipe * pipes_ns_nlp[NSRV][NK];
  Queue * queues_nup_nc[NK][NC];
  Queue * queues_nlp_nup[NK][NK];
  Queue * queues_ns_nlp[NSRV][NK];
*/
  vector <Switch*> switches_lp;
  vector <Switch*> switches_up;
  vector <Switch*> switches_c;

  vector< vector<Pipe*> > pipes_nc_nup;
  vector< vector<Pipe*> > pipes_nup_nlp;
  vector< vector<Pipe*> > pipes_nlp_ns;
  vector< vector<Queue*> > queues_nc_nup;
  vector< vector<Queue*> > queues_nup_nlp;
  vector< vector<Queue*> > queues_nlp_ns;

  vector< vector<Pipe*> > pipes_nup_nc;
  vector< vector<Pipe*> > pipes_nlp_nup;
  vector< vector<Pipe*> > pipes_ns_nlp;
  vector< vector<Queue*> > queues_nup_nc;
  vector< vector<Queue*> > queues_nlp_nup;
  vector< vector<Queue*> > queues_ns_nlp;
  
  FirstFit* ff;
  Logfile* logfile;
  EventList* eventlist;
  int failed_links;
  queue_type qt;

  FatTreeTopology(int no_of_nodes, mem_b queuesize, Logfile* log,EventList* ev,FirstFit* f, queue_type q);
  FatTreeTopology(int no_of_nodes, mem_b queuesize, Logfile* log,EventList* ev,FirstFit* f, queue_type q, int fail);

  void init_network();
  virtual vector<const Route*>* get_paths(int src, int dest);

  Queue* alloc_src_queue(QueueLogger* q);
  Queue* alloc_queue(QueueLogger* q, mem_b queuesize);
  Queue* alloc_queue(QueueLogger* q, uint64_t speed, mem_b queuesize);

  void count_queue(Queue*);
  void print_path(std::ofstream& paths,int src,const Route* route);
  vector<int>* get_neighbours(int src) { return NULL;};
  int no_of_nodes() const {return _no_of_nodes;}
 private:
  map<Queue*,int> _link_usage;
  int find_lp_switch(Queue* queue);
  int find_up_switch(Queue* queue);
  int find_core_switch(Queue* queue);
  int find_destination(Queue* queue);
  void set_params(int no_of_nodes);
  int K, NK, NC, NSRV;
  int _no_of_nodes;
  mem_b _queuesize;
};

#endif
