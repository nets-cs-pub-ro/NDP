#include "../common/logger.h"
#include <arpa/inet.h>
#include "defs.h"
#include "helpers.h"
#include <netinet/in.h>

#include <stdio.h>

//the remote address better be an IPv4 one for now :-s
int ndp_connect(const char* remote_addr, unsigned int remote_port)
{
	struct in_addr aux_in_addr;
	if(!inet_aton(remote_addr, &aux_in_addr))
		LOG_FMSG_DO(LOGLVL1, return -11, "%s -> ndp_connect -> !inet_aton(remote_addr, &aux_in_addr)", __func__);

	struct lib_socket_descriptor *d = get_available_socket();
	if(!d)
		LOG_FMSG_DO(LOGLVL2, return -1, "%s -> cannot get_available_socket()", __func__);

	int err;

	//the core will set the src addr & port
	d->socket_info.socket->tuple.remote_addr = aux_in_addr.s_addr;
	d->socket_info.socket->tuple.remote_port = ndp_port_hton(remote_port);

	if(UNLIKELY (!comm_ring_write(NDP_INSTANCE_OP_CONNECT, d->socket_info.index)))
		LOG_FMSG_DO(LOGLVL2, err = -2; goto reclaim_socket, "%s -> !comm_ring_write(NDP_INSTANCE_OP_CONNECT, d->index)", __func__);

	/* wait for core to confirm the connect request; at this point we are sure that the missing
	tuple information is available */
	//TODO another easy way out via busy waiting
	volatile struct ndp_socket_flags *flags = &d->socket_info.socket->flags;
	while(!flags->core_connect_acknowledged)
		SPIN_BODY();

	if(UNLIKELY (prepare_connect_socket(d)))
		LOG_FMSG_DO(LOGLVL2, err = -3; goto reclaim_socket, "%s -> set_mac_addrs error", __func__);

	return d->socket_info.index;

reclaim_socket:
	//TODO reclaim socket descriptor here
	return err;
}

