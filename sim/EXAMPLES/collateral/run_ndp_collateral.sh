#!/bin/sh
../../datacenter/htsim_ndp_incast_collateral -strat perm -nodes 432 -conns 64 -q 8 -cwnd 15
../../parse_output logout.dat -ascii | grep NDP_SINK > ndp_output
grep 7976 ndp_output > ndp_longflow
grep -v 7976 ndp_output > ndp_incast_raw
python process_ndp_collateral.py > ndp_incast
gnuplot collateral_ndp.plot
