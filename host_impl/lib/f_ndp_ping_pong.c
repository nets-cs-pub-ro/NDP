#include "defs.h"
#include <errno.h>
#include "f_helpers.h"
#include "functions.h"
#include "helpers.h"
#include "hoover_ndp.h"
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "timeout.h"
#include <unistd.h>

//#define CLIENT_SENDS		(2 * 1000 * 1000 * 1000)
//#define CLIENT_SENDS		(2 * 1000 * 1000 * 1000)
#define CLIENT_SENDS		1000

//#define SERVER_SENDS		1000
#define SERVER_SENDS		(1000)


#define CLIENT_INIT_MEMORY_FIRST		1
#define SERVER_INIT_MEMORY_FIRST		1


#if NDP_CURRENT_TEST_ENVIRONMENT == ENV_CAM
	char *destination_server = "10.0.0.3";
#elif NDP_CURRENT_TEST_ENVIRONMENT == ENV_COCOS
	char *destination_server = "10.0.0.6";
#elif NDP_CURRENT_TEST_ENVIRONMENT == ENV_GAINA
	char *destination_server = "10.0.0.7";
#else
	zzzz
#endif


//char bufA[CLIENT_SENDS];
//char bufB[SERVER_SENDS];

static volatile int keep_running = 1;

static void ctrlc_handler(int x __attribute__((unused)))
{
//LOG_MSG(LOGLVL1, "ctrl-c pressed!");
	keep_running = 0;
}

static unsigned int test_sum(char *p, size_t n)
{
	int s = 0;
	for(int i = 0; i < n; i++)
		s += i * p[i];
	return s;
}


//*********

int f_ndp_ping_pong(int argc, char ** argv)
{
	if(argc != 2)
	{
		printf("0|1|2|... ?\n");
		exit(1);
	}

	//uint64_t t1 = lib_read_tsc();
	int init_result = ndp_init();
	//uint64_t t2 = lib_read_tsc();

	if(init_result)
		exit_msg(1, "ndp_init error");

	printf("ndp_init complete\n");

	//****


	int id = atoi(argv[1]);

	printf("hi %d!\n", id);

	size_t client_sends = CLIENT_SENDS;
	size_t server_sends = SERVER_SENDS;

	ssize_t n1, n2;

	char *buf1 = malloc(client_sends);
	char *buf2 = malloc(server_sends);
	//char *buf1 = bufA;
	//char *buf2 = bufB;

	if(!buf1 || !buf2)
	{
		printf("malloc error\n");
		exit(1);
	}

	if(id % 2 == 0)
	{
		#if CLIENT_INIT_MEMORY_FIRST
			for(int i = 0; i < client_sends; i++)
				buf1[i] = i;
		#endif

		int sleep_just_in_case_for = 2;

		//printf("sleeping %ds...\n", sleep_just_in_case_for);

		for(int JJ = 0; JJ < 10; JJ++)
		{
			sleep(sleep_just_in_case_for);

			uint64_t t3 = ndp_read_start_cycles();

			int s = ndp_connect(destination_server, 1000 + id / 2);

			//printf("connected; socket = %d\n", s);

			n1 = ndp_send_all(s, buf1, client_sends);

			//uint64_t t4 = lib_read_tsc();

			n2 = ndp_recv_all(s, buf2, server_sends);

			uint64_t t5 = ndp_read_end_cycles();

			double tsc_hz = lib.instance_info.shm_begins->tsc_hz;

			/*
			printf("sent %ld! sum is %d ; dT1 %"PRIu64" dT2 %"PRIu64" dT3 %"PRIu64"\n", x, Sum,
					tsc_delta_to_us(t2 - t1, tsc_hz),
					tsc_delta_to_us(t4 - t3, tsc_hz),
					tsc_delta_to_us(t5 - t3, tsc_hz));*/

			if(n1 != client_sends || n2 != server_sends)
			{
				printf("n1 || n2 error\n");
				exit(1);
			}

			//unsigned int Sum = test_sum(buf1, client_sends);

			//printf("sent %ld! sum is %u \n", n1, Sum);
			fprintf(stderr, "%d %lu\n", JJ, tsc_delta_to_us(t5 - t3, tsc_hz));

			ndp_close(s);
		}
	}
	else
	{
		if(signal(SIGINT, ctrlc_handler) == SIG_ERR)
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> signal error: %s", __func__, strerror(errno));

		#if SERVER_INIT_MEMORY_FIRST
			for(int i = 0; i < client_sends; i++)
				buf1[i] = 0;
		#endif

		int S = ndp_listen(1000 + id / 2, 10);

		printf("listening; socket = %d\n", S);

		uint64_t counter = 0;

		while(keep_running)
		{
			//keep_running = 0;

			int s = ndp_accept(S, 0);

			n1 = ndp_recv_all(s, buf1, client_sends);

			//uint64_t t4 = lib_read_tsc();

			n2 = ndp_send_all(s, buf2, server_sends);

			if(n1 != client_sends || n2 != server_sends)
			{
				printf("n1 || n2 error\n");
				exit(1);
			}

			counter++;

			//printf("done rcvd %ld! sum is %d delta %"PRIu64"\n", x, test_sum(), (t4 - t3) / 1000);
			printf("done rcvd %ld! counter is %lu sum is %u \n", n1, counter, test_sum(buf1, client_sends));

			ndp_close(s);
		}
	}

	//int sleep_seconds = 1;
	//printf("sleeping for %d second(s) ...\n", sleep_seconds);
	//sleep(sleep_seconds);


	if(ndp_terminate())
		printf("ndp_terminate != 0\n");


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

	return 0;
}
