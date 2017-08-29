// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#ifndef LOGGERTYPES_H
#define LOGGERTYPES_H

class Packet;
class TcpSrc;
class NdpSrc;
class NdpLiteSrc;
class Queue;
class QcnReactor;
class QcnQueue;
class MultipathTcpSrc;
class Logfile;
class RawLogEvent;

class Logged {
 public:
    typedef uint32_t id_t;
    Logged(const string& name) {_name=name; id=LASTIDNUM; Logged::LASTIDNUM++; }
    virtual ~Logged() {}
    virtual void setName(const string& name) { _name=name; }
    virtual const string& str() { return _name; };
    id_t id;
    string _name;
 private:
    static id_t LASTIDNUM;
};

class Logger {
    friend class Logfile;
 public:
    enum EventType { QUEUE_EVENT=0, TCP_EVENT=1, TCP_STATE=2, TRAFFIC_EVENT=3, 
		     QUEUE_RECORD=4, QUEUE_APPROX=5, TCP_RECORD=6, 
		     QCN_EVENT=7, QCNQUEUE_EVENT=8, 
		     TCP_TRAFFIC=9, NDP_TRAFFIC=10, 
		     TCP_SINK = 11, MTCP = 12, ENERGY = 13, 
		     TCP_MEMORY = 14, NDP_EVENT=15, NDP_STATE=16, NDP_RECORD=17, 
		     NDP_SINK = 18, NDP_MEMORY = 19};
    static string event_to_str(RawLogEvent& event);
    Logger() {};
    virtual ~Logger(){};
 protected:
    void setLogfile(Logfile& logfile) { _logfile=&logfile; }
    Logfile* _logfile;
};

class TrafficLogger : public Logger {
 public:
    enum TrafficEvent { PKT_ARRIVE=0, PKT_DEPART=1, PKT_CREATESEND=2, PKT_DROP=3, PKT_RCVDESTROY=4, PKT_CREATE=5, PKT_SEND=6, PKT_TRIM=7, PKT_BOUNCE=8 };
    virtual void logTraffic(Packet& pkt, Logged& location, TrafficEvent ev) =0;
    virtual ~TrafficLogger(){};
};

class QueueLogger : public Logger  {
 public:
    enum QueueEvent { PKT_ENQUEUE=0, PKT_DROP=1, PKT_SERVICE=2, PKT_TRIM=3, PKT_BOUNCE=4 };
    enum QueueRecord { CUM_TRAFFIC=0 };
    enum QueueApprox { QUEUE_RANGE=0, QUEUE_OVERFLOW=1 };
    virtual void logQueue(Queue& queue, QueueEvent ev, Packet& pkt) = 0;
    virtual ~QueueLogger(){};
};

class MultipathTcpLogger  : public Logger {
 public:
    enum MultipathTcpEvent { CHANGE_A=0, RTT_UPDATE=1, WINDOW_UPDATE=2, RATE=3, MEMORY=4 };

    virtual void logMultipathTcp(MultipathTcpSrc &src, MultipathTcpEvent ev) =0;
    virtual ~MultipathTcpLogger(){};
};

class EnergyLogger  : public Logger {
 public:
    enum EnergyEvent { DRAW=0 };

    virtual ~EnergyLogger(){};
};

class TcpLogger  : public Logger {
 public:
    enum TcpEvent { TCP_RCV=0, TCP_RCV_FR_END=1, TCP_RCV_FR=2, TCP_RCV_DUP_FR=3, 
		    TCP_RCV_DUP=4, TCP_RCV_3DUPNOFR=5, 
		    TCP_RCV_DUP_FASTXMIT=6, TCP_TIMEOUT=7 };
    enum TcpState { TCPSTATE_CNTRL=0, TCPSTATE_SEQ=1 };
    enum TcpRecord { AVE_CWND=0 };
    enum TcpSinkRecord { RATE = 0 };
    enum TcpMemoryRecord  {MEMORY = 0};

    virtual void logTcp(TcpSrc &src, TcpEvent ev) =0;
    virtual ~TcpLogger(){};
};

class NdpLogger  : public Logger {
 public:
    enum NdpEvent { NDP_RCV=0, NDP_RCV_FR_END=1, NDP_RCV_FR=2, NDP_RCV_DUP_FR=3, 
		    NDP_RCV_DUP=4, NDP_RCV_3DUPNOFR=5, 
		    NDP_RCV_DUP_FASTXMIT=6, NDP_TIMEOUT=7 };
    enum NdpState { NDPSTATE_CNTRL=0, NDPSTATE_SEQ=1 };
    enum NdpRecord { AVE_CWND=0 };
    enum NdpSinkRecord { RATE = 0 };
    enum NdpMemoryRecord  {MEMORY = 0};

    virtual void logNdp(NdpSrc &src, NdpEvent ev) =0;
    virtual ~NdpLogger(){};
};

class NdpLiteLogger  : public Logger {
 public:
    enum NdpLiteEvent { NDP_RCV=0, NDP_RCV_FR_END=1, NDP_RCV_FR=2, NDP_RCV_DUP_FR=3, 
		    NDP_RCV_DUP=4, NDP_RCV_3DUPNOFR=5, 
		    NDP_RCV_DUP_FASTXMIT=6, NDP_TIMEOUT=7 };
    enum NdpLiteState { NDPSTATE_CNTRL=0, NDPSTATE_SEQ=1 };
    enum NdpLiteRecord { AVE_CWND=0 };
    enum NdpLiteSinkRecord { RATE = 0 };
    enum NdpLiteMemoryRecord  {MEMORY = 0};

    virtual void logNdpLite(NdpLiteSrc &src, NdpLiteEvent ev) =0;
    virtual ~NdpLiteLogger(){};
};

class QcnLogger  : public Logger {
 public:
    enum QcnEvent { QCN_SEND=0, QCN_INC=1, QCN_DEC=2, QCN_INCD=3, QCN_DECD=4 };
    enum QcnQueueEvent { QCN_FB=0, QCN_NOFB=1 };
    virtual void logQcn(QcnReactor &src, QcnEvent ev, double var3) =0;
    virtual void logQcnQueue(QcnQueue &src, QcnQueueEvent ev, double var1, double var2, double var3) =0;
    virtual ~QcnLogger(){};
};

#endif
