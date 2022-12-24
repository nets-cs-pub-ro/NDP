#ifndef LEAF_SPINE_H
#define LEAF_SPINE_H

#include <ostream>
#include <vector>

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

#ifndef QT
#define QT
typedef enum {
    RANDOM, ECN, COMPOSITE, CTRL_PRIO, LOSSLESS, LOSSLESS_INPUT,
    LOSSLESS_INPUT_ECN} queue_type;
#endif

// This is essentially a type-safe, floating point enum for all possible values
// of link rates. The only allowed instances of this class are the static values
// that are defined inside the class. Any other instance of this class will
// throw compiler error.
typedef
class LinkRateEnum {
  private:
    float value;
    LinkRateEnum(float arg) {value=arg;}

  public:
    static const LinkRateEnum ONE_G; //=1.0
    static const LinkRateEnum TWOFIVE;//=2.5
    static const LinkRateEnum TEN_G;//=10.0
    static const LinkRateEnum TWENTYFIVE_G;//=25.0
    static const LinkRateEnum FOURTY_G;//=40.0
    static const LinkRateEnum HUNDRED_G;//=100.0
    operator float() const {return value;}
} linkrates_t;

class LeafSpineTopology : public Topology
{
  public:
    vector <Switch*> tors;
    vector <Switch*> aggrs;
    vector <Switch*> cores;

    // in all of the pairs below, pair.first is upward link and pair.second
    // is downward link
    vector<pair<Pipe*, Pipe*>> pipes_srvr_tor;
    vector<pair<Pipe*, Pipe*>> pipes_tor_aggr;
    vector<pair<Pipe*, Pipe*>> pipes_aggr_core;

    // in all of the queues below, pair.first is upward queue and pair.second
    // is downward queue
    vector<pair<Queue*, Queue*>> queues_srvr_tor;
    vector<pair<Queue*, Queue*>> queues_tor_aggr;
    vector<pair<Queue*, Queue*>> queues_aggr_core;

    FirstFit* ff;
    Logfile* logfile;
    EventList* eventlist;
    queue_type qt;
    static linkrates_t nicspd;
    static linkrates_t aggrspd;
    static linkrates_t corespd;

    // link delays in micro seconds
    static double delayLeafUp;
    static double delayLeafDwn;
    static double delaySpineUp;
    static double delaySpineDwn;
    static double delayCoreUp;
    static double delayCoreDwn;

  private:
    uint32_t numPods, torsPerPod, srvrsPerTor, totalSrvrs;
    uint32_t numAggrs; // number of aggregation switches in each pod
    uint32_t numCores; // number of core switches
    mem_b queueSize;

  public:
    LeafSpineTopology(int srvrsPerTor, int torsPerPod, int numPods,
        mem_b queuesize, Logfile* log, EventList* ev, FirstFit* f,
        queue_type q);
    void init_network();
    virtual vector<const Route*>* get_paths(int src, int dest);
    void print_paths(std::ostream& p, int src, vector<const Route*>* paths);
    void print_path(std::ostream& paths, int src, const Route* route);
    vector<int>* get_neighbours(int src) {return NULL;}
    int no_of_nodes () const {return totalSrvrs;}

  private:
    void set_params();
    Queue* alloc_queue(QueueLogger* queueLogger, bool isSrcQ, uint64_t
        speedMbps, mem_b queuesize);
    int inline getTor(int srvrId) {return srvrId/srvrsPerTor;} // global tor id
    int inline getPod(int srvrId) {return srvrId/(srvrsPerTor*torsPerPod);}
    std::string getSrcStr(int src);
    std::string getTorStr(Queue* q, bool up);
    std::string getAggrStr(Queue* q, bool up);
    std::string getCoreStr(Queue* q);
    std::string getDestStr(Queue* q);
};

#endif //LEAF_SPINE_H
