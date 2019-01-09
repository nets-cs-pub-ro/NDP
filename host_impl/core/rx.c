#include "../common/helpers.h"
#include <arpa/inet.h>
#include "helpers.h"
#include <rte_cycles.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>


static inline void add_pull_req_aux(struct core_socket_descriptor *d, struct RING_BUF_OF_CHOICE(struct, core_socket_descriptor, ptr) *q)
{
#if NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
	if(LIKELY (PRODUCER_AVAILABLE_ACQUIRE(q)))
		PRODUCE_RELEASE(q, d)
#else
	if(LIKELY (PRODUCER_AVAILABLE(q)))
		PRODUCE(q, d)
#endif
	else
		LOG_FMSG(LOGLVL4, "%s -> !produce_at_most(q, &d, sizeof(&d), 1)", __func__);
}

static inline void add_pull_req(struct core_socket_descriptor *d)
{
#if NDP_CORE_PRIOPULLQ_4COSTIN
	if(d->socket_info.index == 0)
	{
		//LOG_MSG(1, "aaa");
		add_pull_req_aux(d, core.pullq2);
	}
	else
		add_pull_req_aux(d, core.pullq);
#else
	add_pull_req_aux(d, core.pullq);
#endif
}

static inline struct core_socket_descriptor* incoming_find_socket(struct ndp_packet_headers *h)
{
	struct ndp_connection_tuple t;

	t.local_addr = h->ip.daddr;
	t.local_port = h->ndp.dst_port;
	t.remote_addr = h->ip.saddr;
	t.remote_port = h->ndp.src_port;

	int left = core.num_active_sockets + core.num_passive_sockets;

	struct core_socket_descriptor *found_passive = NULL;

	for(int i = 0; left && i < NDP_CORE_MAX_SOCK_DESC; i++)
	{
		if(core.socket_descriptor_available[i])
			continue;

		struct core_socket_descriptor *d = &core.socket_descriptors[i];

		//TODO careful with memcmp if it remains in use and the ndp_connection_tuple struct has gaps :-s

		if(d->socket_is_active)
			if(!memcmp(&t, &d->socket_info.socket->tuple, sizeof(t)))
				return d;

		if(d->socket_is_passive)
			if(!memcmp(&t, &d->socket_info.socket->tuple, offsetof(struct ndp_connection_tuple, remote_addr)))
				found_passive = d;
	}

	return found_passive;
}


//TODO if there no room to send the NACK packet, then it will be forgotten; this must be fixed
static inline void send_nack(struct core_socket_descriptor *d, ndp_sequence_number_t seq_in_network_order)
{
	struct ndp_socket *s = d->socket_info.socket;

	//s->control_packet_template.ndp.flags = NDP_HEADER_FLAG_NACK;
	s->nack_template.ndp.sequence_number = seq_in_network_order;

	//s->nack_template.ndp.checksum = nack_seq_checksum(seq_in_network_order);

	send_one_packet(&s->nack_template, sizeof(struct ndp_packet_headers), core.dpdk_default_queue_id);

	#if LOG202_REC
		core_record_event(LOG_RECORD_CORE_NACK_SENT, d->instance_info->id, d->socket_info.index,
				ndp_seq_ntoh(seq_in_network_order), core_read_tsc());
	#endif

	#if LOG202_MSG
	  log_msg("%s -> sent NACK seq %lu iid %ld sidx %ld", __func__, ndp_seq_ntoh(seq_in_network_order),
			  d->instance_info->id, d->socket_info.index);
	#endif
}



#define RX_BURST_SIZE		32
static struct rte_mbuf *bufs[RX_BURST_SIZE];


void check_rx(void)
{
	//(ip hdr + ndp hdr) length in network order
	uint16_t two_hdrs_len = htons(sizeof(struct iphdr) + sizeof(struct ndp_header));

	uint16_t rcvd = rte_eth_rx_burst(core.dpdk_port_id, core.dpdk_default_queue_id, bufs, RX_BURST_SIZE);

	if(!rcvd)
		return;

	//LOG_FMSG(LOGLVL100, "%s -> rte_eth_rx_burst rcvd %d", __func__, rcvd);

	for(uint16_t i = 0; i < rcvd; i++)
	{
		//LOG_FMSG(LOGLVL100, "nb_segs %d", bufs[i]->nb_segs);

		char new_incoming_connection = 0;

		struct ndp_packet_headers *h = rte_pktmbuf_mtod(bufs[i], struct ndp_packet_headers*);

		if(UNLIKELY(h->eth.ether_type != htons(NDP_ETHERTYPE)))
		{
			//LOG_FMSG_DO(LOGLVL100, goto free_mbuf, "%s -> h->eth.ether_type (%d) != NDP_ETHERTYPE", __func__, ntohs(h->eth.ether_type));
			goto free_mbuf;
		}

		if(UNLIKELY(h->ip.protocol != NDP_PROTOCOL_NUMBER))
			LOG_FMSG_DO(    0      , goto free_mbuf, "%s -> h->ip.protocol != NDP_PROTOCOL_NUMBER (%d)", __func__, h->ip.protocol);
			//LOG_FMSG_DO(LOGLVL100, goto free_mbuf, "%s -> h->ip.protocol != NDP_PROTOCOL_NUMBER (%d)", __func__, h->ip.protocol);


		struct core_socket_descriptor *d = incoming_find_socket(h);

		if(UNLIKELY(!d))
		{
			//LOG_FMSG(LOGLVL4, "%s -> could not identify socket for incoming packet", __func__);
			LOG_FMSG(     0   , "%s -> could not identify socket for incoming packet", __func__);


			//log_packet(h);
			//log_mbuf_dump(bufs[i], 20);

			cctrs.no_socket_found++;

			goto free_mbuf;
		}

		if(d->socket_is_passive)
		{
			ndp_socket_index_t idx;

			//if(UNLIKELY(!consume_at_most(d->socket_info.tx_ring, &idx, sizeof(idx))))
			if(LIKELY (CONSUMER_AVAILABLE(d->socket_info.listen_ring)))
				idx = CONSUME_RELEASE(d->socket_info.listen_ring);
			else
				LOG_FMSG_DO(LOGLVL3, goto free_mbuf, "%s -> no available socket for new incoming connection", __func__);

			struct ndp_instance_info *instance_info = d->instance_info;

			d = get_socket_descriptor(instance_info, idx);
			struct ndp_socket *s = d->socket_info.socket;
			struct ndp_connection_tuple *t = &s->tuple;

			memcpy(s->local_mac, &h->eth.d_addr, 6);
			memcpy(s->remote_mac, &h->eth.s_addr, 6);

			t->local_addr = h->ip.daddr;
			t->local_port = h->ndp.dst_port;
			t->remote_addr = h->ip.saddr;
			t->remote_port = h->ndp.src_port;

			//TODO this part can probably be moved to the lib via ndp_accept; also this logic should be placed in one method really;
			//right now the code is both here for incoming connections, and in lib/helpers.c->prepare_headers(...) for outgoing connections
			prepare_ndp_packet_headers(t->local_addr, t->remote_addr, t->local_port, t->remote_port, s->local_mac,
					s->remote_mac, &s->nack_template);
			s->nack_template.ip.tot_len = htons(NDP_CONTROL_PACKET_SIZE);
			s->nack_template.ndp.flags = NDP_HEADER_FLAG_NACK;

			prepare_ndp_packet_headers(t->local_addr, t->remote_addr, t->local_port, t->remote_port, s->local_mac,
					s->remote_mac, &s->pull_template);
			s->pull_template.ip.tot_len = htons(NDP_CONTROL_PACKET_SIZE);
			s->pull_template.ndp.flags = NDP_HEADER_FLAG_PULL;

			//s->pull_template.ndp.flags = NDP_HEADER_FLAG_PULL;

			prepare_socket_descriptor(instance_info, 1, d);

			new_incoming_connection = 1;

			//tempLOG_FMSG(LOGLVL102, "%s -> new incoming connection bound to socket_index %d", __func__, idx);
		}

		//TODO maybe check flag sanity

		ndp_header_flags_t nflags = h->ndp.flags;

		if(nflags & (NDP_HEADER_FLAG_DATA | NDP_HEADER_FLAG_CHOPPED))
		{
			#if NDP_CORE_NACKS_FOR_HOLES
				ndp_sequence_number_t incoming_ndp_seq = ndp_seq_ntoh(h->ndp.sequence_number);
				ndp_sequence_number_t seq_diff = incoming_ndp_seq - d->last_recv_seq_number;
			#endif

			//check if it's a chopped header
			//TODO should check using CHOPPED flag after switch supports it
			if(h->ip.tot_len == two_hdrs_len)
			{
				cctrs.rcvd_chop++;

				#if LOG205_REC
					core_record_event(LOG_RECORD_CORE_HDR_RCVD, d->instance_info->id, d->socket_info.index,
							ndp_seq_ntoh(h->ndp.sequence_number), core_read_tsc());
				#endif

				#if LOG205_MSG
					log_msg("%s -> rcvd HEADER "NDP_PACKET_HDRS_FORMAT" sidx %ld", __func__, NDP_PACKET_HDRS_VALUES(h),
							d->socket_info.index);//, idx);
				#endif


				#if NDP_CORE_NACKS_FOR_HOLES
					if(LIKELY (d->got_at_least_one_data_packet))
					{
						if(LIKELY (seq_diff <= NDP_MAX_SEQ_DIFF))
						{
							ring_size_t ii = GET_PRODUCER_INDEX(d->n4h_ring);
							if(GET_ELEMENT_AT_WRAPPED(d->n4h_ring, ii + seq_diff))
								goto free_mbuf;
							else
								SET_ELEMENT_AT_WRAPPED(d->n4h_ring, ii + seq_diff, 1);
						}
					}
				#endif

				send_nack(d, h->ndp.sequence_number);

				add_pull_req(d);

				goto free_mbuf;
			}

			//-----------------------


			#if NDP_CORE_NACKS_FOR_HOLES

				if(LIKELY (d->got_at_least_one_data_packet))
				{
					if(LIKELY (seq_diff <= NDP_MAX_SEQ_DIFF))
					{
						if(UNLIKELY (seq_diff >= NDP_CORE_NUM_N4H_RING_ELEMENTS - 1))
							LOG_MSG(1, "noooooooooooooooooooooooooooooooooooooooooooooooooooooooooo");

						if(UNLIKELY(seq_diff > cctrs.big_hole))
							cctrs.big_hole = seq_diff;

						ring_size_t ii = GET_PRODUCER_INDEX(d->n4h_ring);

						SET_ELEMENT_AT(d->n4h_ring, ii, 0);

						for(unsigned int nacki = 1; nacki < seq_diff; nacki++)
						{
							if(GET_ELEMENT_AT_WRAPPED(d->n4h_ring, ii + nacki))
							{
								SET_ELEMENT_AT_WRAPPED(d->n4h_ring, ii + nacki, 0);
							}
							else
							{
								add_pull_req(d);

								#if LOG200_REC
									core_record_event(LOG_RECORD_CORE_HDR_IMPL, d->instance_info->id, d->socket_info.index,
										d->last_recv_seq_number + nacki, core_read_tsc());
								#endif

								send_nack(d, ndp_seq_hton(d->last_recv_seq_number + nacki));

								cctrs.nacks4holes++;
							}
						}

						ring_size_t new_pi = RING_WRAP_INDEX(d->n4h_ring, ii + seq_diff);

						SET_ELEMENT_AT(d->n4h_ring, new_pi, 1);
						SET_PRODUCER_INDEX(d->n4h_ring, new_pi);

						d->last_recv_seq_number = incoming_ndp_seq;
					}
					//else
					//	LOG_MSG(LOGLVL1, "NDP_CORE_NACKS_FOR_HOLES seq_diff <= NDP_MAX_SEQ_DIFF");
				}
				else
				{
					d->got_at_least_one_data_packet = 1;
					d->last_recv_seq_number = incoming_ndp_seq;
					SET_ELEMENT_AT(d->n4h_ring, GET_PRODUCER_INDEX(d->n4h_ring), 1);
				}

			#endif





			//------------------------

			ndp_packet_buffer_index_t idx;
			struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *r = d->socket_info.rx_ring;

			ring_size_t ci = GET_CONSUMER_INDEX(r);

			if(LIKELY (CONSUMER_AVAILABLE_ACQUIRE(r)))
				//RING_ARRAY_READ_FROM(r, ndp_packet_buffer_index_t, r->consumer_index, 0, &idx)
				idx = GET_ELEMENT_AT(r, ci);
			else
			{
				//TODO should not really happen; if it does happen, maybe send a NACK

				//LOG_FMSG_DO(LOGLVL4, goto free_mbuf, "%s -> sidx %ld no available pkbuf for incoming packet (ci %lu)", __func__, d->socket_info.index, ci);

				cctrs.no_av_pkbuf++;
				goto free_mbuf;
			}

			#if LOG200_REC
				core_record_event(LOG_RECORD_CORE_DATA_RCVD, d->instance_info->id, d->socket_info.index,
						ndp_seq_ntoh(h->ndp.sequence_number), core_read_tsc());
			#endif

			#if LOG200_MSG
			  log_msg("%s -> rcvd DATA "NDP_PACKET_HDRS_FORMAT" sidx %ld pbidx %d", __func__, NDP_PACKET_HDRS_VALUES(h),
							d->socket_info.index, idx);
			#endif

			struct ndp_packet_buffer *b = get_rx_packet_buffer(&d->socket_info, idx);

			/*int Jc = 0;
			for(unsigned int J = 0; J < rte_pktmbuf_pkt_len(bufs[i]); J++)
			{
				if(Jc % 20 == 0)
					printf("%05u: ", Jc / 20);
				printf("%02x ", ((unsigned char*)h)[J]);
				Jc++;
				if(Jc % 20 == 0)
					printf("\n");
			}
			printf("\n");*/

			memcpy(&b->hdr, h, rte_pktmbuf_pkt_len(bufs[i]));

			//consumer_advance(r, sizeof(idx));
			RELEASE_FENCE();
			SET_CONSUMER_INDEX(r, RING_WRAP_INDEX(r, ci + 1));

			add_pull_req(d);
		}
		else if(nflags & NDP_HEADER_FLAG_ACK)
		{
			#if LOG200_REC
				core_record_event(LOG_RECORD_CORE_ACK_RCVD, d->instance_info->id, d->socket_info.index,
						ndp_seq_ntoh(h->ndp.sequence_number), core_read_tsc());
			#endif

			#if LOG200_MSG
				log_msg("%s -> rcvd ACK seq %lu iid %ld sidx %ld", __func__, ndp_seq_ntoh(h->ndp.sequence_number),
						d->instance_info->id, d->socket_info.index);
			#endif


			struct RING_BUF_OF_CHOICE(struct, ndp_header) *r = d->socket_info.ack_rx_ring;

			//if(!produce_at_most(r, &h->ndp, sizeof(struct ndp_header), 1))
			if(LIKELY (PRODUCER_AVAILABLE(r)))
				PRODUCE_RELEASE(r, h->ndp)
			else
				LOG_FMSG(LOGLVL4, "%s -> no room for incoming ACK", __func__);
		}
		else if(nflags & NDP_HEADER_FLAG_NACK)
		{

			#if LOG202_REC
				core_record_event(LOG_RECORD_CORE_NACK_RCVD, d->instance_info->id, d->socket_info.index,
						ndp_seq_ntoh(h->ndp.sequence_number), core_read_tsc());
			#endif

			#if LOG202_MSG
				log_msg("%s -> rcvd NACK seq %lu iid %ld sidx %ld", __func__, ndp_seq_ntoh(h->ndp.sequence_number),
						d->instance_info->id, d->socket_info.index);
			#endif


			//if(UNLIKELY (h->ndp.checksum != nack_seq_checksum(h->ndp.sequence_number)))
			//	LOG_MSG(LOGLVL1, "!!!!! bad NACK checksum");

			ndp_packet_buffer_index_t idx = find_tx_pkb_for_seq(&d->socket_info, d->instance_info->shm_params.num_tx_packet_buffers,
							h->ndp.sequence_number);

			if(UNLIKELY(idx < 0))
			{
				//LOG_FMSG_DO(LOGLVL4, exit(1), "%s -> cannot find pkb idx for NACK seq %lu", __func__, ndp_seq_ntoh(h->ndp.sequence_number))
				//free_mbuf instead of exit

				LOG_FMSG(LOGLVL4, "%s -> cannot find pkb idx for NACK seq %lu csum %u actual_csum %u", __func__,
						ndp_seq_ntoh(h->ndp.sequence_number), nack_seq_checksum(h->ndp.sequence_number), h->ndp.checksum);

				cctrs.no_pkb4nack++;

				goto free_mbuf;
			}


			struct ndp_packet_buffer *the_pkb_for_this_nack = get_tx_packet_buffer(&d->socket_info, idx);
			the_pkb_for_this_nack->flags.core_sent_to_nic = 0;


			//if(UNLIKELY(!produce_at_most(d->socket_info.rtx_ring, &idx, sizeof(idx), 1)))
			if(LIKELY (PRODUCER_AVAILABLE(d->socket_info.rtx_ring)))
				PRODUCE(d->socket_info.rtx_ring, idx)
			else
				LOG_FMSG(LOGLVL3, "%s -> no room in rtx_ring", __func__);

			//if(UNLIKELY(!produce_at_most(d->socket_info.ack_rx_ring, &h->ndp, sizeof(struct ndp_header), 1)))
			if (LIKELY (PRODUCER_AVAILABLE_ACQUIRE(d->socket_info.ack_rx_ring)))
				PRODUCE_RELEASE(d->socket_info.ack_rx_ring, h->ndp)
			else
				LOG_FMSG(LOGLVL4, "%s -> no room for incoming NACK", __func__);
		}
		else if(nflags & NDP_HEADER_FLAG_PULL)
		{
			ndp_pull_number_t diff = ndp_pull_ntoh(h->ndp.pull_number) - d->highest_pull_rcvd;

			//fancy way of saying tx_alw_inc = (diff <= NDP_MAX_SEQ_DIFF) * diff
			ndp_pull_number_t tx_alw_inc = diff > NDP_MAX_SEQ_DIFF;
			tx_alw_inc = (tx_alw_inc - 1) & diff;

			d->tx_allowance += tx_alw_inc;
			d->highest_pull_rcvd += tx_alw_inc;

			#if LOG200_REC
				core_record_event(LOG_RECORD_CORE_PULL_RCVD, d->instance_info->id, d->socket_info.index,
						ndp_pull_ntoh(h->ndp.pull_number), core_read_tsc());
			#endif

			#if LOG200_MSG
				log_msg("%s -> rcvd PULL "NDP_PACKET_HDRS_FORMAT" sidx %ld alw_inc %u", __func__,
						NDP_PACKET_HDRS_VALUES(h), d->socket_info.index, tx_alw_inc);
			#endif

		}
		else
			LOG_FMSG(LOGLVL3, "%s -> weird ndp flags %04x", __func__, nflags);



	free_mbuf:

		//is it really that unlikely?
		if(UNLIKELY(new_incoming_connection))
		{
			RELEASE_FENCE(); //needed?
			d->socket_info.socket->flags.core_connection_can_be_accepted = 1;
			//tempLOG_FMSG(LOGLVL102, "%s -> connection can be accepted", __func__);
		}

		rte_pktmbuf_free(bufs[i]);
	}
}
