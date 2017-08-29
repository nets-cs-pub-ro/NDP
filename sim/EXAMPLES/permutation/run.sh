#!/bin/sh
../../datacenter/htsim_ndp_permutation -o logfile -strat perm -conns 432 -nodes 432 -cwnd 23 -q 8 > debugout 
python process_data.py logfile
gnuplot permutation.plot
