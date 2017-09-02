CC = g++
CFLAGS = -Wall -g -O
CRT=`pwd`
INCLUDE= -I/$(CRT)/.. -I$(CRT) 
#-I$(CRT)/ksp -I$(CRT)/ksp/boost
LIB=-L.. 
#-Lksp

all:	htsim_tcp htsim_ndp htsim_dctcp_permutation htsim_ndp_permutation htsim_ndp_random htsim_ndp_permutation_lossless htsim_ndp_permutation_fail htsim_ndp_incast htsim_ndp_incast_shortflows htsim_ndp_incast_collateral htsim_ndp_outcast htsim_ndp_outcast_shortflows htsim_tcp_permutation htsim_tcp_perm_shortflows htsim_ndp_perm_shortflows htsim_tcp_incast_shortflows htsim_dctcp_permutation_lossless htsim_ndp_incast_shortflows_lossless htsim_dctcp_incast_shortflows htsim_dctcp_random_shortflows htsim_dctcp_random_shortflows_lossless htsim_dctcp_perm_shortflows htsim_dctcp_perm_shortflows_lossless htsim_ndp_random_shortflows htsim_ndp_oversubscribed htsim_dctcp_oversubscribed htsim_ndp_in_out htsim_dctcp_incast_collateral htsim_dctcp_incast_collateral_lossless 
#htsim_ndp_incast_shortflows_demo 

htsim_tcp: main.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_tcp

htsim_tcp_permutation: main_tcp_permutation.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_tcp_permutation.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_tcp_permutation

htsim_dctcp_permutation: main_dctcp_permutation.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_dctcp_permutation.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_dctcp_permutation

htsim_dctcp_random_shortflows: main_dctcp_random_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_dctcp_random_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_dctcp_random_shortflows

htsim_ndp_random_shortflows: main_ndp_random_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_random_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_random_shortflows

htsim_dctcp_random_shortflows_lossless: main_dctcp_random_shortflows_lossless.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_dctcp_random_shortflows_lossless.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_dctcp_random_shortflows_lossless


htsim_tcp_incast_shortflows: main_tcp_incast_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_tcp_incast_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_tcp_incast_shortflows

htsim_dctcp_incast_shortflows: main_dctcp_incast_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_dctcp_incast_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_dctcp_incast_shortflows

htsim_ndp: main_ndp.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp

htsim_ndp_permutation: main_ndp_permutation.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_permutation.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_permutation

htsim_ndp_random: main_ndp_random.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_random.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_random

htsim_ndp_permutation_lossless: main_ndp_permutation_lossless.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_permutation_lossless.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_permutation_lossless

htsim_dctcp_permutation_lossless: main_dctcp_permutation_lossless.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_dctcp_permutation_lossless.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_dctcp_permutation_lossless

htsim_ndp_oversubscribed_shortflows:  main_ndp_oversubscribed_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o  main_ndp_oversubscribed_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o  htsim_ndp_oversubscribed_shortflows

htsim_ndp_permutation_fail: main_ndp_permutation_fail.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_permutation_fail.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_permutation_fail

htsim_ndp_incast: main_ndp_incast.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_incast.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_incast

htsim_ndp_incast_shortflows: main_ndp_incast_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_incast_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_incast_shortflows


htsim_ndp_incast_shortflows_lossless: main_ndp_incast_shortflows_lossless.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_incast_shortflows_lossless.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_incast_shortflows_lossless

#htsim_ndp_incast_shortflows_demo: main_ndp_incast_shortflows_demo.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
#	$(CC) $(CFLAGS) firstfit.o main_ndp_incast_shortflows_demo.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_incast_shortflows_demo

htsim_ndp_incast_collateral: main_ndp_incast_collateral.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_incast_collateral.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_incast_collateral

htsim_ndp_outcast: main_ndp_outcast.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_outcast.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_outcast

htsim_ndp_in_out: main_ndp_in_out.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_in_out.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_in_out

htsim_ndp_perm_shortflows: main_ndp_perm_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_perm_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_perm_shortflows

htsim_ndplite_perm_shortflows: main_ndplite_perm_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndplite_perm_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndplite_perm_shortflows

htsim_dctcp_perm_shortflows: main_dctcp_perm_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_dctcp_perm_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_dctcp_perm_shortflows

htsim_dctcp_perm_shortflows_lossless: main_dctcp_perm_shortflows_lossless.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_dctcp_perm_shortflows_lossless.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_dctcp_perm_shortflows_lossless

htsim_dctcp_incast_collateral: main_dctcp_incast_collateral.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_dctcp_incast_collateral.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_dctcp_incast_collateral

htsim_dctcp_incast_collateral_lossless: main_dctcp_incast_collateral_lossless.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_dctcp_incast_collateral_lossless.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_dctcp_incast_collateral_lossless

htsim_tcp_perm_shortflows: main_tcp_perm_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_tcp_perm_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_tcp_perm_shortflows

htsim_ndp_outcast_shortflows: main_ndp_outcast_shortflows.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_outcast_shortflows.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_outcast_shortflows

htsim_ndp_oversubscribed: main_ndp_oversubscribed.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_ndp_oversubscribed.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_ndp_oversubscribed


htsim_dctcp_oversubscribed: main_dctcp_oversubscribed.o firstfit.o ../libhtsim.a vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o multihomed_fat_tree_topology.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main_dctcp_oversubscribed.o vl2_topology.o fat_tree_topology.o bcube_topology.o connection_matrix.o oversubscribed_fat_tree_topology.o shortflows.o star_topology.o multihomed_fat_tree_topology.o $(LIB) -lhtsim -o htsim_dctcp_oversubscribed

main.o: main.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main.cpp 

main_tcp_permutation.o: main_tcp_permutation.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_tcp_permutation.cpp 

main_dctcp_permutation.o: main_dctcp_permutation.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_dctcp_permutation.cpp 

main_dctcp_random_shortflows.o: main_dctcp_random_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_dctcp_random_shortflows.cpp 

main_ndp_random_shortflows.o: main_ndp_random_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_random_shortflows.cpp 

main_dctcp_random_shortflows_lossless.o: main_dctcp_random_shortflows_lossless.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_dctcp_random_shortflows_lossless.cpp 

main_tcp_incast_shortflows.o: main_tcp_incast_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_tcp_incast_shortflows.cpp 

main_dctcp_incast_shortflows.o: main_dctcp_incast_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_dctcp_incast_shortflows.cpp 

main_ndp_incast_shortflows_lossless.o: main_ndp_incast_shortflows_lossless.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_incast_shortflows_lossless.cpp 

main_ndp.o: main_ndp.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp.cpp 

main_ndp_permutation.o: main_ndp_permutation.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_permutation.cpp 

main_ndplite_permutation.o: main_ndplite_permutation.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndplite_permutation.cpp 

main_ndp_random.o: main_ndp_random.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_random.cpp 

main_ndp_permutation_lossless.o: main_ndp_permutation_lossless.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_permutation_lossless.cpp 

main_dctcp_permutation_lossless.o: main_dctcp_permutation_lossless.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_dctcp_permutation_lossless.cpp 

main_ndp_bcube_permutation.o: main_ndp_bcube_permutation.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_bcube_permutation.cpp 

main_ndp_camcube_permutation.o: main_ndp_camcube_permutation.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_camcube_permutation.cpp 

main_ndp_oversubscribed.o: main_ndp_oversubscribed.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_oversubscribed.cpp

main_dctcp_oversubscribed.o: main_dctcp_oversubscribed.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_dctcp_oversubscribed.cpp 

main_ndp_oversubscribed_shortflows.o: main_ndp_oversubscribed_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c  main_ndp_oversubscribed_shortflows.cpp

main_ndp_permutation_fail.o: main_ndp_permutation_fail.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_permutation_fail.cpp 

main_ndp_incast.o: main_ndp_incast.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_incast.cpp 

main_ndp_incast_shortflows.o: main_ndp_incast_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_incast_shortflows.cpp 

main_ndplite_incast_shortflows.o: main_ndplite_incast_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndplite_incast_shortflows.cpp 

#main_ndp_incast_shortflows_demo.o: main_ndp_incast_shortflows_demo.cpp
#	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_incast_shortflows_demo.cpp 

main_ndp_incast_collateral.o: main_ndp_incast_collateral.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_incast_collateral.cpp 

main_ndplite_incast_collateral.o: main_ndplite_incast_collateral.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndplite_incast_collateral.cpp 

main_ndp_outcast.o: main_ndp_outcast.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_outcast.cpp 

main_ndp_in_out.o: main_ndp_in_out.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_in_out.cpp 


main_ndp_perm_shortflows.o: main_ndp_perm_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_perm_shortflows.cpp 

main_ndplite_perm_shortflows.o: main_ndplite_perm_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndplite_perm_shortflows.cpp 

main_dctcp_perm_shortflows.o: main_dctcp_perm_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_dctcp_perm_shortflows.cpp 

main_dctcp_perm_shortflows_lossless.o: main_dctcp_perm_shortflows_lossless.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_dctcp_perm_shortflows_lossless.cpp 

main_tcp_perm_shortflows.o: main_tcp_perm_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_tcp_perm_shortflows.cpp 

main_dctcp_incast_collateral.o: main_dctcp_incast_collateral.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_dctcp_incast_collateral.cpp 

main_dctcp_incast_collateral_lossless.o: main_dctcp_incast_collateral_lossless.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_dctcp_incast_collateral_lossless.cpp 

main_ndp_outcast_shortflows.o: main_ndp_outcast_shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main_ndp_outcast_shortflows.cpp 


star_topology.o: star_topology.cpp star_topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c star_topology.cpp 

shortflows.o: shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c shortflows.cpp 

connection_matrix.o: connection_matrix.cpp bcube_topology.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c connection_matrix.cpp 

firstfit.o: firstfit.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c firstfit.cpp

vl2_topology.o: vl2_topology.cpp vl2_topology.h topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c vl2_topology.cpp

bcube_topology.o: bcube_topology.cpp bcube_topology.h topology.h ../matrix.h
	$(CC) $(INCLUDE) $(CFLAGS) -c bcube_topology.cpp

camcubetopology.o: camcubetopology.cpp camcubetopology.h topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c camcubetopology.cpp

subflow_control.o: subflow_control.cpp subflow_control.h 
	$(CC) $(INCLUDE) $(CFLAGS) -c subflow_control.cpp

fat_tree_topology.o: fat_tree_topology.cpp fat_tree_topology.h topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c fat_tree_topology.cpp

oversubscribed_fat_tree_topology.o: oversubscribed_fat_tree_topology.cpp oversubscribed_fat_tree_topology.h topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c oversubscribed_fat_tree_topology.cpp

multihomed_fat_tree_topology.o: multihomed_fat_tree_topology.cpp multihomed_fat_tree_topology.h topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c multihomed_fat_tree_topology.cpp

clean:	
	rm -f *.o htsim_ndp* htsim_tcp* htsim_dctcp*
