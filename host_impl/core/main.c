#include "../common/logger.h"
#include "defs.h"
#include <errno.h>
#include "helpers.h"
#include <inttypes.h>
#include <netinet/in.h>
#include <rte_cycles.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

//TODO don't forget that dpdk (or because of the switch) must wait a while before sending any packets


struct core_stupid_counters cctrs;


struct ndp_core_data core;

static volatile int keep_running = 1;

static void ctrlc_handler(int x __attribute__((unused)))
{
//LOG_MSG(LOGLVL1, "ctrl-c pressed!");
	keep_running = 0;
}


int main(int argc, char ** argv)
{

#if ! __x86_64__

	exit_msg(1, "this probably only works on x86_64 for now");
#endif


	memset(&cctrs, 0, sizeof(struct core_stupid_counters));


	if(signal(SIGINT, ctrlc_handler) == SIG_ERR)
		LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> signal error: %s", __func__, strerror(errno));

	if(init_pre_dpdk(argc, argv))
		exit(1);

	if(init_dpdk())
		exit(1);

	#if NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
		int pullq_lcore_started = 0;
	#endif

	#if NDP_CORE_RECORD_HW_TS_ANY
		int hw_ts_recorder_started = 0;
	#endif

	unsigned int slave_lcore_id;
	RTE_LCORE_FOREACH_SLAVE(slave_lcore_id)
	{

		#if NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
			if(!pullq_lcore_started)
			{
				if(rte_eal_remote_launch(pullq_thread_func, NULL, slave_lcore_id))
					LOG_MSG_DO(LOGLVL1, exit(1), "pullq_thread_func rte_eal_remote_launch err")
				else
					LOG_MSG(LOGLVL10, "pullq lcore thread launched");

				pullq_lcore_started = 1;
				continue;
			}
		#endif

		#if NDP_CORE_RECORD_HW_TS_ANY
			if(!hw_ts_recorder_started)
			{
				//was slave_lcore_id++ for some reason at one point
				if(rte_eal_remote_launch(record_hw_ts_thread_func, NULL, slave_lcore_id))
					LOG_MSG_DO(LOGLVL1, exit(1), "recorder rte_eal_remote_launch err")
				else
					LOG_MSG(LOGLVL10, "hw ts recorder thread launched");

				hw_ts_recorder_started = 1;
				continue;
			}
		#endif

		LOG_MSG_DO(LOGLVL1, exit(1), "more lcores than needed ?");
	}


	printf("hi!\n");

	//ndp_cycle_count_t t2 = 0;


	while(LIKELY(keep_running))
	{
		//core.pseudo_ts = core_read_tsc();

		check_global_comm_ring();
		check_comm_rings();

		/*
		if(LIKELY(t2))
		{
			ndp_cycle_count_t diff = ndp_rdtsc() - t2;
			if(diff > core.maxxx)
			{
				//printf("new maxxx %lu \n", diff);
				core.maxxx = diff;
			}
		}*/

		check_tx();

		//t2 = ndp_rdtsc();

		check_rx();

		#if ! NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
			check_pullq_in_core_loop();
		#endif
	}

	#if NDP_CORE_RECORD_HW_TS_ANY
		LOG_FMSG(LOGLVL10, "hw ts poi sent to nic: %"PRIu64, core.rec_hw_ts_num_poi_sent_to_nic);
	#endif


	#if NDP_CORE_RECORD_SW_BUF_SIZE

		LOG_FMSG(LOGLVL10, "record_sw_buf: %"PRIu64" front entries (index %"PRIu64"), %"PRIu64" back entries (index %"PRIu64")",
				core.record_sw_buf.num_front_entries, core.record_sw_buf.front_index,
				core.record_sw_buf.num_back_entries, core.record_sw_buf.back_index);

		char buf[300];
		core_output_file_name(buf, 300, "/core_rec_sw");


		if(core.record_sw_buf.num_front_entries + core.record_sw_buf.num_back_entries)
		{
			LOG_FMSG(LOGLVL10, "writing records to %s", buf);

			FILE * f = fopen(buf, "wb");

			if(!f)
				LOG_FMSG(LOGLVL10, "%s -> %s fopen error\n", __func__, buf)
			else
			{
				ndp_cycle_count_t tsc_hz = core.tsc_hz;
				if(!fwrite(&tsc_hz, sizeof(tsc_hz), 1, f))
					LOG_FMSG(LOGLVL10, "%s -> %s could not write tsc_hz", __func__, buf);

				write_two_headed_buffer_to_file(&core.record_sw_buf, f);
			}
		}

	#endif


	#if NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
		core.keep_running_pullq_core = 0;

		while(!core.pullq_core_finished)
			SPIN_BODY();
	#endif

	///*
	struct rte_eth_stats eth_stats;

	if(rte_eth_stats_get(core.dpdk_port_id, &eth_stats))
		rte_exit(1, "rte_eth_stats_get error\n");

	printf(	"eth_stats ipackets %lu ierrors %lu imissed %lu rx_nombuf %lu \n"
			"          opackets %lu oerrors %lu\n", eth_stats.ipackets, eth_stats.ierrors,
			eth_stats.imissed, eth_stats.rx_nombuf, eth_stats.opackets, eth_stats.oerrors);
	//*/

	rte_mempool_free(core.mbuf_pool);


	#ifdef NDP_BUILDING_FOR_XEN
		//TODO check for return code?
		xengnttab_close(core.gh);
	#endif
}

