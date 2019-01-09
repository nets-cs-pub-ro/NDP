#include "../common/logger.h"
#include "../common/ring.h"
#include <arpa/inet.h>
#include "helpers.h"
#include "hoover_ndp.h"
#include "recv_aux.h"

ssize_t ndp_recv(int sock, void *dst, size_t len, int flags)
{
	//actually pretty important check
	if(!len)
		return 0;

	uint8_t dont_block = flags & NDP_RECV_DONT_BLOCK;

	struct lib_socket_descriptor *d = &lib.socket_descriptors[sock];
	ssize_t result = 0;

	MY_LOCK_ACQUIRE(d->recv_lock);

	if(UNLIKELY (!d->can_send_recv))
	{
		LOG_FMSG(LOGLVL3, "% -> socket %d is not active", __func__, sock);
		result = -10;
		goto unlock_and_return;
	}

	struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *r = d->socket_info.rx_ring;
	struct RING_BUF(ndp_packet_buffer_index_t) *w = d->rx_wnd_ring;

	ring_size_t new_data_packets = 0;
	//ring_size_t waiting_data_packets = 0;
	ndp_packet_buffer_index_t idx = GET_ELEMENT_AT(w, GET_PRODUCER_INDEX(w));

	if(idx < 0)
	{
		ndp_cycle_count_t ofof;

	one_more_time:
		//TODO doing busy waiting again

		//TODO ofof
		ofof = ndp_rdtsc();

		while(! (new_data_packets = check_for_incoming_data(d, 1)))
		{
			//TODO this may cause receive_window related deadlocks because the following code does not make sense
			//when we don't block (or does it?) :-s
			if(dont_block)
				goto unlock_and_return;

			if(LIKELY(d->syn_received))
			{
				ndp_cycle_count_t ofof2 = ndp_rdtsc();

				if(ofof2 - ofof >= lib.cycles_per_timeout)
				{
					ofof = ofof2;

					d->largest_advertised_rwnd_edge = d->recv_pkt_seq + NDP_INITIAL_RX_WINDOW_SIZE;

					ndp_sequence_number_t aux_seq = d->recv_pkt_seq - 1;

					if(d->recv_pkt_seq != d->recv_isn)
						send_ack(d, ndp_seq_hton(aux_seq), d->largest_advertised_rwnd_edge, 1);
				}
			}

			SPIN_BODY();
		}

		//waiting_data_packets += new_data_packets;

		idx = GET_ELEMENT_AT(w, GET_PRODUCER_INDEX(w));

		if(idx < 0)
		{
			send_pending_acks(d, new_data_packets, 1);

			if(dont_block)
				goto unlock_and_return;
			else
				goto one_more_time;
		}
	}
	//!!!!!!!!!!!!!!!
	else if(UNLIKELY (!d->syn_received))
	{
		if(dont_block)
			goto unlock_and_return;
		else
			goto one_more_time;
	}


	ring_size_t num_depleted_buffers = 0;

	while(idx >= 0)
	{
		//LOG_FMSG(LOGLVL1, "idx : %ld", idx);

		struct ndp_packet_buffer *b = get_rx_packet_buffer(&d->socket_info, idx);

		int sz = ntohs(b->hdr.ip.tot_len) - sizeof(struct iphdr) - sizeof(struct ndp_header);

		int off = d->recv_pkt_off;
		sz -= off;

		if(sz > len)
		{
			sz = len;
			d->recv_pkt_off += sz;
		}
		else
		{
			d->recv_pkt_off = 0;
			d->recv_pkt_seq++;

			num_depleted_buffers++;

			//RING_ARRAY_WRITE(w, ndp_packet_buffer_index_t, 0, -1);
			SET_ELEMENT_AT(w, GET_PRODUCER_INDEX(w), -1);

			//producer_advance(w, sizeof(ndp_packet_buffer_index_t));
			PRODUCER_ADVANCE(w, 1);

			//if(!produce_at_most(r, &idx, sizeof(idx), 1))
			if(LIKELY (PRODUCER_AVAILABLE_ACQUIRE(r)))
				PRODUCE_RELEASE(r, idx)
			else
			{
				//shouldn't happen
				LOG_FMSG(LOGLVL2, "%s -> no room to recycle rx pkb %ld", __func__, idx);
			}

			//RING_ARRAY_READ(w, ndp_packet_buffer_index_t, 0, &idx);
			idx = GET_ELEMENT_AT(w, GET_PRODUCER_INDEX(w));

			//if(d->socket_info.index == 3)
			//	printf("aasdad %d\n", idx);

			//LOG_FMSG(LOGLVL1, "pr av %lu %d", w->producer_index, idx);
		}

		memcpy((char*)dst + result, b->payload + off, sz);

		len -= sz;
		result += sz;

		//LOG_FMSG(LOGLVL1, "sz : %ld", sz);

		if(!len)
			break;
	}

	//if(d->socket_info.index)
		//printf("mw %u %u %u %u %u\n", d->socket_info.index, d->recv_pkt_seq, d->largest_advertised_rwnd_edge, new_data_packets, num_depleted_buffers);

	if(new_data_packets)
		send_pending_acks(d, new_data_packets, 1);
	//TODO it sucks if these ACKs are lost
	//else //if(d->largest_advertised_rwnd_edge <= d->recv_pkt_seq && num_depleted_buffers)
	else if(num_depleted_buffers)
	{
		if(LIKELY(d->syn_received && d->recv_pkt_seq != d->recv_isn))
		{
			d->largest_advertised_rwnd_edge = d->recv_pkt_seq + NDP_INITIAL_RX_WINDOW_SIZE;

			ndp_sequence_number_t aux_seq = d->recv_pkt_seq - 1;

			send_ack(d, ndp_seq_hton(aux_seq), d->largest_advertised_rwnd_edge, 1);
			lctrs.rwnd_acks++;
		}
	}

unlock_and_return:
	MY_LOCK_RELEASE(d->recv_lock);
	return result;
}
