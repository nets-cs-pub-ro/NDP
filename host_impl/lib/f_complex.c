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



//!!!!!!!
#define MASTER_ID					0
//!!!!!!!



#define MAX_NUM_SERVERS				8
#define LISTEN_PORT					5678


#define MAX_SERVER_CONNECTIONS			3
#define MAX_NUM_SR						10


#define BUF_SIZE					(10 * 1024 * 1024)



#define ONE_G						(1000 * 1000 * 1000)


typedef uint32_t msg_t;
static msg_t MSG_GOGOGO		=		1;




static double tsc_hz;
static int listen_sock;


//for master
static int control_socks[MAX_NUM_SERVERS];

//for others
static int control_sock;


ndp_cycle_count_t cycles[MAX_SERVER_CONNECTIONS * MAX_NUM_SR];
int num_cycles;


struct snr_result
{
	ndp_cycle_count_t cycles[MAX_SERVER_CONNECTIONS * MAX_NUM_SR];
	int send_socks[MAX_SERVER_CONNECTIONS];
	int recv_socks[MAX_SERVER_CONNECTIONS];
};


struct snr_result send_and_recv(const char **send_to, int num_send_to, size_t send_size, int num_sends,
										  int listen_sock, int num_accepts, size_t recv_size, int num_recvs)
{
	char *buf = malloc(BUF_SIZE);

	for(int i = 0; i < BUF_SIZE; i++)
		buf[i] = i % 147;

	//todo WHAT ???!?!?! ^ v ^ v
	//memset(buf, 0, BUF_SIZE);

	struct snr_result r;


	size_t sent[MAX_SERVER_CONNECTIONS];
	memset(sent, 0, sizeof(sent));


	size_t rcvd[MAX_SERVER_CONNECTIONS];
	memset(rcvd, 0, sizeof(rcvd));


	int done_sent[MAX_SERVER_CONNECTIONS];
	memset(done_sent, 0, sizeof(done_sent));


	int done_rcvd[MAX_SERVER_CONNECTIONS];
	memset(done_rcvd, 0, sizeof(done_rcvd));


	size_t min;
	ssize_t n;
	msg_t msg;


	for(int i = 0; i < num_send_to; i++)
	{
		r.send_socks[i] = ndp_connect(send_to[i], LISTEN_PORT);
		if(UNLIKELY (r.send_socks[i] < 0))
			exit_msg(1, "send_sock < 0");
	}

	for(int i = 0; i < num_accepts; i++)
		r.recv_socks[i] = -1;

	for(int i = 0; i < num_send_to; i++)
		r.cycles[i * num_sends] = ndp_rdtsc();


	int total_left = num_send_to * num_sends +  num_accepts * num_recvs;


	while(total_left)
	{
		for(int i = 0; i < num_send_to; i++)
		{
			if(done_sent[i] == num_sends)
				continue;

			if(sent[i] == send_size)
			{
				n = ndp_recv(r.send_socks[i], &msg, sizeof(msg), NDP_RECV_DONT_BLOCK);

				if(n)
				{
					ndp_cycle_count_t *cycles = &r.cycles[i * num_sends + done_sent[i]];
					*cycles = ndp_rdtsc() - *cycles;

					done_sent[i]++;
					total_left--;
					sent[i] = 0;

					//printf("hmm\n");

					if(done_sent[i] < num_sends)
						r.cycles[i * num_sends + done_sent[i]] = ndp_rdtsc();
					else
						continue;
				}
				else
					continue;
			}

			size_t left = send_size - sent[i];
			min = left > BUF_SIZE ? BUF_SIZE : left;

			n = ndp_send(r.send_socks[i], buf, min, NDP_SEND_DONT_BLOCK);
			sent[i] += n;
		}

		for(int i = 0; i < num_accepts; i++)
		{
			if(done_rcvd[i] == num_recvs)
				continue;

			if(r.recv_socks[i] < 0)
			{
				r.recv_socks[i] = ndp_accept(listen_sock, NDP_ACCEPT_DONT_BLOCK);

				if(r.recv_socks[i] < 0  && r.recv_socks[i] != NDP_ACCEPT_ERR_NONE_AVAILABLE)
					exit_msg(1, "accept error");

				continue;
			}

			size_t left = recv_size - rcvd[i];
			min = left > BUF_SIZE ? BUF_SIZE : left;

			n = ndp_recv(r.recv_socks[i], buf, min, NDP_RECV_DONT_BLOCK);
			rcvd[i] += n;

			if(rcvd[i] == recv_size)
			{
				n = ndp_send(r.recv_socks[i], &msg, sizeof(msg), NDP_SEND_DONT_BLOCK);
				if(n != sizeof(msg))
					exit_msg(1, "send after recv error");

				//printf("zzz\n");

				done_rcvd[i]++;
				total_left--;
				rcvd[i] = 0;
			}
		}
	}

	//printf("laa\n");

	return r;
}


void do_stuff(int id)
{

	int senders[] = {0, 1};
	int receivers[] = {4, 5};


	//***********************

	int num_send_to = 0;
	size_t send_size = (128 * 1024 * 1024);
	int num_sends = 8;

	int num_accepts = 0;
	size_t recv_size = send_size;
	int num_recvs = num_sends;


	for(int i = 0; i < sizeof(senders) / sizeof(int); i++)
		if(id == senders[i])
			num_send_to++;

	for(int i = 0; i < sizeof(receivers) / sizeof(int); i++)
		if(id == receivers[i])
			num_accepts++;

	struct snr_result r;

	if(id == 0)
	{
		const char *send_to[] = {"10.0.0.5"};
		r = send_and_recv(send_to, num_send_to, send_size, num_sends, listen_sock, num_accepts, recv_size, num_recvs);
	}
	else if(id == 1)
	{
		const char *send_to[] = {"10.0.0.6"};
		r = send_and_recv(send_to, num_send_to, send_size, num_sends, listen_sock, num_accepts, recv_size, num_recvs);
	}
	else if(id == 2)
	{
		const char *send_to[] = {"10.0.0.7"};
		r = send_and_recv(send_to, num_send_to, send_size, num_sends, listen_sock, num_accepts, recv_size, num_recvs);
	}
	else if(id == 3)
	{
		const char *send_to[] = {"10.0.0.8"};
		r = send_and_recv(send_to, num_send_to, send_size, num_sends, listen_sock, num_accepts, recv_size, num_recvs);
	}
	else if(id == 4)
	{
		const char *send_to[] = {"10.0.0.1"};
		r = send_and_recv(send_to, num_send_to, send_size, num_sends, listen_sock, num_accepts, recv_size, num_recvs);
	}
	else if(id == 5)
	{
		const char *send_to[] = {"10.0.0.2"};
		r = send_and_recv(send_to, num_send_to, send_size, num_sends, listen_sock, num_accepts, recv_size, num_recvs);
	}
	else if(id == 6)
	{
		const char *send_to[] = {"10.0.0.3"};
		r = send_and_recv(send_to, num_send_to, send_size, num_sends, listen_sock, num_accepts, recv_size, num_recvs);
	}
	else if(id == 7)
	{
		const char *send_to[] = {"10.0.0.4"};
		r = send_and_recv(send_to, num_send_to, send_size, num_sends, listen_sock, num_accepts, recv_size, num_recvs);
	}
	else
		exit_msg(1, "asdasdasldas;dla");

	num_cycles = num_send_to * num_sends;

	if(id != MASTER_ID)
	{
		ndp_send_all(control_sock, (char*)&num_cycles, sizeof(num_cycles));

		if(num_cycles)
			ndp_send_all(control_sock, (char*)r.cycles , sizeof(ndp_cycle_count_t) * num_cycles);

		msg_t msg;
		ndp_recv(control_sock, &msg, sizeof(msg), 0);
	}
	else
		memcpy(cycles, r.cycles, sizeof(cycles));


	//ndp_close(sock);
}



static void* thread_close_function(void *param)
{
	int sock = *(int*)param;
	ndp_close(sock);

	return NULL;
}


int f_complex(int argc, char ** argv)
{
	if(argc < 2)
		exit_msg(1, "id?");

	int id = atoi(argv[1]);

	if(id == MASTER_ID && argc < 3)
		exit_msg(1, "other workers?");

	if(ndp_init())
		exit_msg(1, "ndp_init failed");

	listen_sock = ndp_listen(LISTEN_PORT, MAX_NUM_SERVERS - 1);

	if(UNLIKELY (listen_sock < 0))
		exit_msg(1, "ndp_listen error");

	if(id == MASTER_ID)
	{
		tsc_hz = ndp_estimate_tsc_hz(2);

		const char *workers[MAX_NUM_SERVERS];
		int num_workers = 0;

		for(int i = 2; i < argc; i++)
			workers[num_workers++] = argv[i];

		for(int i = 0; i < num_workers; i++)
		{
			control_socks[i] = ndp_connect(workers[i], LISTEN_PORT);
			if(UNLIKELY (control_socks[i]) < 0)
				exit_msg(1, "master connect_socks < 0");
		}

		for(int i = num_workers - 1; i >= 0; i--)
		{
			//printf("%d\n", control_socks[i]);
			ssize_t n = ndp_send_all(control_socks[i], (char*)&MSG_GOGOGO, sizeof(msg_t));
			if(UNLIKELY(n < 0))
				exit_msg(1, "initial n < 0");
		}

		//printf("SENT\n");

		do_stuff(MASTER_ID);

		//printf("DIDD\n");
		msg_t msg;

		//*********************************************************************************************************

		for(int i = 0; i < num_cycles; i++)
			printf("%lf ", cycles[i] / (tsc_hz / (1000 * 1000)));

		for(int i = 0; i < num_workers; i++)
		{
			ndp_recv_all(control_socks[i], (char*)&num_cycles, sizeof(num_cycles));

			if(num_cycles)
				ndp_recv_all(control_socks[i], (char*)cycles, num_cycles * sizeof(ndp_cycle_count_t));

			ndp_send(control_socks[i], &msg, sizeof(msg), 0);

			for(int i = 0; i < num_cycles; i++)
				printf("%lf ", cycles[i] / (tsc_hz / (1000 * 1000)));
		}

		printf("\n");

		pthread_t close_threads[MAX_NUM_SERVERS];

		for(int i = 0; i < num_workers; i++)
			if(UNLIKELY (pthread_create(&close_threads[i], NULL, thread_close_function, &control_socks[i])))
				exit_msg(1, "pthread_create error for close");

		for(int i = 0; i < num_workers; i++)
			if(pthread_join(close_threads[i], NULL))
				exit_msg(1, "pthread_join error for close");

	}
	else
	{
		control_sock = ndp_accept(control_sock, 0);

		if(UNLIKELY (control_sock < 0))
			exit_msg(1, "control_sock < 0");

		msg_t msg;

		/*ssize_t n = */ndp_recv_all(control_sock, (char*)&msg, sizeof(msg));

		if(UNLIKELY (msg != MSG_GOGOGO))
			exit_msg(1, "msg != MSG_GOGOGO");

		do_stuff(id);


		close(control_sock);
	}

	if(ndp_terminate())
		printf("ndp_terminate != 0\n");

	return 0;
}

/*

struct send_and_recv_result
{
	ndp_cycle_count_t cycles;
	int send_sock;
	int recv_sock;
};

struct send_and_recv_result send_and_recv(const char *send_to, size_t send_size, int listen_sock, size_t recv_size)
{
	char *buf = malloc(BUF_SIZE);

	for(int i = 0; i < BUF_SIZE; i++)
		buf[i] = i % 147;

	//todo WHAT ???!?!?! ^ v ^ v
	//memset(buf, 0, BUF_SIZE);

	ndp_cycle_count_t start = ndp_read_start_cycles();

	int send_sock = ndp_connect(send_to, LISTEN_PORT);
	if(UNLIKELY (send_sock < 0))
		exit_msg(1, "send_sock < 0");

	int recv_sock = -1;
	size_t min;
	ssize_t n;

	msg_t msg;

	while(send_size || recv_size)
	{
		if(send_size)
		{
			min = send_size > BUF_SIZE ? BUF_SIZE : send_size;
			n = ndp_send(send_sock, buf, min, NDP_SEND_DONT_BLOCK);
			send_size -= n;
		}

		if(recv_size)
		{
			if(recv_sock < 0)
			{
				recv_sock = ndp_accept(listen_sock, NDP_ACCEPT_DONT_BLOCK);
				if(UNLIKELY (recv_sock < 0 && recv_sock != NDP_ACCEPT_ERR_NONE_AVAILABLE))
					exit_msg(1, "recv sock < 0");
			}
			else
			{
				min = recv_size > BUF_SIZE ? BUF_SIZE : recv_size;
				n = ndp_recv(recv_sock, buf, min, NDP_RECV_DONT_BLOCK);
				recv_size -= n;
			}
		}
	}

	ndp_send_all(recv_sock, (char*)&msg, sizeof(msg));
	ndp_recv_all(send_sock, (char*)&msg, sizeof(msg));

	ndp_cycle_count_t total = ndp_read_end_cycles() - start;

	free(buf);

	struct send_and_recv_result result;
	result.cycles = total;
	result.send_sock = send_sock;
	result.recv_sock = recv_sock;

	return result;
}
*/

