#include "../common/logger.h"
#include "../common/ring.h"
#include <arpa/inet.h>
#include "helpers.h"
#include "recv_aux.h"
#include <time.h>
#include "timeout.h"


void* timeout_thread_function(void *arg)
{
	struct timespec sleep_req;
	sleep_req.tv_sec = NDP_TIMEOUT_THREAD_SLEEP_NS / NS_PER_SECOND;
	sleep_req.tv_nsec = NDP_TIMEOUT_THREAD_SLEEP_NS % NS_PER_SECOND;

	while(lib.timeout_thread_may_run)
	{
		nanosleep(&sleep_req, NULL);

		for(unsigned int i = 0; i < NDP_LIB_MAX_SOCKETS; i++)
		{
			struct lib_socket_descriptor *d = &lib.socket_descriptors[i];

			if(!MY_LOCK_ACQUIRE_ONE_TRY(d->recv_lock))
				goto check_send_stuff;

			if(!d->can_send_recv)
				goto unlock_recv_and_continue;

			ring_size_t new_data_packets = check_for_incoming_data(d, 0);

			if(new_data_packets)
				send_pending_acks(d, new_data_packets, 0);

		unlock_recv_and_continue:
			MY_LOCK_RELEASE(d->recv_lock);

		check_send_stuff:
			if(!MY_LOCK_ACQUIRE_ONE_TRY(d->send_lock))
				continue;

			if(!d->can_send_recv)
				goto unlock_send_and_continue;

			check_acks(d, 0);

			//ndp_timestamp_ns_t pseudo_now = lib_get_ts();

			check_for_timeouts(d, 0);

		unlock_send_and_continue:
			MY_LOCK_RELEASE(d->send_lock);
		}
	}

	return NULL;
}

