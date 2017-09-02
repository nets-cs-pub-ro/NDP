#!/bin/sh
../../datacenter/htsim_ndp_permutation -o logfile -strat perm -conns 432 -nodes 432 -cwnd 23 -q 8 > debugout 
python process_data.py logfile
../../datacenter/htsim_dctcp_permutation -o dctcp_logfile -nodes 432 -conns 432 -cwnd 15 -q 100 > debug_dctcp
python process_dctcp_data.py dctcp_logfile
gnuplot permutation.plot
