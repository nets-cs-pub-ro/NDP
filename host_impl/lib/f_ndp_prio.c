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
#define RCV_FROM_WORKER_BUF_SIZE		(10 * 1024 * 1024)
#define WORKER_BUF_SIZE					(10 * 1024 * 1024)


#define PRIO_REPLY_SIZE		(700 * 1000 * 1000)





static char *worker_ips[MAX_NUM_WORKERS];

static size_t reply_size_for[MAX_NUM_WORKERS];

static ndp_cycle_count_t cycles[MAX_NUM_WORKERS];
static int connect_socks[MAX_NUM_WORKERS];
static char *bufs[MAX_NUM_WORKERS];
static size_t rcvd[MAX_NUM_WORKERS];





static void* thread_close_function(void *param)
{
	int sock = *(int*)param;
	ndp_close(sock);

	return NULL;
}


static void* prio_thread_function(void *param)
{
	const int pidx = 0;

	ssize_t n;


	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 400 * 1000 * 1000;
	nanosleep(&ts, NULL);


	ndp_cycle_count_t start = ndp_read_start_cycles();

	//connect_socks[pidx] =  ndp_connect(worker_ips[pidx], WORKER_SERVER_PORT);

	//if(UNLIKELY (connect_socks[pidx]) < 0)
		//exit_msg(1, "master connect_socks < 0");

	n = ndp_send_all(connect_socks[pidx], (char*)&reply_size_for[pidx], sizeof(reply_size_for[pidx]));
	if(UNLIKELY (n != sizeof(reply_size_for[pidx])))
		exit_msg(1, "n1 != sizeof(reply_size)");

	do
	{
		n = ndp_recv(connect_socks[pidx], bufs[pidx], RCV_FROM_WORKER_BUF_SIZE, NDP_RECV_DONT_BLOCK);
		rcvd[pidx] += n;

	} while(rcvd[pidx] < reply_size_for[pidx]);

	cycles[pidx] += ndp_read_end_cycles() - start;

	return NULL;
}



static int master_function(int argc, char ** argv)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	pthread_t current_thread = pthread_self();
	if(pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &mask))
		exit_msg(1, "cannot set current thread affinity");

	int i;
	int num_workers = 0;

	if (argc < 4)
		exit_msg(1, "0 <reply_size> <workers ...>");

	uint64_t reply_size = atoll(argv[2]);

	if(ndp_init())
		exit_msg(1, "ndp_init failed");

	double tsc_hz = ndp_estimate_tsc_hz(2);
	ndp_cycle_count_t start;

	for(i = 3; i < argc; i++)
	{
		worker_ips[num_workers] = argv[i];
		bufs[num_workers] = malloc(RCV_FROM_WORKER_BUF_SIZE);

		if(num_workers)
			reply_size_for[num_workers] = PRIO_REPLY_SIZE;
		else
			reply_size_for[num_workers] = reply_size;

		num_workers++;
	}

	//for(int J = 0; J < 20; J++)
	//{


	for(i = 0; i < num_workers; i++)
	{
		//cycles[i] = 0;
		memset(bufs[i], 0, RCV_FROM_WORKER_BUF_SIZE);
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

	pthread_t prio_thread;
	pthread_attr_t prio_thread_attr;

	CPU_ZERO(&mask);
	pthread_attr_init(&prio_thread_attr);
	CPU_SET(3, &mask);
	pthread_attr_setaffinity_np(&prio_thread_attr, sizeof(cpu_set_t), &mask);

	if(UNLIKELY (pthread_create(&prio_thread, &prio_thread_attr, prio_thread_function, NULL)) )
		exit_msg(1, "pthread_create error for prio_thread");



	ssize_t n;
	start = ndp_read_start_cycles();



	for(i = 1; i < num_workers; i++)
	{
		//TODO ndp_send is still blocking; this work only as long as we send less data than the sum of tx_buf sizes,
		//which should be the case here, because we're just sending a number
		n = ndp_send_all(connect_socks[i], (char*)&reply_size_for[i], sizeof(reply_size_for[i]));
		if(UNLIKELY (n != sizeof(reply_size_for[i])))
			exit_msg(1, "n1 != sizeof(reply_size)");
	}

	int done_receiving = 0;

	while(done_receiving < num_workers - 1)
	{
		for(i = 1; i < num_workers; i++)
		{
			if(rcvd[i] == reply_size_for[i])
				continue;

			char *buf = bufs[i];

			n = ndp_recv(connect_socks[i], buf, RCV_FROM_WORKER_BUF_SIZE, NDP_RECV_DONT_BLOCK);

			rcvd[i] += n;

			if(rcvd[i] == reply_size_for[i])
			{
				cycles[i] = ndp_rdtsc() - start;
				done_receiving++;

				//printf("done receiving from %d\n", i);
			}
		}
	}

	if(pthread_join(prio_thread, NULL))
		exit_msg(1, "pthread_join error for prio");

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

	double min, max;
	min = max = 0;

	for(i = 0; i < num_workers; i++)
	{
		double elapsed = cycles[i] / (tsc_hz / (1000 * 1000));
		printf("%lf ", elapsed);

		if(i == 1)
		{
			min = max = elapsed;
		}
		else if(i > 1)
		{
			if(min > elapsed)
				min = elapsed;
			else if(max < elapsed)
				max = elapsed;
		}
	}
	printf("%lu\n", (unsigned long)(max - min));


	//sleep(3);
	//}

	return 0;
}

static int worker_function(int argc, char ** argv)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(3, &mask);
	pthread_t current_thread = pthread_self();
	if(pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &mask))
		exit_msg(1, "cannot set current thread affinity");

	if(ndp_init())
		exit_msg(1, "ndp_init failed");

	int listenfd = ndp_listen(WORKER_SERVER_PORT, 10);

	if(UNLIKELY (listenfd < 0))
		exit_msg(1, "listen error");

	char *buf = malloc(WORKER_BUF_SIZE);

	//while(1)
	//{


	for(int i = 0; i < WORKER_BUF_SIZE; i++)
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
		size_t min = WORKER_BUF_SIZE < reply_size ? WORKER_BUF_SIZE : reply_size;

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


int f_ndp_prio(int argc, char **argv)
{
	if(argc < 2)
		exit_msg(1, "type?");

	int id = atoi(argv[1]);
	int result;

	if(id == 0)
		result = master_function(argc, argv);
	else
		result = worker_function(argc, argv);

	if(ndp_terminate())
		printf("ndp_terminate != 0\n");

	return result;
}



