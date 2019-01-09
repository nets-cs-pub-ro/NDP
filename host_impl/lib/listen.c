#include "../common/logger.h"
#include "../common/ring.h"
#include <arpa/inet.h>
#include "defs.h"
#include "helpers.h"

int ndp_listen(ndp_port_number_t port, int backlog)
{
	struct lib_socket_descriptor *d = get_available_socket();
	if(!d)
		LOG_FMSG_DO(LOGLVL2, return -1, "%s -> cannot get_available_socket", __func__);

	/*For listen sockets, we repurpose the memory starting with the tx ring to accommodate one big ring
	that should have enough space to hold socket indices for accepting incoming connections.
	As this ring is used, data is going to be written all over the other rings (including meta data, like
	the buf_size) and packet buffers. Everything is reverted back to normal when the socket is reclaimed
	and reset (most likely in the get_available_socket function).*/

	struct shm_parameters *p = &lib.instance_info.shm_params;

	ring_size_t mega_ring_size = p->num_tx_ring_elements 		* sizeof(ndp_packet_buffer_index_t) +
								 p->num_rtx_ring_elements   	* sizeof(ndp_packet_buffer_index_t) +
								 p->num_to_rtx_ring_elements   	* sizeof(ndp_packet_buffer_index_t) +
								 p->num_ack_tx_ring_elements	* sizeof(struct ndp_packet_headers) +
								 p->num_rx_ring_elements		* sizeof(ndp_packet_buffer_index_t) +
								 p->num_ack_rx_ring_elements	* sizeof(struct ndp_header) +
								 p->packet_buffer_mtu * (p->num_tx_packet_buffers + p->num_rx_packet_buffers);

	//TODO this is probably safe to do (with respect to the amount of space available), but better be sure at some point
	RING_INIT(d->socket_info.listen_ring, mega_ring_size / sizeof(ndp_socket_index_t));

	d->listen_consumer_shadow = 0;


	//struct ndp_connection_tuple *tuple = &sd->info.start->tuple;


	//the core will add the local address
	d->socket_info.socket->tuple.local_port = ndp_port_hton(port);


	//sd->info.start->flags.lib_is_listening = 1;
	//sd->info.start->flags.lib_in_use = 1;


	//getting sockets for the backlog, one by one
	int i;
	for (i = 0; i < backlog; i++)
	{
		struct lib_socket_descriptor *x = get_available_socket();

		if(!x)
			LOG_FMSG_DO(LOGLVL2, break, "%s -> could only find %d little sockets instead of %d", __func__, i, backlog);

		//if(!produce_at_most(d->socket_info.tx_ring, &x->socket_info.index, sizeof(ndp_socket_index_t), 1))
		//	LOG_FMSG_DO(LOGLVL2, break, "%s -> no more produce room after %d little sockets", __func__, i);

		if(LIKELY (PRODUCER_AVAILABLE(d->socket_info.listen_ring)))
			PRODUCE(d->socket_info.listen_ring, x->socket_info.index)
		else
			LOG_FMSG_DO(LOGLVL2, break, "%s -> no more produce room after %d little sockets", __func__, i);


		//VOLATILE(&x->socket_info.socket->tuple)->local_port = ndp_port_hton(port);
		//info.start->flags.lib_in_use = 1;
	}

	//TODO also, if i = 0 at this point, it's pretty much an error too

	if(!comm_ring_write(NDP_INSTANCE_OP_LISTEN, d->socket_info.index))
	{
		LOG_FMSG(LOGLVL1, "%s -> !comm_ring_write(NDP_INSTANCE_OP_LISTEN, d->socket_info.index)", __func__);
		//TODO should also reclaim sockets
		return -1;
	}

	d->is_listening = 1;

	return d->socket_info.index;
}
