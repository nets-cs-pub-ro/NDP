#include <arpa/inet.h>
#include <errno.h>
#include "helpers.h"
#include "logger.h"
#include "mac_db.h"
#include "ring.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>



void exit_msg(int status, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
	exit(status);
}

void init_lib_instance_name(char *buf, pid_t pid)
{
	sprintf(buf,"/" LIB_INSTANCE_NAME_PREFIX "%d", pid);
}

static inline ndp_packet_buffer_size_t ndp_packet_buffer_size(ndp_packet_buffer_size_t mtu)
{
	return sizeof(struct ndp_packet_buffer) + mtu;
}

//TODO this is actually more than necessary; substract some headers before adding mtu
size_t aligned_ndp_packet_buffer_size(ndp_packet_buffer_size_t mtu)
{
	return aligned_size(ndp_packet_buffer_size(mtu),
			__alignof__(struct ndp_packet_buffer_flags));
}

size_t aligned_ndp_socket_size(const struct shm_parameters *shm_params)
{
	struct ndp_socket_info aux;
	struct ndp_instance_info aux2;

	aux2.sockets_begin = 0;
	aux2.shm_params = *shm_params;

	init_ndp_socket_info(&aux2, 0, &aux);

	return (size_t)aux.rx_packet_buffers + shm_params->num_rx_packet_buffers * aux.aligned_packet_buffer_size;
}

size_t shm_bytes_until_first_socket(const struct shm_parameters *shm_params)
{
	return aligned_size(offsetof(struct shm_layout, rest) +
			RING_SIZE(RING_BUF_OF_CHOICE(struct, ndp_instance_op_msg), shm_params->num_comm_ring_elements),
			__alignof__(struct ndp_socket));
}

size_t shm_size(const struct shm_parameters *shm_params)
{
	return shm_bytes_until_first_socket(shm_params) + shm_params->num_sockets * aligned_ndp_socket_size(shm_params);
}

void prepare_ndp_instance_info(void *shm_begins, const struct shm_parameters *shm_params, struct ndp_instance_info *instance_info)
{
	instance_info->shm_begins = (struct shm_layout*)shm_begins;
	instance_info->shm_params = *shm_params;

	instance_info->comm_ring =
			(struct RING_BUF_OF_CHOICE(struct, ndp_instance_op_msg) *)instance_info->shm_begins->rest;

	instance_info->sockets_begin = (char*)shm_begins + shm_bytes_until_first_socket(shm_params);
	instance_info->aligned_socket_size = aligned_ndp_socket_size(shm_params);
}

//void init_ndp_socket_info(char *sockets_begin, size_t aligned_socket_size, ndp_socket_index_t socket_index,
//		const struct shm_parameters *p, struct ndp_socket_info *socket_info)
void init_ndp_socket_info(const struct ndp_instance_info *instance_info, ndp_socket_index_t socket_index,
		struct ndp_socket_info *socket_info)
{
	socket_info->index = socket_index;
	socket_info->socket	= (struct ndp_socket*)(instance_info->sockets_begin + socket_index * instance_info->aligned_socket_size);

	char *aux = socket_info->socket->rest;

	socket_info->tx_ring = (struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *)aux;
	socket_info->listen_ring = (struct RING_BUF(ndp_socket_index_t) *)aux;

	aux += RING_ALIGNED_SIZE(RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t),
			instance_info->shm_params.num_tx_ring_elements,
			__alignof__(struct RING_BUF_OF_CHOICE(char)));

	socket_info->rtx_ring = (struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *)aux;

	aux += RING_ALIGNED_SIZE(RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t),
			instance_info->shm_params.num_rtx_ring_elements,
			__alignof__(struct RING_BUF_OF_CHOICE(char)));

	socket_info->to_rtx_ring = (struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *)aux;

	aux += RING_ALIGNED_SIZE(RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t),
			instance_info->shm_params.num_to_rtx_ring_elements,
			__alignof__(struct RING_BUF_OF_CHOICE(char)));

	socket_info->ack_tx_ring = (struct RING_BUF_OF_CHOICE(struct, ndp_packet_headers)  *)aux;

	aux += RING_ALIGNED_SIZE(RING_BUF_OF_CHOICE(struct, ndp_packet_headers),
			instance_info->shm_params.num_ack_tx_ring_elements,
			__alignof__(struct RING_BUF_OF_CHOICE(char)));

	socket_info->rx_ring = (struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *)aux;

	aux += RING_ALIGNED_SIZE(RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t),
			instance_info->shm_params.num_rx_ring_elements,
			__alignof__(struct RING_BUF_OF_CHOICE(char)));

	socket_info->ack_rx_ring = (struct RING_BUF_OF_CHOICE(struct, ndp_header) *)aux;

	//socket_info->tx_packet_buffers = aux + aligned_ring_size(instance_info->shm_params.num_ack_rx_ring_elements,
	//		__alignof__(struct ndp_packet_buffer_flags));

	aux += RING_ALIGNED_SIZE(RING_BUF_OF_CHOICE(struct, ndp_header),
			instance_info->shm_params.num_ack_rx_ring_elements,
			__alignof__(struct RING_BUF_OF_CHOICE(char)));

	socket_info->tx_packet_buffers = aux;

	socket_info->aligned_packet_buffer_size = aligned_ndp_packet_buffer_size(instance_info->shm_params.packet_buffer_mtu);

	socket_info->rx_packet_buffers = socket_info->tx_packet_buffers +
			instance_info->shm_params.num_tx_packet_buffers * socket_info->aligned_packet_buffer_size;

#if DEBUG_LEVEL > 0
	socket_info->num_tx_packet_buffers = instance_info->shm_params.num_tx_packet_buffers;
	socket_info->num_rx_packet_buffers = instance_info->shm_params.num_rx_packet_buffers;
#endif
}



void prepare_ndp_packet_headers(ndp_net_addr_t saddr, ndp_net_addr_t daddr, ndp_port_number_t sport,
		ndp_port_number_t dport, void *smac, void *dmac, struct ndp_packet_headers *h)
{
	memset(h, 0, sizeof(struct ndp_packet_headers));

	memcpy(&h->eth.MAC_SRCFLD, smac, 6);
	memcpy(&h->eth.MAC_DSTFLD, dmac, 6);
	h->eth.ether_type = htons(NDP_ETHERTYPE);

	h->ip.version = 4;
	h->ip.ihl = 5;
	h->ip.ttl = 200;
	h->ip.protocol = NDP_PROTOCOL_NUMBER;
	h->ip.saddr = saddr;
	h->ip.daddr = daddr;

	h->ndp.src_port = sport;
	h->ndp.dst_port = dport;
}

ndp_packet_buffer_index_t find_tx_pkb_for_seq(struct ndp_socket_info *socket_info, ndp_packet_buffer_index_t num_pkbs,
												ndp_sequence_number_t seq_in_network_order)
{
	for(ndp_socket_index_t i = 0; i < num_pkbs; i++)
	{
		struct ndp_packet_buffer *b = get_tx_packet_buffer(socket_info, i);

		if(!b->flags.lib_in_use)
			continue;

		if(b->hdr.ndp.sequence_number == seq_in_network_order)
			return i;
	}

	return -1;
}


void log_packet(void *p)
{
	#ifdef BUILDING_WITH_DPDK
	  struct ether_hdr *eth;
	#else
	  struct ether_header *eth;
	#endif

	eth = p;

	log_msg(">>>>> ett %04x src "MAC_PRINTF_FORMAT" dst "MAC_PRINTF_FORMAT, ntohs(eth->ether_type), MAC_PRINTF_VALUES(&eth->MAC_SRCFLD),
			MAC_PRINTF_VALUES(&eth->MAC_DSTFLD));

	struct iphdr *ip = (struct iphdr*)&eth[1];

	log_msg("ipp %u src "MY_IPV4_FORMAT" dst "MY_IPV4_FORMAT" len %u", ip->protocol, MY_IPV4_VALUES(ip->saddr), MY_IPV4_VALUES(ip->daddr),
			ntohs(ip->tot_len));

	if(ip->protocol == NDP_PROTOCOL_NUMBER)
	{
		struct ndp_header *ndp = (struct ndp_header*)&ip[1];
		log_msg("ndp flags %u sp %u dp %u seq %u <<<<<", ndp->flags, ndp_port_ntoh(ndp->src_port), ndp_port_ntoh(ndp->dst_port),
				ndp_seq_ntoh(ndp->sequence_number));
	}
}



double ns_per_frame_len(uint32_t len)
{
	//min frame
	if(len < 60)
		len = 60;

	//+ 8 for preamble & start of frame
	//+ 4 for checksum
	//TODO interframe gap not considered (afraid of creating too much spacing)
	//TODO 802.1q tagging not considered (eww)
	len += 12;

	double aux;

	aux = (double)NDP_LINK_SPEED_BPS / NS_PER_SECOND; //bits_per_ns
	aux = (len << 3) / aux; //ns_for_len

	return aux;
}


ndp_cycle_count_t cycles_per_frame_len(uint32_t len, double tsc_hz)
{
	double aux = ns_per_frame_len(len); //ns_for_len
	return aux * tsc_hz / NS_PER_SECOND; //cycles_for_len
}

//yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy


static inline int get_mac_address_aux(ndp_net_addr_t ip, void *buf)
{
	char *x = (char*)&ip;
	int computer_idx = x[3] - 1;

	//LOG_FMSG(LOGLVL1, "%d", computer_idx);

	//if(computer_idx != 7)
		//exit(1);

	#if   NDP_CURRENT_TEST_ENVIRONMENT == ENV_GAINA
		return gaina_get_mac(computer_idx, 0, buf);
	#elif NDP_CURRENT_TEST_ENVIRONMENT == ENV_GAINA_FPGA
		return gaina_get_mac(computer_idx, 1, buf);
	#elif NDP_CURRENT_TEST_ENVIRONMENT == ENV_COCOS
		return cocos_get_mac(computer_idx, 0, buf);
	#elif NDP_CURRENT_TEST_ENVIRONMENT == ENV_CAM
		return cam_get_mac(computer_idx, 0, buf);
	#endif

	return -1;
}

#if NDP_HARDCODED_SAME_NEXT_HOP
	int get_mac_address(ndp_net_addr_t ip, void *buf, ndp_net_addr_t local_addr)
#else
	int get_mac_address(ndp_net_addr_t ip, void *buf)
#endif
{
	#if NDP_HARDCODED_SAME_NEXT_HOP

		if(ip == local_addr)
			return get_mac_address_aux(ip, buf);

		char *x = (char*)&local_addr;
		int computer_idx = x[3] - 1;

		int interface_idx = -1;

		#if   NDP_CURRENT_TEST_ENVIRONMENT == ENV_GAINA
			interface_idx = computer_idx - 5;
		#elif NDP_CURRENT_TEST_ENVIRONMENT == ENV_COCOS
			switch(computer_idx)
			{
				case 20 - 1 : interface_idx = 0; break;
				case 9 - 1 : interface_idx = 1; break;
				case 19 - 1: interface_idx = 2; break;
				case 8 - 1: interface_idx = 3; break;
				default: interface_idx = -1; break;
			}
			//interface_idx = computer_idx - 5;
		#endif

		//LOG_FMSG(1, "ii %d", interface_idx);

		if(interface_idx < 0 || interface_idx > 3)
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> interface_idx < 0 || interface_idx > 3 (%d)", __func__, interface_idx);

		#if   NDP_CURRENT_TEST_ENVIRONMENT == ENV_GAINA
			return gaina_get_mac(1, interface_idx, buf);
		#elif NDP_CURRENT_TEST_ENVIRONMENT == ENV_COCOS
			return cocos_get_mac(9, interface_idx, buf);
		#endif

		return -1;

	#else
		return get_mac_address_aux(ip, buf);
	#endif

}


//////


#ifdef NDP_BUILDING_FOR_XEN

	size_t num_pages_for_bytes(size_t num_bytes)
	{
		size_t num_pages = num_bytes / ZZZ_PAGE_SIZE;
		if(num_bytes % ZZZ_PAGE_SIZE)
			num_pages++;

		if(num_pages == 0)
		{
			printf("hmm num_pages_for_bytes == 0\n");
			exit(1);
		}

		return num_pages;
	}

	size_t num_meta_pages(size_t num_pages)
	{
		size_t x = num_pages_for_bytes(num_pages * sizeof(uint32_t));

		if(x == 1)
			return 1;
		else
			return x + num_meta_pages(x);
	}

#endif



/////////////////////////////////////////////////////////////

void aux_sleep(long ns)
{
	if(ns)
	{
		struct timespec sleep_req = { .tv_nsec = ns };
		nanosleep(&sleep_req, NULL);
	}
}
