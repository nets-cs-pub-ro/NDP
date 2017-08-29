OBJS=eventlist.o tcppacket.o pipe.o queue.o queue_lossless.o queue_lossless_input.o queue_lossless_output.o ecnqueue.o tcp.o dctcp.o mtcp.o loggers.o logfile.o clock.o config.o network.o qcn.o exoqueue.o randomqueue.o cbr.o cbrpacket.o sent_packets.o ndp.o ndppacket.o eth_pause_packet.o tcp_transfer.o tcp_periodic.o compositequeue.o prioqueue.o cpqueue.o ndp_transfer.o compositeprioqueue.o switch.o dctcp_transfer.o fairpullqueue.o route.o
HDRS=network.h ndp.h queue_lossless.h queue_lossless_input.h queue_lossless_output.h compositequeue.h prioqueue.h cpqueue.h queue.h loggers.h loggertypes.h pipe.h eventlist.h config.h tcp.h dctcp.h mtcp.h sent_packets.h tcppacket.h ndppacket.h eth_pause_packet.h ndp_transfer.h compositeprioqueue.h ecnqueue.h switch.h dctcp_transfer.h 

CC=g++ 
CFLAGS= -Wall -g

all:	htsim lib 

lib:	$(OBJS) $(HDRS)
	ar -rvu libhtsim.a $(OBJS)

parse_output: parse_output.o
	$(CC) $(CFLAGS) parse_output.o libhtsim.a -o parse_output 

htsim:	$(OBJS) main.o
	$(CC) $(CFLAGS) $(OBJS) main.o -o htsim

clean:	
	rm -f *.o htsim htsim_* libhtsim.a
parse_output.o: parse_output.cpp libhtsim.a
config.o:	config.cpp config.h
switch.o: 	switch.cpp switch.h
eventlist.o:    eventlist.cpp eventlist.h config.h
main.o:		main.cpp $(HDRS)
sent_packets.o:		sent_packets.h sent_packets.cpp
queue.o:	queue.cpp  $(HDRS)
queue_lossless.o:	queue_lossless.cpp  $(HDRS)
queue_lossless_input.o:	queue_lossless_input.cpp  $(HDRS)
queue_lossless_output.o:	queue_lossless_output.cpp  $(HDRS)
ecnqueue.o:	ecnqueue.cpp  $(HDRS)
exoqueue.o:	exoqueue.cpp $(HDRS)
pipe.o:		pipe.cpp $(HDRS)
network.o:	network.cpp  $(HDRS)
fairpullqueue.o:	fairpullqueue.cpp  $(HDRS)
route.o:	route.cpp  $(HDRS)
tcp.o:		tcp.cpp  $(HDRS)
dctcp.o:		dctcp.cpp  $(HDRS)
ndp.o:		ndp.cpp $(HDRS)
ndplite.o:	ndplite.cpp $(HDRS)
mtcp.o:		mtcp.cpp $(HDRS)
tcppacket.o:	tcppacket.cpp $(HDRS)
loggers.o:	loggers.cpp $(HDRS)
logfile.o:	logfile.cpp  $(HDRS)
clock.o:	clock.cpp clock.h eventlist.h config.h
compositequeue.o: compositequeue.cpp $(HDRS)
prioqueue.o: prioqueue.cpp $(HDRS)
cpqueue.o: cpqueue.cpp $(HDRS)
compositeprioqueue.o: compositeprioqueue.cpp $(HDRS)
ndp_transfer.o: ndp_transfer.cpp $(HRDS)
qcn.o: qcn.cpp qcn.h loggers.h config.h 

.cpp.o:
	source='$<' object='$@' libtool=no depfile='$(DEPDIR)/$*.Po' tmpdepfile='$(DEPDIR)/$*.TPo' $(CXXDEPMODE) $(depcomp) $(CC) $(CFLAGS) -Wuninitialized  -c -o $@ `test -f $< || echo '$(srcdir)/'`$<
