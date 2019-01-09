#include "../common/logger.h"
#include "../common/ring.h"
#include <arpa/inet.h>
#include "defs.h"
#include "helpers.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#ifdef NDP_BUILDING_FOR_XEN
	#include <xengnttab.h>
#endif


int init_lib_socket_descriptor(struct lib_socket_descriptor *d)
{
	size_t x = lib.instance_info.shm_params.num_tx_packet_buffers + 1;

	d->tx_av_pkb_ring = malloc(RING_SIZE(RING_BUF(ndp_packet_buffer_index_t), x));

	if(!d->tx_av_pkb_ring)
		return -1;

	RING_INIT(d->tx_av_pkb_ring, x);

	//***

	//TODO is +1 correct? (there is always 1 unusable position in the ring)
	//x = NDP_TX_PACKET_WINDOW + 1;
	x = lib.instance_info.shm_params.num_tx_packet_buffers + 1;

	d->tx_wnd_ring = malloc(RING_SIZE(RING_BUF(char), x));

	if(!d->tx_wnd_ring)
		return -1;

	RING_INIT(d->tx_wnd_ring, x);

	//***

	//TODO hmmmmm, was NDP_TX_PACKET_WINDOW correct?
	//x = NDP_TX_PACKET_WINDOW * sizeof(ndp_packet_buffer_index_t);
	x = lib.instance_info.shm_params.num_rx_packet_buffers + 1;

	d->rx_wnd_ring = malloc(RING_SIZE(RING_BUF(ndp_packet_buffer_index_t), x));

	if(!d->rx_wnd_ring)
		return -1;

	RING_INIT(d->rx_wnd_ring, x);


	//same size for the following ring
	d->pending_to_be_acked = malloc(RING_SIZE(RING_BUF(ndp_packet_buffer_index_t), x));

	if(!d->pending_to_be_acked)
		return -1;

	RING_INIT(d->pending_to_be_acked, x);

	//

	reset_lib_socket_descriptor(d);

	return 0;
}

void reset_lib_socket_descriptor(struct lib_socket_descriptor *d)
{
	//TODO this is kinda brittle; if the structure member order is changed, bad things
	//are most likely going to happen
	memset(d, 0, offsetof(struct lib_socket_descriptor, socket_info));

	d->tx_pkb_in_use = -1;

	RING_RESET(d->tx_av_pkb_ring);

	RING_RESET(d->tx_wnd_ring);
	for(ring_size_t i = 0; i < RING_NUM_ELEMENTS(d->tx_wnd_ring); i++)
		SET_ELEMENT_AT(d->tx_wnd_ring, i, 0);

	RING_RESET(d->rx_wnd_ring);
	for(ring_size_t i = 0; i < RING_NUM_ELEMENTS(d->rx_wnd_ring); i++)
		SET_ELEMENT_AT(d->rx_wnd_ring, i, -1);

	d->rx_consumer_shadow = 0;

	RING_RESET(d->pending_to_be_acked);

	d->recv_pkt_off = 0;

	const struct ndp_instance_info *instance_info = &lib.instance_info;
	const struct shm_parameters *shm_params = &instance_info->shm_params;
	struct ndp_socket_info *socket_info = &d->socket_info;

	RING_INIT(socket_info->tx_ring, 		shm_params->num_tx_ring_elements);
	RING_INIT(socket_info->rtx_ring, 		shm_params->num_rtx_ring_elements);
	RING_INIT(socket_info->to_rtx_ring, 	shm_params->num_to_rtx_ring_elements);
	RING_INIT(socket_info->ack_tx_ring, 	shm_params->num_ack_tx_ring_elements);
	RING_INIT(socket_info->rx_ring, 		shm_params->num_rx_ring_elements);
	RING_INIT(socket_info->ack_rx_ring, 	shm_params->num_ack_rx_ring_elements);

	ndp_packet_buffer_index_t j;

	//tx bufs
	for(j = 0; j < instance_info->shm_params.num_tx_packet_buffers; j++)
		reset_tx_packet_buffer(d, j);

	//rx bufs
	for(j = 0; j < instance_info->shm_params.num_rx_packet_buffers; j++)
		reset_rx_packet_buffer(d, j);

	//set initial sequence number for transmission
	d->tx_next_seq_number = random();
	d->send_isn = d->tx_next_seq_number;
	d->tx_first_not_ack = d->tx_next_seq_number;

	//what was I thinking?!?
	//d->largest_advertised_rwnd_edge = d->tx_next_seq_number;

	d->remote_rwnd_edge = d->tx_next_seq_number + NDP_INITIAL_RX_WINDOW_SIZE;

	d->tx_pkb_in_use = -1;

	//d->listen_consumer_shadow is reset in ndp_listen()

	//printf("asdf %d %d %d\n", socket_info->index, PRODUCER_AVAILABLE(socket_info->rx_ring), CONSUMER_AVAILABLE(socket_info->rx_ring));

	//TODO ?!?!?!!?
	d->last_send_check_acknto = ndp_rdtsc();
	//d->maxxx_difff = 0;
}

int set_mac_addrs(struct lib_socket_descriptor *d)
{
	struct ndp_socket *s = d->socket_info.socket;
	struct ndp_connection_tuple *t = &s->tuple;

	#if NDP_HARDCODED_SAME_NEXT_HOP
		if(get_mac_address(t->local_addr, s->local_mac, lib.local_addr))
			return -1;
		if(get_mac_address(t->remote_addr, s->remote_mac, lib.local_addr))
			return -2;
	#else
		if(get_mac_address(t->local_addr, s->local_mac))
			return -1;
		if(get_mac_address(t->remote_addr, s->remote_mac))
			return -2;
	#endif

	return 0;
}

void prepare_headers(struct lib_socket_descriptor *d)
{
	struct ndp_socket *s = d->socket_info.socket;
	const struct ndp_connection_tuple *t = &s->tuple;


	//tx pkt buffers
	for(ndp_socket_index_t i = 0; i < lib.instance_info.shm_params.num_tx_packet_buffers; i++)
	{
		struct ndp_packet_buffer *pb = get_tx_packet_buffer(&d->socket_info, i);

		prepare_ndp_packet_headers(t->local_addr, t->remote_addr, t->local_port, t->remote_port,
				s->local_mac, s->remote_mac, &pb->hdr);

		pb->hdr.ndp.flags = NDP_HEADER_FLAG_DATA;
	}

	//ack template
	prepare_ndp_packet_headers(t->local_addr, t->remote_addr, t->local_port, t->remote_port,
					s->local_mac, s->remote_mac, &d->ack_template);

	d->ack_template.ip.tot_len = htons(NDP_CONTROL_PACKET_SIZE);
	d->ack_template.ndp.flags = NDP_HEADER_FLAG_ACK;

	if(d->is_from_connect)
	{
		prepare_ndp_packet_headers(t->local_addr, t->remote_addr, t->local_port, t->remote_port,
				s->local_mac, s->remote_mac, &s->nack_template);
		s->nack_template.ip.tot_len = htons(NDP_CONTROL_PACKET_SIZE);
		s->nack_template.ndp.flags = NDP_HEADER_FLAG_NACK;

		prepare_ndp_packet_headers(t->local_addr, t->remote_addr, t->local_port, t->remote_port,
				s->local_mac, s->remote_mac, &s->pull_template);
		s->pull_template.ip.tot_len = htons(NDP_CONTROL_PACKET_SIZE);
		s->pull_template.ndp.flags = NDP_HEADER_FLAG_PULL;
	}
}

int prepare_connect_socket(struct lib_socket_descriptor *d)
{
	if(set_mac_addrs(d))
		return -1;

	d->is_from_connect = 1;

	return prepare_accept_socket(d);
}

int prepare_accept_socket(struct lib_socket_descriptor *d)
{
	prepare_headers(d);

	//TODO is this necessary here?
	RELEASE_FENCE();

	d->can_send_recv = 1;
	return 0;
}

void reset_tx_packet_buffer(struct lib_socket_descriptor *d, ndp_packet_buffer_index_t idx)
{
	struct ndp_packet_buffer *buf = get_tx_packet_buffer(&d->socket_info, idx);

	//is this sync really necessary? (should I actually care about this race?)
	MY_LOCK_ACQUIRE(buf->sync_helper);

	memset(&buf->flags, 0, sizeof(struct ndp_packet_buffer_flags));

	MY_LOCK_RELEASE(buf->sync_helper);

	buf->hdr.ndp.flags = NDP_HEADER_FLAG_DATA;

	buf->bytes_left = lib.instance_info.shm_params.packet_buffer_mtu -
			(sizeof(struct iphdr) + sizeof(struct ndp_header));

	//if(!produce_at_most(d->tx_av_pkb_ring, &idx, sizeof(idx), 1))
	//	LOG_FMSG(LOGLVL2, "%s -> !produce_at_most(d->tx_av_pkb_ring, &idx, sizeof(idx), 1)", __func__);

	if(LIKELY (PRODUCER_AVAILABLE_ACQUIRE(d->tx_av_pkb_ring)))
		PRODUCE_RELEASE(d->tx_av_pkb_ring, idx)
	else
		LOG_FMSG(LOGLVL2, "%s -> !PRODUCER_AVAILABLE(d->tx_av_pkb_ring)", __func__);
}

void reset_rx_packet_buffer(struct lib_socket_descriptor *d, ndp_packet_buffer_index_t idx)
{
	struct ndp_packet_buffer *buf = get_rx_packet_buffer(&d->socket_info, idx);

	memset(&buf->flags, 0, sizeof(struct ndp_packet_buffer_flags));

	//if(!produce_at_most(d->socket_info.rx_ring, &idx, sizeof(idx), 1))
	//	LOG_FMSG(LOGLVL2, "%s -> !produce_at_most(d->socket_info->rx_ring, &idx, sizeof(idx), 1)", __func__);

	if(LIKELY (PRODUCER_AVAILABLE(d->socket_info.rx_ring)))
		PRODUCE(d->socket_info.rx_ring, idx)
	else
		LOG_FMSG(LOGLVL2, "%s -> !PRODUCER_AVAILABLE(d->socket_info.rx_ring)", __func__);
}

struct lib_socket_descriptor* get_available_socket()
{
	//ndp_socket_index_t aux;

	//if(consume_at_most(lib_data.av_sock_ring, &aux, sizeof(ndp_socket_index_t)))
	//	return &lib_data.socket_descriptors[aux];

	if(CONSUMER_AVAILABLE(lib.av_sock_ring))
		return &lib.socket_descriptors[CONSUME(lib.av_sock_ring)];

	return NULL;
}

ring_size_t comm_ring_write(ndp_instance_op_t type, ndp_socket_index_t socket_index)
{
	struct ndp_instance_op_msg msg;
	msg.type = type;
	msg.socket_index = socket_index;

	//return produce_at_most(lib_data.instance_info.comm_ring, &msg, sizeof(msg), 1);
	if(LIKELY (PRODUCER_AVAILABLE_ACQUIRE(lib.instance_info.comm_ring)))
	{
		PRODUCE_RELEASE(lib.instance_info.comm_ring, msg);
		return 1;
	}

	return 0;
}


/*
ndp_timestamp_ns_t lib_get_ts()
{
	struct timespec tspc;

	if(UNLIKELY(clock_gettime(CLOCK_MONOTONIC_RAW, &tspc)))
	{
		printf("clock_gettime error: %s\n", strerror(errno));
		exit(1);
	}

	return tspc.tv_sec * (1000 * 1000 * 1000) + tspc.tv_nsec;
}
*/

static inline void update_remote_rwnd_edge(struct lib_socket_descriptor *d, ndp_sequence_number_t seq,
		ndp_recv_window_t recv)
{
	//TODO maybe add a maximum possible window increase check?
	ndp_sequence_number_t aux_seq = seq + recv;

	//printf("update %u %u %u %u\n", seq, recv, d->remote_rwnd_edge, aux_seq - d->remote_rwnd_edge);

	if(LIKELY (aux_seq - d->remote_rwnd_edge <= NDP_MAX_SEQ_DIFF))
	{
		//window increase
		d->remote_rwnd_edge = aux_seq;
	}
	else
	{
		//TODO will this be generally triggered by reordering?
		//LOG_MSG(LOGLVL4, "strange receive window update");
		//exit(1);
	}

	//printf("sidx %u isn %u new w_edge %u diff %u fnack %u\n",
		//	d->socket_info.index, d->send_isn, d->remote_rwnd_edge, d->remote_rwnd_edge - d->send_isn, d->tx_first_not_ack);
}

//TODO maybe inline

//TODO currently, a kind of lock must be held when running this function between the main thread and the timeout thread
//maybe put the locking inside the function, just to be sure nobody forgets this at some point
//also maybe lock per packet buffer ... dunno
void check_acks(struct lib_socket_descriptor *d, int from_main_thread)
{
	//ndp_cycle_count_t pseudo_now = lib_read_tsc();

	struct ndp_header aux_h;

	//while(consume_at_most(d->socket_info.ack_rx_ring, &aux_h, sizeof(struct ndp_header)))
	while(CONSUMER_AVAILABLE_ACQUIRE(d->socket_info.ack_rx_ring))
	{
		aux_h = CONSUME_RELEASE(d->socket_info.ack_rx_ring);

		char is_ack = 1;

		if(aux_h.flags & NDP_HEADER_FLAG_NACK)
			is_ack = 0;
		else
			lctrs.acks_totrx++;

		ndp_sequence_number_t a_seq = ndp_seq_ntoh(aux_h.sequence_number);
		ndp_sequence_number_t b_seq = d->tx_first_not_ack;
		//ndp_sequence_number_t d_seq = a_seq > b_seq ? a_seq - b_seq : b_seq - a_seq;
		ndp_sequence_number_t d_seq = a_seq - b_seq;

		if(d_seq > NDP_MAX_SEQ_DIFF)
		{
			//old (n)ack
			if(is_ack)
			{
				#if LOG201_REC
					lib_record_event(LOG_RECORD_LIB_OLD_ACK_RCVD, lib.instance_info.id, d->socket_info.index,
							a_seq, lib_read_tsc(), from_main_thread);
				#endif

				#if LOG201_MSG
					log_msg("%s -> rcvd OLD ACK seq %lu RWND %lu", __func__, a_seq, ndp_rwnd_ntoh(aux_h.recv_window));
				#endif

				update_remote_rwnd_edge(d, a_seq, ndp_rwnd_ntoh(aux_h.recv_window));

				lctrs.old_acks++;
			}
			else
			{
				#if LOG203_REC
					lib_record_event(LOG_RECORD_LIB_OLD_NACK_RCVD, lib.instance_info.id, d->socket_info.index,
							a_seq, lib_read_tsc(), from_main_thread);
				#endif

				#if LOG203_MSG
					log_msg("%s -> rcvd OLD NACK seq %lu", __func__, a_seq);
				#endif

				lctrs.old_nacks++;
			}

			continue;
		}
		//else if(d_seq >= NDP_TX_PACKET_WINDOW)
		else if(a_seq - d->tx_next_seq_number <= NDP_MAX_SEQ_DIFF)
		{
			//"future" (n)ack (outside of pkt window) ... shouldn't really happen
			if(is_ack)
				LOG_FMSG_DO(LOGLVL4, continue, "%s -> rcvd future ACK  seq %u tx_first_not_ack %lu tx_next_seq %u sidx %d",
						__func__, a_seq - d->send_isn, b_seq - d->send_isn, d->tx_next_seq_number - d->send_isn, d->socket_info.index)
			else
				LOG_FMSG_DO(LOGLVL4, continue, "%s -> rcvd future NACK seq %u tx_first_not_ack %lu tx_next_seq %u sidx %d",
						__func__, a_seq - d->send_isn, b_seq - d->send_isn, d->tx_next_seq_number - d->send_isn, d->socket_info.index);
		}

		ndp_packet_buffer_index_t pkb_idx = find_tx_pkb_for_seq(&d->socket_info, lib.instance_info.shm_params.num_tx_packet_buffers,
																aux_h.sequence_number);

		if(is_ack)
		{
			#if LOG201_REC
				lib_record_event(LOG_RECORD_LIB_ACK_RCVD, lib.instance_info.id, d->socket_info.index,
						a_seq, lib_read_tsc(), from_main_thread);
			#endif

			#if LOG201_MSG
				log_msg("%s -> rcvd ACK seq %lu sidx %ld", __func__, a_seq - d->send_isn, d->socket_info.index);
			#endif

			//if(a_seq - d->send_isn >= 4100)
			//	log_msg("zzzz rcvd ACK seq %lu sidx %ld", a_seq - d->send_isn, d->socket_info.index);


			update_remote_rwnd_edge(d, a_seq, ndp_rwnd_ntoh(aux_h.recv_window));


			if(UNLIKELY (pkb_idx < 0))
			{
				lctrs.ack_no_tx_pkb++;
				/*LOG_FMSG(LOGLVL3, "%s -> cannot find tx pkb for ACK seq %lu tx_first_not_ack %lu X %d sidx %d",
						__func__, a_seq - d->send_isn, b_seq - d->send_isn,
						GET_ELEMENT_AT_WRAPPED(d->tx_wnd_ring, GET_PRODUCER_INDEX(d->tx_wnd_ring) + d_seq), d->socket_info.index)*/
			}
			else
			{
				//zzzzzzzzzzzzzzzzz
				if(LIKELY (d->got_first_ack))
				{
					ndp_sequence_number_t ack_seq_diff = a_seq - d->zmost_recent_ack;

					if(LIKELY (ack_seq_diff <= NDP_MAX_SEQ_DIFF && ack_seq_diff))
					{
						lctrs.ack_holes += ack_seq_diff - 1;
						d->zmost_recent_ack = a_seq;
					}
				}
				else
				{
					d->got_first_ack = 1;
					d->zmost_recent_ack = a_seq;
					lctrs.ack_holes += a_seq - d->send_isn;
				}
				//zzzzzzzzzzzzzzzzz


				reset_tx_packet_buffer(d, pkb_idx);

				//if(d_seq > d->maxxx_difff)
				//{
				//	d->maxxx_difff = d_seq;
				//	LOG_FMSG(1, "new maxxx difff %lu", d_seq);
				//}

				//RING_ARRAY_WRITE(d->tx_wnd_ring, char, d_seq, 1);
				SET_ELEMENT_AT_WRAPPED(d->tx_wnd_ring, GET_PRODUCER_INDEX(d->tx_wnd_ring) + d_seq, 1);

				while(1)
				{
					//RING_ARRAY_READ(d->tx_wnd_ring, char, 0, &aux_c);

					char aux_c = GET_ELEMENT_AT(d->tx_wnd_ring, GET_PRODUCER_INDEX(d->tx_wnd_ring));


					if(aux_c)
					{
						//RING_ARRAY_WRITE(d->tx_wnd_ring, char, 0, 0);
						SET_ELEMENT_AT(d->tx_wnd_ring, GET_PRODUCER_INDEX(d->tx_wnd_ring), 0);

						//producer_advance(d->tx_wnd_ring, 1);
						PRODUCER_ADVANCE(d->tx_wnd_ring, 1);

						d->tx_first_not_ack++;
					}
					else
						break;
				}
			}
		}
		else
		{
			//ndp_cycle_count_t tsc = lib_read_tsc();

			#if LOG203_REC
				lib_record_event(LOG_RECORD_LIB_NACK_RCVD, lib.instance_info.id, d->socket_info.index,
						a_seq, lib_read_tsc(), from_main_thread);
			#endif

			if(UNLIKELY (pkb_idx < 0))
			{
				//LOG_FMSG(LOGLVL3, "%s -> cannot find tx pkb for NACK seq %lu tx_first_not_ack %lu sidx %d",
				//		__func__, a_seq - d->send_isn, b_seq - d->send_isn, d->socket_info.index)
				lctrs.nack_no_tx_pkb++;
			}
			else
			{
				struct ndp_packet_buffer *pkb = get_tx_packet_buffer(&d->socket_info, pkb_idx);
				pkb->timeout_ref = 0;
			}

			#if LOG203_MSG
				log_msg("%s -> rcvd NACK seq %lu sidx %ld", __func__, a_seq, d->socket_info.index);
			#endif

		}
	}
}

void lib_output_file_name(char *buf, size_t len, const char *middle)
{
	char suffix[50];
	//TODO this format pretty much assumes local_addr is IPv4
	sprintf(suffix, "_%08x.dat", lib.local_addr);

	if(!getcwd(buf, len))
		LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> getcwd error", __func__);

	if(strlen(buf) + strlen(middle) + strlen(suffix) >= len - 1)
		LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> full name too long", __func__);


	sprintf(buf + strlen(buf), "%s%s", middle, suffix);
}


#ifdef NDP_BUILDING_FOR_XEN

	void free_alloc_for_grant(xengntshr_handle *gh, const struct alloc_for_grant_result *r)
	{
		if(xengntshr_unshare(gh, r->mem, r->num_pages))
			printf("free unshare mem err\n");
		if(xengntshr_unshare(gh, r->ref_chain, num_meta_pages(r->num_pages)))
			printf("free unshare ref_chain err\n");

		//if(xengntshr_close(gh->h))
		//	printf("free gntshr_close err\n");
	}

#endif


