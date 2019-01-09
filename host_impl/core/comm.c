#include "../common/logger.h"
#include <arpa/inet.h>
#include "helpers.h"
#include <sys/mman.h>
#include <unistd.h>


#ifdef NDP_BUILDING_FOR_XEN

	static void unmap_grant(xengnttab_handle *gh, const struct map_grant_result *r)
	{
	  int x = xengnttab_unmap(gh, r->mem, r->num_pages);
	  if(x)
		printf("unmap_grant err = %d\n", x);
	}

#endif


void check_comm_rings(void)
{
	int instances_left = core.num_lib_instances;

	struct ndp_instance_op_msg msg;

	for(int i = 0; instances_left && i < NDP_CORE_MAX_LIBS; i++)
	{
		if(core.instance_descriptor_available[i])
			continue;

		struct ndp_instance_info *instance_info = &core.instance_descriptors[i].instance_info;

		//if(!instance_info->shm_begins->flags.lib_done_handshake)
		//	continue;

		//while(consume_at_most(instance_info->comm_ring, &msg, sizeof(msg)))
		while(CONSUMER_AVAILABLE_ACQUIRE(instance_info->comm_ring))
		{
			msg = CONSUME_RELEASE(instance_info->comm_ring);

			if(msg.type == NDP_INSTANCE_OP_CONNECT)
			{

				struct core_socket_descriptor *d = get_socket_descriptor(instance_info, msg.socket_index);

				if(!d)
					LOG_FMSG_DO(LOGLVL2, continue, "%s -> from instace_id %ld fpr socket_index %d NULL get_socket_descriptor",
							instance_info->id, msg.socket_index, __func__);

				struct ndp_connection_tuple *tuple = &d->socket_info.socket->tuple;

				//tempLOG_FMSG(LOGLVL9, "%s -> CONNECT from instance_id %ld for socket_index %d to "MY_IPV4_FORMAT":%u", __func__,
						//instance_info->id, msg.socket_index, MY_IPV4_VALUES(tuple->remote_addr), ndp_port_ntoh(tuple->remote_port));

				//fill in local addr & port; TODO better local port management
				tuple->local_addr = core.local_addr;
				tuple->local_port = ndp_port_hton(--core.next_available_port);

				RELEASE_FENCE();

				//TODO are flags properly reset on socket reuse?
				d->socket_info.socket->flags.core_connect_acknowledged = 1;

				prepare_socket_descriptor(instance_info, 1, d);
			}
			else if(msg.type == NDP_INSTANCE_OP_LISTEN)
			{
				struct core_socket_descriptor *d = get_socket_descriptor(instance_info, msg.socket_index);
				if(!d)
					LOG_FMSG_DO(LOGLVL2, continue, "%s -> LISTEN -> NULL get_socket_descriptor(instance_info, msg.socket_index)", msg.type, __func__);


				init_ndp_socket_info(instance_info, msg.socket_index, &d->socket_info);

				LOG_FMSG(LOGLVL9, "%s -> LISTEN from instance_id %ld for socket_index %ld on port %lu", __func__, instance_info->id,
						msg.socket_index, ndp_port_ntoh(d->socket_info.socket->tuple.local_port));

				//TODO should check if the specified local port is actually available !

				d->socket_info.socket->tuple.local_addr = core.local_addr;

				prepare_socket_descriptor(instance_info, 0, d);

				core.maxxx = 0;

			}
			else if(msg.type == NDP_INSTANCE_OP_CLOSE)
			{
				struct core_socket_descriptor *d = get_existing_socket_descriptor(instance_info, msg.socket_index);

				if(UNLIKELY (!d))
					LOG_FMSG_DO(LOGLVL2, continue, "%s -> CLOSE cannot find socket", __func__);

				core.num_active_sockets -= d->socket_is_active;
				core.num_passive_sockets -= d->socket_is_passive;

				memset(d, 0, offsetof(struct core_socket_descriptor, index));

				#if NDP_CORE_NACKS_FOR_HOLES
					RING_RESET(d->n4h_ring);
				#endif

				core.socket_descriptor_available[d->index] = 1;
			}
			else if(msg.type == NDP_INSTANCE_OP_TERMINATE)
			{
				//TODO this should actually take much more things into account; for example some pointers to sockets belonging to
				//this instance may still be in the pull queue

				//first make release all associated socket descriptors; there should be a sockets-descriptors-in-use ring per instance
				//descriptor so they can be easily singled out, but for now ah well

				for(int k = 0; k < NDP_CORE_MAX_SOCK_DESC; k++)
				{
					if(! core.socket_descriptor_available[k])
					{
						struct core_socket_descriptor *d = &core.socket_descriptors[k];

						if(d->instance_info->id == instance_info->id)
						{
							core.num_active_sockets -= d->socket_is_active;
							core.num_passive_sockets -= d->socket_is_passive;
							core.socket_descriptor_available[k] = 1;
						}
					}
				}

				//clean everything else up

				RELEASE_FENCE();

				instance_info->shm_begins->flags.core_terminate_acknowledged = 1;

				#ifdef NDP_BUILDING_FOR_XEN

					unmap_grant(core.gh, &core.instance_descriptors[i].map_grant_res);

				#else

					if(UNLIKELY (munmap(instance_info->shm_begins, shm_size(&instance_info->shm_params))))
						LOG_FMSG(LOGLVL10, "%s -> terminate error unmapping shm area", __func__);

					if(UNLIKELY (close(core.instance_descriptors[i].shm_fd)))
						LOG_FMSG(LOGLVL10, "%s -> terminate close shm_fd error", __func__);
				#endif

				core.instance_descriptor_available[i] = 1;
				core.num_lib_instances--;

				LOG_FMSG(LOGLVL10, "%s -> instance id %d terminated successfully", __func__, i);//, CONSUMER_AVAILABLE_ACQUIRE(core.pullq));

				//zzzzzzzz
				print_core_stupid_counters();
				memset(&cctrs, 0, sizeof(struct core_stupid_counters));
				//zzzzzzzz

				///*
				struct rte_eth_stats eth_stats;

				if(rte_eth_stats_get(core.dpdk_port_id, &eth_stats))
					rte_exit(1, "rte_eth_stats_get error\n");

				printf(	"eth_stats ipackets %lu ierrors %lu imissed %lu rx_nombuf %lu \n"
						"          opackets %lu oerrors %lu\n", eth_stats.ipackets, eth_stats.ierrors,
						eth_stats.imissed, eth_stats.rx_nombuf, eth_stats.opackets, eth_stats.oerrors);

				rte_eth_stats_reset(core.dpdk_port_id);
				//*/

				break;

			}/*
			else if(msg.type == NDP_INSTANCE_OP_CLOSE)
			{
				LOGF_MSG(1, "received CLOSE from instance_id %ld for socket_index %d", instance_data->id, msg.socket_index);

				struct core_socket_descriptor *socket_descriptor = find_socket_descriptor(msg.socket_index);

				if(!socket_descriptor)
					LOGF_DO(1, continue, "cannot find matching socket_descriptor for index %d", msg.socket_index);

				struct ndp_socket_flags *flags = &socket_descriptor->map.start->flags;
				VOLATILE(flags)->core_close_acknowledged = 1;

				socket_descriptor->in_use = 0;
				num_active_sockets--;
			}*/
			else
				LOG_FMSG(LOGLVL2, "%s -> received BAD op type : %d", __func__, msg.type);

		}

		instances_left--;
	}
}


