#include "defs.h"
#include <errno.h>
#include "functions.h"
#include "helpers.h"
#include <signal.h>
#include <stdio.h>

struct ndp_lib_data lib;

struct lib_stupid_counters lctrs;



static void ctrlc_handler(int x __attribute__((unused)))
{
	printf("Ctrl-C pressed ... will exit\n");

	#if NDP_LIB_RECORD_SW_BUF_SIZE

		LOG_FMSG(LOGLVL10, "record_sw_buf: %"PRIu64" front entries (index %"PRIu64"), %"PRIu64" back entries (index %"PRIu64")",
				lib.record_sw_buf.num_front_entries, lib.record_sw_buf.front_index,
				lib.record_sw_buf.num_back_entries, lib.record_sw_buf.back_index);

		char buf[300];
		lib_output_file_name(buf, 300, "/lib_rec_sw");

		if(lib.record_sw_buf.num_front_entries + lib.record_sw_buf.num_back_entries)
		{
			LOG_FMSG(LOGLVL10, "writing records to %s", buf);

			FILE * f = fopen(buf, "wb");

			if(!f)
				LOG_FMSG(LOGLVL10, "%s -> %s fopen error\n", __func__, buf)
			else
			{
				ndp_cycle_count_t tsc_hz = lib.instance_info.shm_begins->tsc_hz;

				if(!fwrite(&tsc_hz, sizeof(tsc_hz), 1, f))
					LOG_FMSG(LOGLVL10, "%s -> %s could not write tsc_hz", __func__, buf);

				write_two_headed_buffer_to_file(&lib.record_sw_buf, f);
			}
		}

	#endif

	exit(1);
}

/*
#include "hoover_ndp.h"
#include <unistd.h>
static int f_dummy()
{
	int x = ndp_init();
	if(x)
	{
		printf("ndp init error %d\n", x);
		return -1;
	}

	printf("zzz\n");
	sleep(3);

	ndp_terminate();

	return 0;
}
*/


int main(int argc, char ** argv)
{
	#if ! __x86_64__
	exit_msg(1, "this probably only works on x86_64 for now (ask me why that is)");
	#endif

	if(signal(SIGINT, ctrlc_handler) == SIG_ERR)
		LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> signal error: %s", __func__, strerror(errno));

	int result = -1;

	result = f_ndp_ping_pong(argc, argv);

	//result = f_ndp_incast(argc, argv);
	//result = f_ndp_incast_single_thread(argc, argv);
	//result = f_ndp_incast_single_thread2(argc, argv);
	//result = f_ndp_prio(argc, argv);

	//result = f_complex(argc, argv);

	//result = f_ndp_incast2(argc, argv);
	//result = f_ndp_basic_client_server(argc, argv);

	//result = f_dummy();

	return result;
}
