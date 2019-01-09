#include "../common/logger.h"
#include "../common/ring.h"
#include <arpa/inet.h>
#include "helpers.h"
#include "hoover_ndp.h"
#include "timeout.h"


ssize_t ndp_send(int sock, const void *src, size_t len, int flags)
{
	//actually pretty important check
	if(!len)
		return 0;

	struct lib_socket_descriptor *d = &lib.socket_descriptors[sock];
	ssize_t result = 0;

	MY_LOCK_ACQUIRE(d->send_lock);

	if(UNLIKELY (!d->can_send_recv))
	{
		LOG_FMSG(LOGLVL3, "% -> socket %d is not active", __func__, sock);
		result = -10;
		goto unlock_and_return;
	}

	//*******aldjalkdjlkasjdkalsjdalskjdaldjadlksdkaljd


	//TODO this probably doesn't really do much / or does it?
	ndp_cycle_count_t now_cycles = ndp_rdtsc();

	if(now_cycles - d->last_send_check_acknto >= lib.cycles_per_timeout)
	{
		d->last_send_check_acknto = now_cycles;

		check_acks(d, 1);
		check_for_timeouts(d, 1);
	}

	//****askldjaklsdjaslkjdalsjdlasjkdad

	size_t left = len;

	/* Currently, we don't return until everything can be placed in tx pkt_buffers. If large amounts
	of data are sent, this means we actually wait for ACKs & stuff. */
	while(left)
	{
		//printf("ndp_send left %lu %u %u\n", left, d->send_isn, d->remote_rwnd_edge - d->tx_next_seq_number);

		int pkb_was_not_in_use = 1;

		if(d->tx_pkb_in_use < 0)
		{
			//TODO busyyy

			//TODO ofof
			ndp_cycle_count_t asdff = ndp_rdtsc();

			while(UNLIKELY (d->tx_next_seq_number == d->remote_rwnd_edge ||
					d->tx_next_seq_number >= d->tx_first_not_ack + lib.instance_info.shm_params.num_tx_packet_buffers)) //!CONSUMER_AVAILABLE(d->tx_av_pkb_ring)))
			{
				check_acks(d, 1);
				check_for_timeouts(d, 1);

				ndp_cycle_count_t ehh = ndp_rdtsc();
				if(UNLIKELY (ehh - asdff > lib.cycles_per_timeout)) //* 500)
				{
					asdff = ehh;
					LOG_FMSG(1, "eh %u %u %u %u", d->tx_next_seq_number, d->remote_rwnd_edge, d->tx_first_not_ack, CONSUMER_AVAILABLE(d->tx_av_pkb_ring));
				}

				if((flags & NDP_SEND_DONT_BLOCK) && (d->tx_next_seq_number == d->remote_rwnd_edge ||
					d->tx_next_seq_number >= d->tx_first_not_ack + lib.instance_info.shm_params.num_tx_packet_buffers))
						goto unlock_and_return;
			}

			//volatile ndp_sequence_number_t *first_not_ack = &d->tx_first_not_ack;
			//while(d->tx_next_seq_number - *first_not_ack >= NDP_TX_PACKET_WINDOW)
				//SPIN_BODY();

			//TODO another easy way out via busy waiting
			//while(!consume_at_most(d->tx_av_pkb_ring, &idx, sizeof(idx)))
			//while(UNLIKELY (!CONSUMER_AVAILABLE(d->tx_av_pkb_ring)))
			//	check_acks(d, 1);

			d->tx_pkb_in_use = CONSUME(d->tx_av_pkb_ring);
		}
		else
			pkb_was_not_in_use = 0;

		struct ndp_packet_buffer *pb = get_tx_packet_buffer(&d->socket_info, d->tx_pkb_in_use);

		if(pkb_was_not_in_use)
		{
			pb->flags.lib_in_use = 1;
			pb->hdr.ndp.sequence_number = ndp_seq_hton(d->tx_next_seq_number);

			if(UNLIKELY (!d->syn_sent))
			{
				pb->hdr.ndp.flags |= NDP_HEADER_FLAG_SYN;
				d->syn_sent = 1;
			}

			d->tx_next_seq_number++;
		}

		ndp_packet_buffer_size_t min = pb->bytes_left;

		if(min > left)
			min = left;

		void *dst = (char*)&pb->hdr.ip + lib.instance_info.shm_params.packet_buffer_mtu - pb->bytes_left;

		memcpy(dst, src + len - left, min);

		left -= min;
		pb->bytes_left -= min;

		//sooo ugly
		result += min;

		if(!pb->bytes_left || !(flags & NDP_ONLY_SEND_FULL))
		{
			pb->hdr.ip.tot_len = htons(lib.instance_info.shm_params.packet_buffer_mtu - pb->bytes_left);
			pb->timeout_ref = 0; //special meaning for timeout thread; kinda clunky though
								 //why was it necessary?

			/*would this still be necessary if the assignment is moved after the following "while" ?
			... I would guess not, because the condition involves a function call, but I'm not sure
			... then again, what if I use a MACRO or inline function instead of a regular function ?
			.... programming is hard :( LATER ADDITION: it's probably necessary, don't have enough
			time to think now */

			RELEASE_FENCE();
			pb->flags.lib_ready_to_send = 1;

			//TODO another easy way out via busy waiting
			//while(!produce_at_most(d->socket_info.tx_ring, &idx, sizeof(idx), 1))
			while(!PRODUCER_AVAILABLE_ACQUIRE(d->socket_info.tx_ring))
				SPIN_BODY();

			PRODUCE_RELEASE(d->socket_info.tx_ring, d->tx_pkb_in_use);

			d->tx_pkb_in_use = -1;


			#if LOG201_REC
				lib_record_event(LOG_RECORD_LIB_DATA_SENT, lib.instance_info.id, d->socket_info.index,
						ndp_seq_ntoh(pb->hdr.ndp.sequence_number), lib_read_tsc(), 1);
			#endif

			#if LOG201_MSG
				log_msg("%s -> added DATA seq %lu to tx ring sidx %ld", __func__,
						ndp_seq_ntoh(pb->hdr.ndp.sequence_number), d->socket_info.index);
			#endif

			//if(ndp_seq_ntoh(pb->hdr.ndp.sequence_number) - d->send_isn >= 4100)
				//log_msg("zzzz added DATA seq %lu to tx ring sidx %ld",
					//					ndp_seq_ntoh(pb->hdr.ndp.sequence_number)- d->send_isn, d->socket_info.index);
		}
	}

	//should always be 0 for now; not really since DONT_BLOCK was added
	//result = len - left;

unlock_and_return:
	MY_LOCK_RELEASE(d->send_lock);
	return result;
}
