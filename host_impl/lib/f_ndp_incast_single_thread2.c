#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>


#include "../common/helpers.h"
#include "../common/util.h"
#include "../common/tsc.h"
#include "f_helpers.h"
#include "functions.h"
#include "helpers.h"
#include "hoover_ndp.h"
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>



#define MAX_NUM_WORKERS 	8
#define WORKER_SERVER_PORT	5000
//#define RCV_FROM_WORKER_BUF_SIZE		(1024 * 1024)

#define BUF_SIZE					(512 * 1024)


static void* thread_close_function(void *param)
{
	int sock = *(int*)param;
	ndp_close(sock);

	return NULL;
}


static int master_function(int argc, char ** argv)
{
	char *worker_ips[MAX_NUM_WORKERS];

	size_t reply_size_for[MAX_NUM_WORKERS];

	ndp_cycle_count_t cycles[MAX_NUM_WORKERS];
	int connect_socks[MAX_NUM_WORKERS];
	char *bufs[MAX_NUM_WORKERS];
	size_t rcvd[MAX_NUM_WORKERS];

	int i;
	int num_workers = 0;

	if (argc < 4)
		exit_msg(1, "0 <reply_size> <workers ...>");

	uint64_t reply_size = atoll(argv[2]);

	if(ndp_init())
		exit_msg(1, "ndp_init failed");

	double tsc_hz = ndp_estimate_tsc_hz(2);
	ndp_cycle_count_t start, total;

	for(i = 3; i < argc; i++)
	{
		worker_ips[num_workers] = argv[i];
		bufs[num_workers] = malloc(BUF_SIZE);

		reply_size_for[num_workers] = reply_size;

		num_workers++;
	}

	//for(int J = 0; J < 20; J++)
	//{


	for(i = 0; i < num_workers; i++)
	{
		//cycles[i] = 0;
		for(unsigned j = 0; j < BUF_SIZE; j++)
			bufs[i][j] = j;

		rcvd[i] = 0;
	}



	for(i = 0; i < num_workers; i++)
	{
		ndp_cycle_count_t t1 = ndp_rdtsc();

		connect_socks[i] =  ndp_connect(worker_ips[i], WORKER_SERVER_PORT);
		if(UNLIKELY (connect_socks[i]) < 0)
			exit_msg(1, "master connect_socks < 0");

		cycles[i] = ndp_rdtsc() - t1;
	}

	start = ndp_read_start_cycles();

	ssize_t n;

	for(i = 0; i < num_workers; i++)
	{
		//TODO ndp_send is still blocking; this work only as long as we send less data than the sum of tx_buf sizes,
		//which should be the case here, because we're just sending a number
		n = ndp_send_all(connect_socks[i], (char*)&reply_size_for[i], sizeof(reply_size_for[i]));
		if(UNLIKELY (n != sizeof(reply_size_for[i])))
			exit_msg(1, "n1 != sizeof(reply_size)");
	}

	int done_receiving = 0;

	while(done_receiving < num_workers)
	{
		for(i = 0; i < num_workers; i++)
		{
			if(rcvd[i] == reply_size_for[i])
				continue;

			char *buf = bufs[i];

			n = ndp_recv(connect_socks[i], buf, BUF_SIZE, NDP_RECV_DONT_BLOCK);

			rcvd[i] += n;

			if(rcvd[i] == reply_size_for[i])
			{
				cycles[i] = ndp_rdtsc() - start;
				done_receiving++;

				//printf("done receiving from %d\n", i);
			}
		}
	}

	total = ndp_read_end_cycles() - start;

	//multiple threads just for close
	pthread_t threads[MAX_NUM_WORKERS];

	for(i = 0; i < num_workers; i++)
	{
		int x = pthread_create(&threads[i], NULL, thread_close_function, (void*)&connect_socks[i]);

		if(UNLIKELY (x))
			exit_msg(1, "pthread_create error for close");
	}

	for(i = 0 ; i < num_workers; i++)
	{
		if(pthread_join(threads[i], NULL))
			exit_msg(1, "pthread_join error for close");
	}

	double elapsed = total / (tsc_hz / (1000 * 1000));
	printf("%lf ", elapsed);

	for(i = 0; i < num_workers; i++)
	{
		elapsed = cycles[i] / (tsc_hz / (1000 * 1000));
		printf("%lf ", elapsed);
	}
	printf("\n");


	//sleep(3);
	//}

	return 0;
}

static int worker_function(int argc, char ** argv)
{
	if(ndp_init())
		exit_msg(1, "ndp_init failed");

	int listenfd = ndp_listen(WORKER_SERVER_PORT, 10);

	if(UNLIKELY (listenfd < 0))
		exit_msg(1, "listen error");

	char *buf = malloc(BUF_SIZE);

	//while(1)
	//{


	for(int i = 0; i < BUF_SIZE; i++)
		buf[i] = i;

	memset(&lctrs, 0, sizeof(struct lib_stupid_counters));

	int sock = ndp_accept(listenfd, 0);

	//printf("accept returned!\n");

	if(UNLIKELY (sock < 0))
		exit_msg(1, "accept error");

	size_t reply_size;
	ssize_t n;

	n = ndp_recv_all(sock, (char*)&reply_size, sizeof(reply_size));
	//printf("rcvd all!\n");

	if(UNLIKELY (n != sizeof(reply_size)))
		exit_msg(1, "recv_all n1 != sizeof(x)");


	while(reply_size)
	{
		size_t min = BUF_SIZE < reply_size ? BUF_SIZE : reply_size;

		n = ndp_send_all(sock, buf, min);

		if(UNLIKELY (n != min))
			exit_msg(1, "send_all n != min");

		reply_size -= n;
	}

	ndp_close(sock);

	print_stupid_counters();


	//}

	return 0;
}


int ac;
char ** av;


static void* thread_zzz_function(void *param)
{
	if(ac < 2)
		exit_msg(1, "type?");

	int id = atoi(av[1]);
	//int result;

	if(id == 0)
		master_function(ac, av);
	else
		worker_function(ac, av);

	return NULL;
}


int f_ndp_incast_single_thread2(int argc, char **argv)
{
	ac = argc;
	av = argv;

	cpu_set_t mask;
	pthread_t zzz_thread;
	pthread_attr_t zzz_thread_attr;

	CPU_ZERO(&mask);
	pthread_attr_init(&zzz_thread_attr);
	CPU_SET(3, &mask);
	pthread_attr_setaffinity_np(&zzz_thread_attr, sizeof(cpu_set_t), &mask);

	if(UNLIKELY (pthread_create(&zzz_thread, &zzz_thread_attr, thread_zzz_function, NULL)) )
		exit_msg(1, "pthread_create error for zzz_thread");

	pthread_join(zzz_thread, NULL);

	if(ndp_terminate())
		printf("ndp_terminate != 0\n");

	return 0;
}

