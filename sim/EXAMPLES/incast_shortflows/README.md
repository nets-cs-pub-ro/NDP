# NDP Example: Incast as a Function of Number of Senders

In this example we generate an incast traffic matrix: _n_ hosts sends
to one receiver.  We vary the number of senders.  The file size is
450KB, or 50 9KB packets.  All flows start simultaneously, which is
the worst-case example for an incast.  The topology is a 432-node
10Gb/s FatTree, with 8 packet queues.  The initial window is 23
packets, whcih is excessive for NDP - a smaller window would be
preferred in this case - but 23 packets are required for the full
permutation traffic matrix, so this demonstrates that incast behaviour
is good, even with a suboptimal choice of IW.

## Running the Example

You'll need python and gnuplot.

* To run the example, simply run "./run.sh".
* The data is graphed in incast.pdf

This is essentially the NDP curve from Figure 16 of the NDP Sigcomm'17
paper.

## Comments

Many packets arrive simultaneously.  
