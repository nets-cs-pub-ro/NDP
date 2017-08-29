#!/bin/sh
flowsize=450000
cwnd=23
for ((conns=1;conns<=431;conns=conns+10)) ;
do
    echo ../../datacenter/htsim_ndp_incast_shortflows -o incast${cwnd}_q8_c${conns}_f${flowsize} -conns $conns -nodes 432 -cwnd ${cwnd} -q 8 -strat perm -flowsize $flowsize
    ../../datacenter/htsim_ndp_incast_shortflows -o incast${cwnd}_q8_c${conns}_f${flowsize} -conns $conns -nodes 432 -cwnd ${cwnd} -q 8 -strat perm -flowsize $flowsize > ts_incast${cwnd}_q8_c${conns}_f${flowsize}
	python process_data_incast_conns.py incast${cwnd}_q8_c${conns}_f${flowsize} $conns $flowsize $cwnd ndp
done
gnuplot incast.plot
