#include "../common/logger.h"
#include "defs.h"
#include "helpers.h"
#include "hoover_ndp.h"
#include <unistd.h>

//TODO this is just a dummy version of close that only works for active sockets
int ndp_close(int sock)
{
	struct lib_socket_descriptor *d = &lib.socket_descriptors[sock];

	int result = 0;

	if(UNLIKELY (!d->can_send_recv))
	{
		LOG_FMSG(LOGLVL3, "%s -> socket %d is not active", __func__, sock);
		return -1;
	}

	sleep(2);

	reset_lib_socket_descriptor(d);

	MY_LOCK_ACQUIRE(lib.ndp_close_lock);


	if(UNLIKELY (!PRODUCER_AVAILABLE(lib.av_sock_ring)))
	{
		LOG_FMSG(LOGLVL1, "%s -> !PRODUCER_AVAILABLE(lib_data.av_sock_ring)", __func__);
		result = -1;
	}
	else
		PRODUCE(lib.av_sock_ring, d->socket_info.index);

	if(UNLIKELY (!comm_ring_write(NDP_INSTANCE_OP_CLOSE, d->socket_info.index)))
	{
		LOG_FMSG(LOGLVL2, "%s -> !comm_ring_write(NDP_INSTANCE_OP_CLOSE, d->index)", __func__);
		result = -2;
	}


	MY_LOCK_RELEASE(lib.ndp_close_lock);

	return result;


}

