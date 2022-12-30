#include "leafspine_topology.h"
#include "string.h"
#include <sstream>
#include <iostream>
#include "queue.h"
#include "switch.h"
#include "compositequeue.h"
#include "prioqueue.h"
#include "queue_lossless.h"
#include "queue_lossless_input.h"
#include "queue_lossless_output.h"
#include "ecnqueue.h"

const LinkRateEnum LinkRateEnum::ONE_G(1.0f);
const LinkRateEnum LinkRateEnum::TWOFIVE(2.5f);
const LinkRateEnum LinkRateEnum::TEN_G(10.0f);
const LinkRateEnum LinkRateEnum::TWENTYFIVE_G(25.0f);
const LinkRateEnum LinkRateEnum::FOURTY_G(40.0f);
const LinkRateEnum LinkRateEnum::HUNDRED_G(100.0f);
const LinkRateEnum LinkRateEnum::FOURHUNDRED_G(400.0f);

linkrates_t LeafSpineTopology::nicspd = linkrates_t::HUNDRED_G;
linkrates_t LeafSpineTopology::aggrspd = linkrates_t::FOURHUNDRED_G;
linkrates_t LeafSpineTopology::corespd = linkrates_t::FOURHUNDRED_G;

// double LeafSpineTopology::delayLeafDwn = 0.75;
// double LeafSpineTopology::delaySpineUp = 0.25;
// double LeafSpineTopology::delaySpineDwn = 0.25;
// double LeafSpineTopology::delayCoreUp = 0.25;
// double LeafSpineTopology::delayCoreDwn = 0.25;
double LeafSpineTopology::delayLeafUp = 1.0;
double LeafSpineTopology::delayLeafDwn = 1.0;
double LeafSpineTopology::delaySpineUp = 1.0;
double LeafSpineTopology::delaySpineDwn = 1.0;
double LeafSpineTopology::delayCoreUp = 1.0;
double LeafSpineTopology::delayCoreDwn = 1.0;


string ntoa(double n);
string itoa(uint64_t n);

LeafSpineTopology::LeafSpineTopology(int srvrsPerTor, int torsPerPod,
        int numPods, mem_b queuesize, Logfile* log, EventList* ev, FirstFit* f,
        queue_type q)
    : tors()
    , aggrs()
    , cores()
    , pipes_srvr_tor()
    , pipes_tor_aggr()
    , pipes_aggr_core()
    , queues_srvr_tor()
    , queues_tor_aggr()
    , queues_aggr_core()
    , ff(f)
    , logfile(log)
    , eventlist(ev)
    , qt(q)
    , numPods(numPods)
    , torsPerPod(torsPerPod)
    , srvrsPerTor(srvrsPerTor)
    , totalSrvrs(numPods*torsPerPod*srvrsPerTor)
    , numAggrs(0)
    , numCores(0)
    , queueSize(queuesize)
{
    set_params();
    init_network();
}

void
LeafSpineTopology::set_params()
{
    float torBw = srvrsPerTor*nicspd;

    // number of aggr switches in each pod; also equal to number of aggr port on
    // each aggregation switch or tor switch.
    int aggrPorts = ceil(torBw/static_cast<float>(aggrspd)); 
    numAggrs = aggrPorts;

    // round up the srvrsPerTor based on rounded computed number of aggrPorts 
    srvrsPerTor = aggrPorts*aggrspd/nicspd; 
    torBw = srvrsPerTor*nicspd;

    float podBw = torBw*torsPerPod;
    
    // number of core links on each agrreation switch, also equal to the total
    // number of core switches
    if (numPods > 1) {
        int corePorts = ceil(podBw/numAggrs/corespd);
        numCores = corePorts;

        podBw = corePorts*numAggrs*corespd;
        // rounded up torsPerPod value based on computed numCores. 
        torsPerPod = numCores*corespd/aggrspd;
    }

    totalSrvrs = srvrsPerTor*torsPerPod*numPods;
    cout<< " totalSrvrs " << totalSrvrs << endl;

}

void
LeafSpineTopology::init_network()
{
    QueueLoggerSampling* qLoggerUp;
    QueueLoggerSampling* qLoggerDwn;
    for (size_t pod_id = 0; pod_id < numPods; pod_id++) {

        // Tor switches and links/queues between servers between tors.
        for (size_t tor_id = 0; tor_id < torsPerPod; tor_id++) {
            if (qt == LOSSLESS) {
                tors.push_back(new Switch(
                    "Switch_TOR_"+ntoa(tor_id+torsPerPod*pod_id)));
            }
           
            for (size_t srvr_id = 0; srvr_id < srvrsPerTor; srvr_id++) {
                // create links
                pipes_srvr_tor.push_back(make_pair(
                    new Pipe(timeFromUs(delayLeafUp), *eventlist),
                    new Pipe(timeFromUs(delayLeafDwn), *eventlist)));

                // allocate queues
                qLoggerUp = new QueueLoggerSampling(timeFromMs(100),
                    *eventlist);
                logfile->addLogger(*qLoggerUp);
                qLoggerDwn = new QueueLoggerSampling(timeFromMs(100),
                    *eventlist);
                logfile->addLogger(*qLoggerDwn);
                queues_srvr_tor.push_back(make_pair(
                    alloc_queue(qLoggerUp, 1, (float)nicspd*1e3, queueSize),
                    alloc_queue(qLoggerDwn, 0, (float)nicspd*1e3, queueSize)
                    ));
                
                // add lossless ports to switches if needed
                auto qPair = queues_srvr_tor.back();
                qPair.first ->setName("SRV-" + ntoa(srvr_id+tor_id*srvrsPerTor) + "->ToR-" + ntoa(tor_id));
                qPair.second ->setName("ToR-" + ntoa(tor_id) + "->SRV-" + ntoa(+tor_id*srvrsPerTor));
                if (qt == LOSSLESS) {
                    tors.back()->addPort(qPair.second);
                    ((LosslessQueue*)(qPair.second))->setRemoteEndpoint(
                        qPair.first);
                } else if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    //no virtual queue needed at server
                    new LosslessInputQueue(*eventlist, qPair.first);
                }
            }
        }

        // aggregation switches and links/queues between aggregations switches
        // and tors.
        cout << "allocate the aggregation switch "<<endl;
        cout << " numAggrs " << numAggrs << " torsPerPod "<< torsPerPod << endl;
        for (size_t aggr_id = 0; aggr_id < numAggrs; aggr_id++) {
            if (qt == LOSSLESS) {
                aggrs.push_back(new Switch(
                    "Switch_AGGR_"+ntoa(aggr_id+numAggrs*pod_id)));
            }

            for (size_t tor_id = 0; tor_id < torsPerPod; tor_id++) {
                 pipes_tor_aggr.push_back(make_pair(
                    new Pipe(timeFromUs(delaySpineUp), *eventlist),
                    new Pipe(timeFromUs(delaySpineDwn), *eventlist)));

                qLoggerUp = new QueueLoggerSampling(timeFromMs(100),
                    *eventlist);
                logfile->addLogger(*qLoggerUp);
                qLoggerDwn = new QueueLoggerSampling(timeFromMs(100),
                    *eventlist);
                logfile->addLogger(*qLoggerDwn);
                queues_tor_aggr.push_back(make_pair(
                    alloc_queue(qLoggerUp, 0, (float)aggrspd*1e3, queueSize),
                    alloc_queue(qLoggerDwn, 0, (float)aggrspd*1e3, queueSize)));

                auto qPair = queues_tor_aggr.back();
                qPair.first ->setName("ToR-" + ntoa(tor_id) + "->AGG-" + ntoa(aggr_id));
                qPair.second->setName("AGG-" + ntoa(aggr_id) + "->ToR-" + ntoa(tor_id));
                if (qt == LOSSLESS) {
                    tors[tor_id+pod_id*torsPerPod]->addPort(qPair.first);
                    ((LosslessQueue*)(qPair.first))->setRemoteEndpoint(
                        qPair.second);
                    aggrs.back()->addPort(qPair.second);
                    ((LosslessQueue*)(qPair.second))->setRemoteEndpoint(
                        qPair.first);
                } else if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    new LosslessInputQueue(*eventlist, qPair.first);
                    new LosslessInputQueue(*eventlist, qPair.second);
                }
            }
        }
    }

    // core switches and links/queues between aggregation and core switches.
    cout << " Allocate core switch "<< endl;
    cout << " numCores " << numCores << " numPods " << numPods << endl;
    for (size_t core_id = 0; core_id < numCores; core_id++) {
        if (qt == LOSSLESS) {
            cores.push_back(new Switch("Switch_CORE_"+ntoa(core_id)));
        }

        for (size_t pod_id = 0; pod_id < numPods; pod_id++) {
            for (size_t aggr_id = 0; aggr_id < numAggrs; aggr_id++) {
                pipes_aggr_core.push_back(make_pair(
                    new Pipe(timeFromUs(delayCoreUp), *eventlist),
                    new Pipe(timeFromUs(delayCoreDwn), *eventlist)));

                qLoggerUp = new QueueLoggerSampling(timeFromMs(100),
                    *eventlist);
                logfile->addLogger(*qLoggerUp);
                qLoggerDwn = new QueueLoggerSampling(timeFromMs(100),
                    *eventlist);
                logfile->addLogger(*qLoggerDwn);
                queues_aggr_core.push_back(make_pair(
                    alloc_queue(qLoggerUp, 0, (float)corespd*1e3, queueSize),
                    alloc_queue(qLoggerDwn, 0, (float)corespd*1e3, queueSize)));

                auto qPair = queues_aggr_core.back();
                if (qt == LOSSLESS) {
                    int upLinkId =
                        core_id*numPods*numAggrs + pod_id*numAggrs + aggr_id;
                    aggrs[upLinkId]->addPort(qPair.first);
                    ((LosslessQueue*)(qPair.first))->setRemoteEndpoint(
                        qPair.second);
                    cores.back()->addPort(qPair.second);
                    ((LosslessQueue*)(qPair.second))->setRemoteEndpoint(
                        qPair.first);
                } else if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    new LosslessInputQueue(*eventlist, qPair.first);
                    new LosslessInputQueue(*eventlist, qPair.second);
                }
            }
        }
    }

    //init thresholds for lossless operation
    if (qt == LOSSLESS) {
        for (auto tor : tors){
            tor->configureLossless();
        }
        for (auto aggr : aggrs){
            aggr->configureLossless();
        }
        for (auto core : cores){
            core->configureLossless();
        }
    }
}

Queue*
LeafSpineTopology::alloc_queue(QueueLogger* queueLogger, bool isSrcQ,
        uint64_t speedMbps, mem_b queuesize)
{
    if (isSrcQ) {
        return new PriorityQueue(speedFromMbps(static_cast<float>(nicspd)*1e3),
            memFromPkt(FEEDER_BUFFER), *eventlist, queueLogger);
    }

    if (qt==RANDOM)
        return new RandomQueue(speedFromMbps(speedMbps),
            memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
            *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
    else if (qt==COMPOSITE)
        return new CompositeQueue(speedFromMbps(speedMbps), queuesize,
            *eventlist, queueLogger);
    else if (qt==CTRL_PRIO)
        return new CtrlPrioQueue(speedFromMbps(speedMbps), queuesize,
            *eventlist, queueLogger);
    else if (qt==ECN)
        return new ECNQueue(speedFromMbps(speedMbps),
            memFromPkt(2*SWITCH_BUFFER), *eventlist,
            queueLogger, memFromPkt(15));
    else if (qt==LOSSLESS)
        return new LosslessQueue(speedFromMbps(speedMbps),
            memFromPkt(50), *eventlist, queueLogger, NULL);
    else if (qt==LOSSLESS_INPUT)
        return new LosslessOutputQueue(speedFromMbps(speedMbps),
            memFromPkt(200), *eventlist, queueLogger);
    else if (qt==LOSSLESS_INPUT_ECN)
        return new LosslessOutputQueue(speedFromMbps(speedMbps),
            memFromPkt(10000), *eventlist, queueLogger,1,memFromPkt(16));
    assert(0);
}

void
check_no_null(Route* rt) {
    int fail = 0;
    for (unsigned int i=1; i<rt->size()-1; i+=2) {
        if (rt->at(i)==NULL){
            fail = 1;
            break;
        }
    }

    if (fail) {
        cout << "Null queue in route: ";
        for (unsigned int i=1; i<rt->size()-1; i+=2) {
          printf("%p ", rt->at(i));
        }
        cout<<endl;
        assert(0);
    }
}

vector<const Route*>*
LeafSpineTopology::get_paths(int src, int dest)
{
    vector<const Route*>* paths = new vector<const Route*>();
    route_t *routeout, *routeback;

    int srcTor = getTor(src);
    int destTor = getTor(dest);
    if (srcTor == destTor) {
        // both src and dest hosts are under the same tor switch
        routeout = new Route();

        // forward path
        routeout->push_back(queues_srvr_tor[src].first);
        routeout->push_back(pipes_srvr_tor[src].first);

        if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
            routeout->push_back(
                queues_srvr_tor[src].first->getRemoteEndpoint());
        }

        routeout->push_back(queues_srvr_tor[dest].second);
        routeout->push_back(pipes_srvr_tor[dest].second);

        // reverse path
        routeback = new Route();
        routeback->push_back(queues_srvr_tor[dest].first);
        routeback->push_back(pipes_srvr_tor[dest].first);

        if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
            routeback->push_back(
                queues_srvr_tor[dest].first->getRemoteEndpoint());
        }

        routeback->push_back(queues_srvr_tor[src].second);
        routeback->push_back(pipes_srvr_tor[src].second);
        
        // set reverse
        routeout->set_reverse(routeback);
        routeback->set_reverse(routeout);

        paths->push_back(routeout); 
        check_no_null(routeout);
        //print_paths(std::cout, src, paths);
        return paths;
    }

    int srcPod = getPod(src);
    int destPod = getPod(dest);
    if (srcPod == destPod) {
        // both src and dest hosts are in the same pod
        for (size_t aggr = 0; aggr < numAggrs; aggr++) {
            routeout = new Route();

            // forward path
            routeout->push_back(queues_srvr_tor[src].first);
            routeout->push_back(pipes_srvr_tor[src].first);

            if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                routeout->push_back(
                    queues_srvr_tor[src].first->getRemoteEndpoint());
            }
            
            int srcTorId = srcTor % torsPerPod; // local id in the pod
            int linkIdTorAggr =
                srcPod*numAggrs*torsPerPod + aggr*torsPerPod + srcTorId;
            routeout->push_back(queues_tor_aggr[linkIdTorAggr].first);
            routeout->push_back(pipes_tor_aggr[linkIdTorAggr].first);

            if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                routeout->push_back(
                    queues_tor_aggr[linkIdTorAggr].first->getRemoteEndpoint());
            }

            int destTorId = destTor % torsPerPod; // local id in the pod
            int linkIdAggrTor =
                destPod*numAggrs*torsPerPod + aggr*torsPerPod + destTorId;
            routeout->push_back(queues_tor_aggr[linkIdAggrTor].second);
            routeout->push_back(pipes_tor_aggr[linkIdAggrTor].second);

            if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                routeout->push_back(
                    queues_tor_aggr[linkIdAggrTor].second->getRemoteEndpoint());
            }

            routeout->push_back(queues_srvr_tor[dest].second);
            routeout->push_back(pipes_srvr_tor[dest].second);

            // reverse path
            routeback = new Route();
            routeback->push_back(queues_srvr_tor[dest].first);
            routeback->push_back(pipes_srvr_tor[dest].first);

            if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                routeback->push_back(
                    queues_srvr_tor[dest].first->getRemoteEndpoint());
            }

            linkIdTorAggr =
                destPod*numAggrs*torsPerPod + aggr*torsPerPod + destTorId;
            routeback->push_back(queues_tor_aggr[linkIdTorAggr].first);
            routeback->push_back(pipes_tor_aggr[linkIdTorAggr].first);

            if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                routeback->push_back(
                    queues_tor_aggr[linkIdTorAggr].first->getRemoteEndpoint());
            }

            linkIdAggrTor =
                srcPod*numAggrs*torsPerPod + aggr*torsPerPod + srcTorId;
            routeback->push_back(queues_tor_aggr[linkIdAggrTor].second);
            routeback->push_back(pipes_tor_aggr[linkIdAggrTor].second);

            if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                routeback->push_back(
                    queues_tor_aggr[linkIdAggrTor].second->getRemoteEndpoint());
            }

            routeback->push_back(queues_srvr_tor[src].second);
            routeback->push_back(pipes_srvr_tor[src].second);

            // set reverse
            routeout->set_reverse(routeback);
            routeback->set_reverse(routeout);

            paths->push_back(routeout); 
            check_no_null(routeout);
        }
        //print_paths(std::cout, src, paths);
        return paths;
    }

    // source and destination hosts are in different pods
    for (size_t aggrUp = 0; aggrUp < numAggrs; aggrUp++) {

        int srcTorId = srcTor % torsPerPod; // local id in the pod
        int fwdIdTorAggr =
            srcPod*numAggrs*torsPerPod + aggrUp*torsPerPod + srcTorId;
        for (size_t core = 0; core < numCores; core++) {
            int fwdIdAggrCore =
                numAggrs*numPods*core + srcPod*numAggrs + aggrUp; 
            for (size_t aggrDwn = 0; aggrDwn < numAggrs; aggrDwn++) {
                int fwdIdCoreAggr =
                    numAggrs*numPods*core + destPod*numAggrs + aggrDwn;
                int destTorId = destTor % torsPerPod; // local id in the pod
                int fwdIdAggrTor = destPod*numAggrs*torsPerPod +
                    aggrDwn*torsPerPod + destTorId;

                routeout = new Route();

                // forward path
                routeout->push_back(queues_srvr_tor[src].first);
                routeout->push_back(pipes_srvr_tor[src].first);

                if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    routeout->push_back(
                        queues_srvr_tor[src].first->getRemoteEndpoint());
                }
                
                routeout->push_back(queues_tor_aggr[fwdIdTorAggr].first);
                routeout->push_back(pipes_tor_aggr[fwdIdTorAggr].first);

                if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    routeout->push_back(queues_tor_aggr[
                        fwdIdTorAggr].first->getRemoteEndpoint());
                }

                routeout->push_back(queues_aggr_core[fwdIdAggrCore].first);
                routeout->push_back(pipes_aggr_core[fwdIdAggrCore].first);

                if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    routeout->push_back(queues_aggr_core[
                        fwdIdAggrCore].first->getRemoteEndpoint());
                }

                routeout->push_back(queues_aggr_core[fwdIdCoreAggr].second);
                routeout->push_back(pipes_aggr_core[fwdIdCoreAggr].second);

                if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    routeout->push_back(queues_aggr_core[
                        fwdIdCoreAggr].second->getRemoteEndpoint());
                }

                routeout->push_back(queues_tor_aggr[fwdIdAggrTor].second);
                routeout->push_back(pipes_tor_aggr[fwdIdAggrTor].second);

                if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    routeout->push_back(queues_tor_aggr[
                        fwdIdAggrTor].second->getRemoteEndpoint());
                }

                routeout->push_back(queues_srvr_tor[dest].second);
                routeout->push_back(pipes_srvr_tor[dest].second);

                // reverse path
                routeback = new Route();
                routeback->push_back(queues_srvr_tor[dest].first);
                routeback->push_back(pipes_srvr_tor[dest].first);

                if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    routeback->push_back(
                        queues_srvr_tor[dest].first->getRemoteEndpoint());
                }

                int revIdTorAggr = destPod*numAggrs*torsPerPod +
                    aggrDwn*torsPerPod + destTorId;
                routeback->push_back(queues_tor_aggr[revIdTorAggr].first);
                routeback->push_back(pipes_tor_aggr[revIdTorAggr].first);

                if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    routeback->push_back(queues_tor_aggr[
                        revIdTorAggr].first->getRemoteEndpoint());
                }

                int revIdAggrCore =
                    numAggrs*numPods*core + destPod*numAggrs + aggrDwn; 
                routeback->push_back(queues_aggr_core[revIdAggrCore].first);
                routeback->push_back(pipes_aggr_core[revIdAggrCore].first);

                if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    routeback->push_back(queues_aggr_core[
                        revIdAggrCore].first->getRemoteEndpoint());
                }

                int revIdCoreAggr =
                    numAggrs*numPods*core + srcPod*numAggrs + aggrUp;
                routeback->push_back(queues_aggr_core[revIdCoreAggr].second);
                routeback->push_back(pipes_aggr_core[revIdCoreAggr].second);

                if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    routeback->push_back(queues_aggr_core[
                        revIdCoreAggr].second->getRemoteEndpoint());
                }

                int revIdAggrTor =
                    srcPod*numAggrs*torsPerPod + aggrUp*torsPerPod + srcTorId;
                routeback->push_back(queues_tor_aggr[revIdAggrTor].second);
                routeback->push_back(pipes_tor_aggr[revIdAggrTor].second);

                if (qt == LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN) {
                    routeback->push_back(queues_tor_aggr[
                        revIdAggrTor].second->getRemoteEndpoint());
                }

                routeback->push_back(queues_srvr_tor[src].second);
                routeback->push_back(pipes_srvr_tor[src].second);

                // set reverse
                routeout->set_reverse(routeback);
                routeback->set_reverse(routeout);

                paths->push_back(routeout); 
                check_no_null(routeout);
            }
        }
    }
    //print_paths(std::cout, src, paths);
    return paths;
}

std::string
LeafSpineTopology::getSrcStr(int src)
{
    int podId = src / (srvrsPerTor*torsPerPod);
    int torId = (src % (srvrsPerTor*torsPerPod)) / srvrsPerTor;
    int srvrId = (src % (srvrsPerTor*torsPerPod)) % srvrsPerTor;
    std::stringstream ss;
    ss << "(" << podId << ", " << torId << ", " << srvrId << ")"; 
    return ss.str(); 
}

std::string
LeafSpineTopology::getTorStr(Queue* q, bool up)
{
    size_t iter = 0;
    Queue* qCheck = NULL;
    while (true) {
        qCheck = NULL;
        if (up) {
            if (iter >= queues_srvr_tor.size()) break;
            qCheck = queues_srvr_tor[iter].first;
        } else {
            if (iter >= queues_tor_aggr.size()) break;
            qCheck = queues_tor_aggr[iter].second;
        }

        if (qCheck == q)
            break;
        iter++;
    }

    if (!qCheck)
        assert(0);

    int i;
    int podId;
    int torId;
    if (up) {
        i = iter/srvrsPerTor;
        podId = i/torsPerPod;
        torId = i%torsPerPod;
    } else {
        podId = iter/(numAggrs*torsPerPod);
        iter =  iter%(numAggrs*torsPerPod);
        torId = iter % torsPerPod;
        i = podId*torsPerPod + torId; 
    }
    std::stringstream ss;
    ss << i << "_(" << podId << ", " << torId << ")"; 
    return ss.str(); 
}

std::string
LeafSpineTopology::getAggrStr(Queue* q, bool up)
{
    size_t iter = 0;
    Queue* qCheck = NULL;
    while (true) {
        qCheck = NULL;
        if (up) {
            if (iter >= queues_tor_aggr.size()) break;
            qCheck = queues_tor_aggr[iter].first;
        } else {
            if (iter >= queues_aggr_core.size()) break;
            qCheck = queues_aggr_core[iter].second;
        }

        if (qCheck == q)
            break;
        iter++;
    }

    if (!qCheck)
        assert(0);

    int i;
    int podId;
    int aggrId;
    if (up) {
        podId = iter/(torsPerPod*numAggrs);
        i = iter/torsPerPod;
        aggrId = i%numAggrs;
        //iter = iter%(torsPerPod*numAggrs);
        //aggrId = (iter/torsPerPod);
        //i = podId*numAggrs + aggrId;
    } else {
        i = iter%(numPods*numAggrs);
        podId = i/numAggrs;
        aggrId = i%numAggrs;
    }
    std::stringstream ss;
    ss << i << "_(" << podId << ", " << aggrId << ")"; 
    return ss.str(); 
}

std::string
LeafSpineTopology::getCoreStr(Queue* q)
{
    size_t iter = 0;
    while (iter < queues_aggr_core.size()) {
        Queue* qCheck = queues_aggr_core[iter].first;
        if (qCheck == q)
            break;
        iter++;
    }

    if (iter == queues_aggr_core.size())
        assert(0);

    int i = iter/(numPods*numAggrs);
    std::stringstream ss;
    ss << i; 
    return ss.str(); 
}

std::string
LeafSpineTopology::getDestStr(Queue* q)
{
    size_t dest = 0;
    while (dest < queues_srvr_tor.size()) {
        if (queues_srvr_tor[dest].second == q)
            break;
        dest++;
    }

    if (dest == queues_srvr_tor.size())
        assert(0);

    int podId = dest / (srvrsPerTor*torsPerPod);
    int torId = (dest % (srvrsPerTor*torsPerPod)) / srvrsPerTor;
    int srvrId = (dest % (srvrsPerTor*torsPerPod)) % srvrsPerTor;
    std::stringstream ss;
    ss << dest << "_(" << podId << ", " << torId << ", " << srvrId << ")"; 
    return ss.str(); 
}

void
LeafSpineTopology::print_paths(std::ostream& p, int src,
        vector<const Route*>* paths)
{
    for (unsigned int i=0; i<paths->size(); i++)
	    print_path(p, src, paths->at(i));
}

void
LeafSpineTopology::print_path(std::ostream& paths, int src,
        const Route* route)
{
    paths << "SRC_" << src << "_" << getSrcStr(src) << " "; 
    if (route->size()/2 == 2) {
        paths << "TOR_" << getTorStr((Queue*)route->at(0), true) << " ";
        paths << "DEST_" << getDestStr((Queue*)route->at(2)) << " ";
    } else if (route->size()/2 == 4) {
        paths << "TOR_" << getTorStr((Queue*)route->at(0), true) << " ";
        paths << "AGGR_" << getAggrStr((Queue*)route->at(2), true) << " ";
        paths << "TOR_" << getTorStr((Queue*)route->at(4), false) << " ";
        paths << "DEST_" << getDestStr((Queue*)route->at(6)) << " ";
    } else if (route->size()/2 == 6) {
        paths << "TOR_" << getTorStr((Queue*)route->at(0), true) << " ";
        paths << "AGGR_" << getAggrStr((Queue*)route->at(2), true) << " ";
        paths << "CORE_" << getCoreStr((Queue*)route->at(4)) << " ";
        paths << "AGGR_" << getAggrStr((Queue*)route->at(6), false) << " ";
        paths << "TOR_" << getTorStr((Queue*)route->at(8), false) << " ";
        paths << "DEST_" << getDestStr((Queue*)route->at(10)) << " ";
    } else {
        paths << "Wrong hop count " << ntoa(route->size()/2);
    }

    paths << endl;
}
