#include "helpers.h"
//#include <rte_cycles.h>

//TODO cleanup these functions; they look really messy

//uint8_t test3[10 *1000 *1000];

static inline int send_packet_buffer(struct core_socket_descriptor *d, struct ndp_packet_buffer *pb)
{
	/*
	ndp_sequence_number_t seqq = ndp_seq_ntoh(pb->hdr.ndp.sequence_number);
	if(test3[seqq]++)
		LOG_FMSG(LOGLVL1, "test3[%lu] = %d", seqq, test3[seqq] - 1);
	 */

	size_t psz = d->instance_info->shm_params.packet_buffer_mtu - pb->bytes_left +
								sizeof(struct ether_hdr);

	int x;

	//uint64_t t1 = core_read_tsc();

	#if NDP_CORE_SEND_CHOPPED_RATIO

		if(core.send_chopped_counter++ == NDP_CORE_SEND_CHOPPED_RATIO)
		{
			core.send_chopped_counter = 0;
			x = send_one_chopped_packet(&pb->hdr);
		}
		else
			x = send_one_packet(&pb->hdr, psz);

	#else

		x = send_one_packet(&pb->hdr, psz, core.dpdk_default_queue_id);

	#endif

	if(!x)
		d->tx_allowance--;

	return x;

	//uint64_t t2 = core_read_tsc();
	//LOG_FMSG(LOGLVL101, "delta: %" PRIu64 "\n", t2 - t1);
}


static inline void check_tx_ring(struct core_socket_descriptor *d)
{
	struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *r = d->socket_info.tx_ring;

	ndp_packet_buffer_index_t pb_idx;
	//while(d->tx_allowance && consumer_peek(r, &pb_idx, sizeof(pb_idx)))


	//ndp_cycle_count_t tsc = ndp_rdtsc();

	while(d->tx_allowance && CONSUMER_AVAILABLE(r))
	{
		pb_idx = GET_ELEMENT_AT(r, GET_CONSUMER_INDEX(r));

		struct ndp_packet_buffer *pb = get_tx_packet_buffer(&d->socket_info, pb_idx);


		if(send_packet_buffer(d, pb))
		{
			continue; //keep trying
			//return;
		}
		else
		{
			pb->flags.core_sent_to_nic = 1;

			#if LOG200_REC
				core_record_event(LOG_RECORD_CORE_DATA_SENT, d->instance_info->id, d->socket_info.index,
						ndp_seq_ntoh(pb->hdr.ndp.sequence_number), core_read_tsc());
			#endif

			#if LOG200_MSG
			  log_msg("%s -> sent DATA seq %lu sidx %ld", __func__, ndp_seq_ntoh(pb->hdr.ndp.sequence_number), d->socket_info.index);
			#endif

			//consumer_advance(r, sizeof(pb_idx));
			RELEASE_FENCE();
			CONSUMER_ADVANCE(r, 1);
		}
	}
}


static inline void check_rtx_ring(struct core_socket_descriptor *d, struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *r)
{
	ndp_packet_buffer_index_t pb_idx;

	char is_timeout = (r == d->socket_info.to_rtx_ring);

	//while((d->tx_allowance || is_timeout) && consumer_peek(r, &pb_idx, sizeof(pb_idx)))
	while((d->tx_allowance || is_timeout) && CONSUMER_AVAILABLE_ACQUIRE(r))
	{
		pb_idx = GET_ELEMENT_AT(r, GET_CONSUMER_INDEX(r));

		struct ndp_packet_buffer *pb = get_tx_packet_buffer(&d->socket_info, pb_idx);

		MY_LOCK_ACQUIRE(pb->sync_helper);

		if(LIKELY(pb->flags.lib_ready_to_send))
		{
			if(send_packet_buffer(d, pb))
				goto release_and_continue;
			else
			{
				#if LOG200_REC
					log_record_type_t rec_type = LOG_RECORD_CORE_DATA_RTX;
					if(is_timeout)
						rec_type = LOG_RECORD_CORE_DATA_TO_RTX;

					core_record_event(rec_type, d->instance_info->id, d->socket_info.index,
							ndp_seq_ntoh(pb->hdr.ndp.sequence_number), core_read_tsc());
				#endif

				#if LOG200_MSG
					log_msg("%s -> sent (rtx = %d) DATA seq %lu sidx %ld", __func__, is_timeout ? 2 : 1,
														ndp_seq_ntoh(pb->hdr.ndp.sequence_number), d->socket_info.index);
				#endif

				if(is_timeout)
				{
					d->tx_allowance++;
					//pb->flags.in_to_rtx_ring = 0;
				}

				pb->flags.core_sent_to_nic = 1;
			}
		}
		else
		{
			//LOG_FMSG(LOGLVL4, "%s -> !pb->flags.lib_ready_to_send", __func__);
			cctrs.not_lrts++;
		}

		//consumer_advance(r, sizeof(pb_idx));
		RELEASE_FENCE();
		CONSUMER_ADVANCE(r, 1);

	release_and_continue:
		MY_LOCK_RELEASE(pb->sync_helper);
	}
}

/*
static void check_tx_or_rtx(struct core_socket_descriptor *d, char check_rtx)
{
	struct ring_buf *r;

	if(check_rtx)
		r = d->socket_info.rtx_ring;
	else
		r = d->socket_info.tx_ring;

	ndp_packet_buffer_index_t pb_idx;

	while(d->tx_allowance && consume_at_most(r, &pb_idx, sizeof(pb_idx)))
	{
		struct ndp_packet_buffer *pb = get_tx_packet_buffer(&d->socket_info, pb_idx);

		MY_ACQUIRE(pb->sync_helper);

		if(LIKELY(pb->flags.lib_ready_to_send))
		{
			size_t psz = d->instance_info->shm_params.packet_buffer_mtu - pb->bytes_left +
									sizeof(struct ether_hdr);

			//uint64_t t1 = core_read_tsc();

			send_one_packet(&pb->hdr, psz);

			//uint64_t t2 = core_read_tsc();
			//LOG_FMSG(LOGLVL101, "delta: %" PRIu64 "\n", t2 - t1);

			d->tx_allowance--;

			LOG_FMSG(LOGLVL200, "%s -> sent (rtx = %d) DATA seq %lu sidx %ld", __func__, check_rtx,
					ndp_seq_ntoh(pb->hdr.ndp.sequence_number), d->socket_info.index);

		}

		MY_RELEASE(pb->sync_helper);
	}
}
*/

//static uint8_t acks[10 * 1000 * 1000];

void check_tx(void)
{
	int sockets_left = core.num_active_sockets;

	for(int i = 0; sockets_left && i < NDP_CORE_MAX_SOCK_DESC; i++)
	{
		if(core.socket_descriptor_available[i])
			continue;

		struct core_socket_descriptor *d = &core.socket_descriptors[i];

		if(!d->socket_is_active)
			continue;

		sockets_left--;

		//RTX & TX rings
		check_rtx_ring(d, d->socket_info.to_rtx_ring);
		check_rtx_ring(d, d->socket_info.rtx_ring);
		check_tx_ring(d);


		//ACK_TX ring

		struct ndp_packet_headers aux_h;

		//while(consume_at_most(d->socket_info.ack_tx_ring, &aux_h, sizeof(struct ndp_packet_headers)))
		while(CONSUMER_AVAILABLE(d->socket_info.ack_tx_ring))
		{
			aux_h = CONSUME_RELEASE(d->socket_info.ack_tx_ring);
			/*
			if(acks[ndp_seq_ntoh(aux_h.ndp.sequence_number)]++)
				LOG_FMSG(LOGLVL1, "mul acks %d seq %lu", acks[ndp_seq_ntoh(aux_h.ndp.sequence_number)] - 1,
						ndp_seq_ntoh(aux_h.ndp.sequence_number));*/

			send_one_packet(&aux_h, sizeof(struct ndp_packet_headers), core.dpdk_default_queue_id);

			#if LOG200_REC
				core_record_event(LOG_RECORD_CORE_ACK_SENT, d->instance_info->id, d->socket_info.index,
						ndp_seq_ntoh(aux_h.ndp.sequence_number), core_read_tsc());
			#endif

			#if LOG200_MSG
			  log_msg("%s -> sent ACK seq %lu sidx %ld", __func__, ndp_seq_ntoh(aux_h.ndp.sequence_number), d->socket_info.index);
			#endif
		}
	}
}
