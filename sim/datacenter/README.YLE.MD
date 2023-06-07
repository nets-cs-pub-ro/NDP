Here are the notes:
1. One bug here. In a case, where NACK packet is set to be not_pull and the receiver sends another PULL packet back to sender;
Unfortunately, the PULL packet gets dropped. When sender processes NACK packet, it erases the timer for this packet, and push the packet into the _rtx_queue;
As PULL packet never gets arrived, only timeout can trigger the retransmission;
However, the retransmit_packet looks for the _sent_times of that packet. because processNack erased the _sent_time entries for that packets, this function would not triggers any retransmision.
At the end, the simulator exits silently without finishing this message;
To fix it, if timeout happens and we did not find any entries in _sent_time, we would call send_packet to push the packets in _rtx_queue out for the retransmission. 
The fix is here https://github.com/yfle0707/NDP/blob/0e0ed7aeb84ed0df6f0a570303d5d1cb7608bb6e/sim/ndp.cpp#L851.

2. Another bug found here. I have a message of two packets.  The first packet gets payload cut, the receiver returns an NACK and PULL packet. 
the second packet gets through, the receiver returns an ACK and PULL packet.
Two PULL packets gets dropped. The NACK packet arrives to the sender earlier than the ACK packet. 
ProcessNack erase the _sent_timer and enqueue the nack to the _rtx_queue;
then, processAck erase the _sent_timer for this packet, and call update_rtx_time.
Because update_rtx_time sets the _rtx_timeout to be timeInf as the _sent_times is empty.  
The timeout never triggers. As a result, the message never finishes. The simulator exits silently. 
This bug is triggered when I run this workloads with the following settup. 

```
time ./htsim_ndp_trace -flow GoogleRPC_he512_lf50_n8000_it0_ih100_ib250000_il5.dat -mtu 1000 -strat single -bdp 120000
```

To understand each line in GoogleRPC_he512_lf50_n8000_it0_ih100_ib250000_il5.dat file, you can refer to this link (https://github.com/alibaba-edu/High-Precision-Congestion-Control/blob/9f4be2a9ead8a90e8bf732c66bd758c00e58e5be/simulation/mix/flow.txt#L259)

3. To reproduce the results that I showed in the meeting, you can use the following command.
```
time ./htsim_ndp_trace -flow GoogleRPC_he512_lf30_n8000_it0_ih0_ib250000_il0.dat -mtu 4000 -strat perm -bdp 120000
```
