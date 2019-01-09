#include "helpers.h"
#include <rte_cycles.h>
#include <rte_ethdev.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

//#if NDP_CORE_RECORD_HW_TS_ANY || NDP_CORE_RECORD_SW_ANY
	/*
	static void write_aux(const char *name, const void *from, size_t unit_size, size_t num_units)
	{
		char core_cwd[1000];

		#define OUT_NAME_SIZE	1000
		char out_name[OUT_NAME_SIZE];

		if(strlen(name) >= OUT_NAME_SIZE - 10)
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> name too long", __func__);

		//TODO this format pretty much assumes local_addr is IPv4
		sprintf(out_name, "rec_%s_%08x.dat", name, core_data.local_addr);

		if(!getcwd(core_cwd, sizeof(core_cwd)))
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> getcwd error", __func__);

		LOG_FMSG(LOGLVL1, "writing recorded %s data to %s/%s ...", name, core_cwd, out_name);

		FILE *f = fopen(out_name, "wb");

		if(!f)
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> %s fopen error", __func__, name);

		if(fwrite(from, unit_size, num_units, f) != num_units)
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> %s fwrite != sz", __func__, name);

		fclose(f);
	}*/

/*
	static FILE* get_file(const char *data_name)
	{
		#define NAME_SIZE	1000

		char core_cwd[NAME_SIZE];
		char out_name[NAME_SIZE];

		if(!getcwd(core_cwd, sizeof(core_cwd)))
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> getcwd error", __func__);

		if(strlen(data_name) + strlen(core_cwd) >= NAME_SIZE - 50)
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> name too long", __func__);

		//TODO this format pretty much assumes local_addr is IPv4
		sprintf(out_name, "rec_%s_%08x.dat", data_name, core.local_addr);

		FILE *f = fopen(out_name, "wb");

		if(!f)
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> %s fopen error", __func__, data_name);

		return f;
	}

	static void write2file(FILE *f, const void *from, size_t unit_size, size_t num_units)
	{
		if(fwrite(from, unit_size, num_units, f) != num_units)
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> fwrite != sz", __func__);
	}

	void record_write_results(void)
	{
		FILE *f;

		#if NDP_CORE_RECORD_HW_TS_TX_MAX
			f = get_file("hw_tx");
			write2file(f, core.rec_hw_ts_tx, sizeof(ndp_timestamp_ns_t), core.rec_hw_ts_tx_idx);
			fclose(f);

			LOG_FMSG(LOGLVL10, "recorder hw tx idx: %zu", core.rec_hw_ts_tx_idx);
		#endif

		#if NDP_CORE_RECORD_HW_TS_RX_MAX
			f = get_file("hw_rx");
			write2file(f, core.rec_hw_ts_rx, sizeof(ndp_timestamp_ns_t), core.rec_hw_ts_rx_idx);
			fclose(f);

			LOG_FMSG(LOGLVL10, "recorder hw rx idx: %zu", core.rec_hw_ts_rx_idx);
		#endif


		f = get_file("core");
		write2file(f, log_data.record1_entries, sizeof(struct log_record1), log_data.next_record1_index);
		log_msg("recorded %lu entries", log_data.next_record1_index);
	}
*/
//#endif


#if NDP_CORE_RECORD_HW_TS_ANY

	int record_hw_ts_thread_func(void *param __attribute__((unused)))
	{
		struct timespec t;
		uint64_t t_ns;

		while(1)
		{

		#if NDP_CORE_RECORD_HW_TS_TX_MAX

			int result = rte_eth_timesync_read_tx_timestamp(core.dpdk_port_id, &t);

			if(result == -EINVAL)
				continue;

			//LOG_MSG(LOGLVL1, "tststssstsss");

			if(UNLIKELY(result))
				LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> rte_eth_timesync_read_tx_timestamp err %d", __func__, result);

			t_ns = t.tv_sec * (1000 * 1000 * 1000) + t.tv_nsec;

			if(LIKELY(core.rec_hw_ts_tx_idx < NDP_CORE_RECORD_HW_TS_TX_MAX))
				core.rec_hw_ts_tx[core.rec_hw_ts_tx_idx++] = t_ns;
		#endif
		}
	}
#endif

