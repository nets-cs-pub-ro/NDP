#ifndef NDP_LIB_RECV_AUX_H_
#define NDP_LIB_RECV_AUX_H_

#include "../common/ring.h"
#include "defs.h"
#include "helpers.h"


static void free_rx_pkb(struct lib_socket_descriptor *d, ndp_packet_buffer_index_t idx)
{
	if(LIKELY (PRODUCER_AVAILABLE_ACQUIRE(d->socket_info.rx_ring)))
		PRODUCE_RELEASE(d->socket_info.rx_ring, idx)
	else
	{
		LOG_FMSG(LOGLVL2, "%s -> no room to recycle bad rx pkb %ld", __func__, idx);
		exit(1);
	}
}


//static inline void add_to_pending_acks(struct lib_socket_descriptor *d, ndp_packet_buffer_index_t idx)
//{
//	//TODO zzzzz the cannot be inlined story
//}


static inline void __attribute__((always_inline)) send_ack(struct lib_socket_descriptor *d, ndp_sequence_number_t seq_in_network_order,
		ndp_sequence_number_t rwnd_edge, int from_main_thread)
{
	d->ack_template.ndp.sequence_number = seq_in_network_order;

	ndp_sequence_number_t seq = ndp_seq_ntoh(seq_in_network_order);
	d->ack_template.ndp.recv_window = ndp_rwnd_hton(rwnd_edge - seq);

	//!!!!!!!!
	//if(d->socket_info.index == 3)
		//printf("s3 sent ack for seq %u\n", seq);


	//guess this shouldn't happen; but I don't know right now really :-s
	//if(!produce_at_most(d->socket_info.ack_tx_ring, &d->ack_template, sizeof(struct ndp_packet_headers), 1))
	//	LOG_FMSG(LOGLVL3, "%s -> no room in socket_info.ack_tx_ring", __func__)
	if(LIKELY (PRODUCER_AVAILABLE_ACQUIRE(d->socket_info.ack_tx_ring)))
	{
		PRODUCE_RELEASE(d->socket_info.ack_tx_ring, d->ack_template);

		#if LOG201_REC
			lib_record_event(LOG_RECORD_LIB_ACK_SENT, lib.instance_info.id, d->socket_info.index,
					ndp_seq_ntoh(seq_in_network_order), lib_read_tsc(), from_main_thread);
		#endif

		#if LOG201_MSG
			log_msg("%s -> sent ACK seq %lu sidx %ld", __func__, ndp_seq_ntoh(seq_in_network_order), d->socket_info.index);
		#endif
	}
	else
		LOG_FMSG(LOGLVL3, "%s -> no room in socket_info.ack_tx_ring", __func__);
}


//was inline at some point
static void send_pending_acks(struct lib_socket_descriptor *d, int n, int from_main_thread)
{
	if(!n)
		return;

	//TODO should it be with "- 1" at the end?
	//ndp_sequence_number_t rwnd_edge = d->recv_pkt_seq + NDP_INITIAL_RX_WINDOW_SIZE - 1;
	ndp_sequence_number_t rwnd_edge = d->recv_pkt_seq + NDP_INITIAL_RX_WINDOW_SIZE;

	if(rwnd_edge - d->largest_advertised_rwnd_edge <= NDP_MAX_SEQ_DIFF)
		d->largest_advertised_rwnd_edge = rwnd_edge;

	while(n--)
	{
		ndp_packet_buffer_index_t idx = CONSUME(d->pending_to_be_acked);
		const struct ndp_packet_buffer *b = get_rx_packet_buffer(&d->socket_info, idx);

		send_ack(d, b->hdr.ndp.sequence_number, rwnd_edge, from_main_thread);
	}
}



/*//!!!!

static inline void pprint_rring(struct lib_socket_descriptor *d, int N)
{
	struct RING_BUF(ndp_packet_buffer_index_t) *w = d->rx_wnd_ring;

	ring_size_t pi = GET_PRODUCER_INDEX(w);
	printf("%u * ", pi);

	for(int i = 0; i < N; i++)
	{
		int x = GET_ELEMENT_AT_WRAPPED(w, pi + i);
		printf("%d ", x);
	}

	printf("\n");
}


///!!!!!*/



static inline ring_size_t check_for_incoming_data(struct lib_socket_descriptor *d, int from_main_thread)
{
	struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *r = d->socket_info.rx_ring;
	struct RING_BUF(ndp_packet_buffer_index_t) *w = d->rx_wnd_ring;

	ring_size_t cci = GET_CONSUMER_INDEX(r);
	ACQUIRE_FENCE();

	if(cci == d->rx_consumer_shadow)
		return 0;

	ring_size_t num_arrivals;

	if(cci >= d->rx_consumer_shadow)
		num_arrivals = cci - d->rx_consumer_shadow;
	else
		num_arrivals = RING_NUM_ELEMENTS(r) - d->rx_consumer_shadow + cci;

	//LOG_FMSG(LOGLVL1, "new %lu pa %lu cci %lu csh %lu", new, producer_available(r), cci, d->rx_consumer_shadow);

	ndp_packet_buffer_index_t idx;

	lctrs.data_totrx += num_arrivals;

	for(ring_size_t i = 0; i < num_arrivals; i++)
	{
		idx = GET_ELEMENT_AT_WRAPPED(r, d->rx_consumer_shadow + i);
		struct ndp_packet_buffer *b = get_rx_packet_buffer(&d->socket_info, idx);
		ndp_sequence_number_t seq = ndp_seq_ntoh(b->hdr.ndp.sequence_number);

		//TODO take the if outside the loop
		if(d->syn_received)
		{
			ndp_sequence_number_t next_in_order_seq = d->recv_pkt_seq;
			ndp_sequence_number_t seq_diff = seq - next_in_order_seq;

			if(UNLIKELY (seq_diff > NDP_MAX_SEQ_DIFF))
			{
				//seq is actually before last_in_order_seq ; old packet

				#if LOG201_REC
					lib_record_event(LOG_RECORD_LIB_OLD_DATA_RCVD, lib.instance_info.id, d->socket_info.index,
							seq, lib_read_tsc(), from_main_thread);
				#endif

				#if LOG201_MSG
					log_msg("%s -> rcvd OLD DATA seq %lu sidx %d", __func__, seq - d->recv_isn, d->socket_info.index);
				#endif

				//TODO maybe send ACK
					send_ack(d, b->hdr.ndp.sequence_number, d->largest_advertised_rwnd_edge, from_main_thread);
					//log_msg("%s -> sent ACK for OLD DATA seq %lu sidx %d", __func__, seq - d->recv_isn, d->socket_info.index);
					lctrs.ack_old_data++;
				///

				free_rx_pkb(d, idx);
			}
			else if(UNLIKELY (seq_diff >= NDP_INITIAL_RX_WINDOW_SIZE))
			{
				//"future" packet (outside of recv window) ... shouldn't really happen
				LOG_FMSG(LOGLVL4, "%s -> received future packet with seq %u next_in_order_seq %u", __func__, seq, next_in_order_seq);

				free_rx_pkb(d, idx);
			}
			else
			{
				#if LOG201_REC
					lib_record_event(LOG_RECORD_LIB_DATA_RCVD, lib.instance_info.id, d->socket_info.index,
							seq, lib_read_tsc(), from_main_thread);
				#endif


				#if LOG201_MSG
					log_msg("%s -> rcvd DATA seq %lu sidx %ld", __func__, seq, d->socket_info.index);
				#endif


				//if(seq - d->recv_isn >= 4100)
				//	log_msg("zzzzz rcvd DATA seq %lu expecting %ld", seq - d->recv_isn, d->recv_pkt_seq - d->recv_isn);

				/*
				if(seq != d->asdf_prev + 1)
					log_msg("zzzzz %lu after %lu", seq - d->recv_isn, d->asdf_prev - d->recv_isn);

				d->asdf_prev = seq;
				*/


				ndp_packet_buffer_index_t what_am_i_overwriting = GET_ELEMENT_AT_WRAPPED(w, GET_PRODUCER_INDEX(w) + seq_diff);

				if(LIKELY (what_am_i_overwriting < 0))
				{
					SET_ELEMENT_AT_WRAPPED(w, GET_PRODUCER_INDEX(w) + seq_diff, idx);


					///!!!!!!
					//if(d->socket_info.index == 3)
					//	printf("placed %u %u %u\n", seq, idx, GET_PRODUCER_INDEX(w) + seq_diff);


					//send_ack(d, b->hdr.ndp.sequence_number, from_main_thread);
					if(LIKELY (PRODUCER_AVAILABLE(d->pending_to_be_acked)))
						PRODUCE(d->pending_to_be_acked, idx)
					else
						LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> !PRODUCER_AVAILABLE(d->pending_to_be_acked)", __func__);
				}
				else
				{
					//LOG_FMSG(LOGLVL4, "%s -> something in w already seq %lu old %d new %d sidx %u", __func__,
							//seq - d->recv_isn, what_am_i_overwriting, idx, d->socket_info.index);

					///!!!!
					//printf("INWWWW1\n");

					lctrs.in_w_already++;

					//TODO is it ok?
					send_ack(d, b->hdr.ndp.sequence_number, d->largest_advertised_rwnd_edge, from_main_thread);

					free_rx_pkb(d, idx);
				}
			}
		}
		else
		{
			//SYN not received yet

			if(!d->any_packet_received)
			{
				#if LOG201_REC
					lib_record_event(LOG_RECORD_LIB_DATA_RCVD, lib.instance_info.id, d->socket_info.index,
							seq, lib_read_tsc(), from_main_thread);
				#endif

				#if LOG201_MSG
					log_msg("%s -> rcvd DATA seq %lu sidx %ld", __func__, seq, d->socket_info.index);
				#endif

				d->recv_low = d->recv_high = seq;

				//newly added
				d->any_packet_received = 1;

				//can we assume PRODUCER_INDEX(w) is always 0 at this point? I guess so
				SET_ELEMENT_AT(w, 0, idx);

				goto add_to_pending;
			}

			ndp_sequence_number_t seq_diff = seq - d->recv_low;

			if(seq_diff > NDP_MAX_SEQ_DIFF)
			{
				//seq is an earlier sequence number

				d->recv_low = seq;

				if(UNLIKELY (d->recv_high - d->recv_low + 1 >= RING_NUM_ELEMENTS(w)))
				{
					//TODO this should never happen under normal circumstances, but we'll have to check
					//for it, and handle it anyway. Right now, we just crash.

					LOG_FMSG_DO(LOGLVL1, exit(1),
							"%s d->recv_high - d->recv_low > RING_NUM_ELEMENTS(d->received_before_syn)", __func__);
				}

				#if LOG201_REC
					lib_record_event(LOG_RECORD_LIB_DATA_RCVD, lib.instance_info.id, d->socket_info.index,
							seq, lib_read_tsc(), from_main_thread);
				#endif

				#if LOG201_MSG
					log_msg("%s -> rcvd DATA seq %lu sidx %ld", __func__, seq, d->socket_info.index);
				#endif

				//seq_diff = -seq_diff;

				//it was - seq_diff before
				ring_size_t new_pi = GET_PRODUCER_INDEX(w) + RING_NUM_ELEMENTS(w) + seq_diff;

				SET_PRODUCER_INDEX(w, RING_WRAP_INDEX(w, new_pi));

				SET_ELEMENT_AT(w, GET_PRODUCER_INDEX(w), idx);

				///!!!!!!
				//if(d->socket_info.index == 3)
				//	printf("placed %u %u %u\n", seq, idx, GET_PRODUCER_INDEX(w) + seq_diff);

			}
			else if(UNLIKELY (seq_diff + 1 > NDP_INITIAL_RX_WINDOW_SIZE))
			{
				//shouldn't happen under normal operation

				LOG_FMSG(LOGLVL4, "%s -> received future packet with seq %u recv_low %u", __func__, seq, d->recv_low);
				free_rx_pkb(d, idx);
				continue;
			}
			else
			{
				#if LOG201_REC
					lib_record_event(LOG_RECORD_LIB_DATA_RCVD, lib.instance_info.id, d->socket_info.index,
							seq, lib_read_tsc(), from_main_thread);
				#endif

				#if LOG201_MSG
					log_msg("%s -> rcvd DATA seq %lu sidx %ld", __func__, seq, d->socket_info.index);
				#endif

				SET_ELEMENT_AT_WRAPPED(w, GET_PRODUCER_INDEX(w) + seq_diff, idx);


				///!!!!!!
				//if(d->socket_info.index == 3)
				//	printf("placed %u %u %u\n", seq, idx, GET_PRODUCER_INDEX(w) + seq_diff);
			}

		add_to_pending:

			if(LIKELY (PRODUCER_AVAILABLE(d->pending_to_be_acked)))
				PRODUCE(d->pending_to_be_acked, idx)
			else
				LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> !PRODUCER_AVAILABLE(d->pending_to_be_acked)", __func__);

			if(b->hdr.ndp.flags & NDP_HEADER_FLAG_SYN)
			{
				d->syn_received = 1;

				d->recv_isn = seq;

				d->largest_advertised_rwnd_edge = seq + NDP_INITIAL_RX_WINDOW_SIZE;

				//d->asdf_prev = seq;

				d->recv_pkt_seq = seq;
			}
		}
	}

	d->rx_consumer_shadow = cci;

	//!!!!!
	//if(d->socket_info.index == 3)
	//	pprint_rring(d, 40);

	if(d->syn_received)
		return CONSUMER_AVAILABLE(d->pending_to_be_acked);

	return 0;
}


#endif /* NDP_LIB_RECV_AUX_H_ */
