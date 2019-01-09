#include "../common/logger.h"
#include "helpers.h"
#include <rte_cycles.h>
//#include <rte_ethdev.h>
#include <rte_ether.h>
//#include <rte_ip.h>
#include <rte_mbuf.h>
#include <unistd.h>


void core_output_file_name(char *buf, size_t len, const char *middle)
{
	char suffix[50];
	//TODO this format pretty much assumes local_addr is IPv4
	sprintf(suffix, "_%08x.dat", core.local_addr);

	if(!getcwd(buf, len))
		LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> getcwd error", __func__);

	if(strlen(buf) + strlen(middle) + strlen(suffix) >= len - 1)
		LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> full name too long", __func__);


	sprintf(buf + strlen(buf), "%s%s", middle, suffix);
}


struct core_instance_descriptor* get_available_instance_descriptor(void)
{
	struct core_instance_descriptor *result = NULL;

	for(int i = 0; i < NDP_CORE_MAX_LIBS; i++)
	{
		if(core.instance_descriptor_available[i])
		{
			result = &core.instance_descriptors[i];
			//setting this in msg processing code from global_comm
			//core.lib_instance_descriptor_available[i] = 0;
			break;
		}
	}

	return result;
}


struct core_socket_descriptor* get_socket_descriptor(struct ndp_instance_info *instance_info,
		ndp_socket_index_t socket_index)
{
	struct core_socket_descriptor *result = NULL;

	for(int i = 0; i < NDP_CORE_MAX_SOCK_DESC; i++)
	{
		if(core.socket_descriptor_available[i])
		{
			result = &core.socket_descriptors[i];
			core.socket_descriptor_available[i] = 0;

			init_ndp_socket_info(instance_info, socket_index, &result->socket_info);

			#if NDP_CORE_NACKS_FOR_HOLES
				if(result->n4h_ring == NULL)
				{
					result->n4h_ring = malloc(RING_SIZE(ROUND_RING_BUF(char), NDP_CORE_NUM_N4H_RING_ELEMENTS));
					RING_INIT(result->n4h_ring, NDP_CORE_NUM_N4H_RING_ELEMENTS);
				}

				for(int j = 0; j < NDP_CORE_NUM_N4H_RING_ELEMENTS; j++)
					SET_ELEMENT_AT(result->n4h_ring, j, 0);
			#endif

			break;
		}
	}

	return result;
}

struct core_socket_descriptor* get_existing_socket_descriptor(struct ndp_instance_info *instance_info,
		ndp_socket_index_t socket_index)
{
	for(int i = 0; i < NDP_CORE_MAX_SOCK_DESC; i++)
	{
		struct core_socket_descriptor *d = &core.socket_descriptors[i];

		if(!core.socket_descriptor_available[i] && d->instance_info == instance_info && d->socket_info.index == socket_index)
			return &core.socket_descriptors[i];
	}

	return NULL;
}


void prepare_socket_descriptor(struct ndp_instance_info *instance_info, char is_active, struct core_socket_descriptor *d)
{
	d->instance_info = instance_info;

	/*d->socket_is_active = 0;
	d->socket_is_passive = 0;

	if(is_active)
	{
		d->socket_is_active = 1;
		core.num_active_sockets++;

		d->tx_allowance = NDP_CORE_INITIAL_TX_ALLOWANCE;
	}
	else
	{
		d->socket_is_passive = 1;
		core.num_passive_sockets++;
	}*/

	d->socket_is_active = !! is_active;
	d->socket_is_passive = ! is_active;

	#if NDP_CORE_NACKS_FOR_HOLES
		d->got_at_least_one_data_packet = 0;
	#endif

	core.num_active_sockets  += d->socket_is_active;
	core.num_passive_sockets += d->socket_is_passive;

	d->tx_allowance = NDP_CORE_INITIAL_TX_ALLOWANCE;
}


void log_mbuf_dump(struct rte_mbuf *b, int bytes_per_line)
{
	int len = rte_pktmbuf_pkt_len(b);
	unsigned char *buf = rte_pktmbuf_mtod(b, unsigned char*);

	for(int i = 0; i < len; i++)
	{
		if(i && (i % bytes_per_line == 0))
			log_msg_partial("\n");
		log_msg_partial("%02x ", buf[i]);
	}

	log_msg("");
}


#if NDP_CORE_SEND_CHOPPED_RATIO
	int send_one_chopped_packet(void *buf)
	{
		//LOG_MSG(LOGLVL1, "choppy!");

		#define NDP_CONTROL_FRAME_SIZE  	(NDP_CONTROL_PACKET_SIZE + sizeof(struct ether_hdr))


		struct rte_mbuf *p = rte_pktmbuf_alloc(core.mbuf_pool);

		if(!p)
			LOG_FMSG_DO(LOGLVL3, return -1, "%s -> ! rte_pktmbuf_alloc", __func__);

		//should never happen :-s
		if(UNLIKELY(!rte_pktmbuf_append(p, NDP_CONTROL_FRAME_SIZE)))
			LOG_FMSG_DO(LOGLVL3, return -2, "%s -> ! rte_pktmbuf_append", __func__);

		memcpy(rte_pktmbuf_mtod(p, void*), buf, NDP_CONTROL_FRAME_SIZE);
		struct ndp_packet_headers *h = rte_pktmbuf_mtod(p, struct ndp_packet_headers*);
		h->ip.tot_len = htons(NDP_CONTROL_PACKET_SIZE);
		h->ndp.flags = NDP_HEADER_FLAG_CHOPPED;

		return send_one_buffer(p);
	}
#endif
