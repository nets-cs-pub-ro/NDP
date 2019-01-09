#include "../common/defs.h"
#include "../common/logger.h"
#include "helpers.h"
#include "hoover_ndp.h"


int ndp_accept(int sock, int flags)
{
	struct lib_socket_descriptor *d = &lib.socket_descriptors[sock];

	if(!d->is_listening)
		LOG_FMSG_DO(LOGLVL2, return -1, "%s -> socket %d is not listening", __func__, sock);

	struct RING_BUF(ndp_socket_index_t) *r = d->socket_info.listen_ring;

	ndp_socket_index_t idx;

	//block until a connection arrives
	//TODO busy waiting again
	while(1)
	{
		//TODO hmm?
		PLS_DONT_OPTIMIZE_AWAY_MY_READS();

		ring_size_t p_cci = GET_CONSUMER_INDEX(r);

		if(p_cci != d->listen_consumer_shadow)
		{
			//RING_ARRAY_READ_FROM(r, ndp_socket_index_t, d->listen_consumer_shadow, 0, &idx);
			idx = GET_ELEMENT_AT(r, d->listen_consumer_shadow);


			//d->listen_consumer_shadow = ring_wrap_index(r, d->listen_consumer_shadow + sizeof(idx));
			d->listen_consumer_shadow = RING_WRAP_INDEX(r, d->listen_consumer_shadow + 1);

			//attempt to find another available socket for the listen backlog
			struct lib_socket_descriptor *x = get_available_socket();

			if(x)
			{
				//shouldn't happen
				if(LIKELY (PRODUCER_AVAILABLE(r)))
					//produce_at_most(r, &x->socket_info.index, sizeof(ndp_socket_index_t), 1));
					PRODUCE_RELEASE(r, x->socket_info.index)
				else
					LOG_FMSG(LOGLVL2, "%s -> couldn't add replacement socket to tx ring", __func__);
			}
			else
			{
				LOG_FMSG(LOGLVL3, "%s -> couldn't find replacement socket", __func__);
			}



			struct lib_socket_descriptor *new = &lib.socket_descriptors[idx];

			volatile struct ndp_socket_flags *f = &new->socket_info.socket->flags;
			//TODO busy waiting for a flag, but this should actually take very little time so it might be ok
			while(!f->core_connection_can_be_accepted)
				;

			//TODO should also reclaim the socket on error
			if(prepare_accept_socket(new))
				LOG_FMSG_DO(LOGLVL2, return -1, "%s -> set_mac_addrs error", __func__);

			//LOG_FMSG(LOGLVL102, "%s -> accepted connection; sock = %d", __func__, d->socket_info.index);

			return new->socket_info.index;
		}
		else if(flags & NDP_ACCEPT_DONT_BLOCK)
			return NDP_ACCEPT_ERR_NONE_AVAILABLE;
		//else
			//LOG_MSG(LOGLVL102, "zzzzzzz");
	}
}
