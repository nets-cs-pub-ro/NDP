#ifndef NDP_LIB_TIMEOUT_H_
#define NDP_LIB_TIMEOUT_H_


//TODO maybe move these defs to the config header?

//#define NDP_TIMEOUT_INTERVAL_NS				(6 * 100 * 1000)  	//600us
#define NDP_TIMEOUT_INTERVAL_NS				(1000 * 1000 * 1000)  		//1s was 2ms

//how much time to wait between checks
#define NDP_TIMEOUT_THREAD_SLEEP_NS			(100 * 1000)	    //100us


void* timeout_thread_function(void *arg);


//TODO we assume some kind of lock is held while calling this
static inline void check_for_timeouts(struct lib_socket_descriptor *d, int from_main_thread)
{
	//TODO should this be placed inside the loop body?
	ndp_cycle_count_t tsc = lib_read_tsc();

	for(ndp_socket_index_t j = 0; j < lib.instance_info.shm_params.num_tx_packet_buffers; j++)
	{
		struct ndp_packet_buffer *pb = get_tx_packet_buffer(&d->socket_info, j);

		if(!pb->flags.lib_ready_to_send ||	!pb->flags.core_sent_to_nic)
			continue;


		if(pb->timeout_ref == 0)
		{
			pb->timeout_ref = tsc;
			continue;
		}

		if(tsc - pb->timeout_ref >= lib.cycles_per_timeout)
		{
			pb->flags.core_sent_to_nic = 0;

			pb->timeout_ref = tsc;

			//if(pb->flags.in_to_rtx_ring)
				//continue;

			if(!pb->flags.lib_was_retransmitted)
			{
				pb->flags.lib_was_retransmitted = 1;
				lctrs.to_pkbs++;
			}

			#if LOG204_REC
				lib_record_event(LOG_RECORD_LIB_TIMEOUT, lib.instance_info.id, d->socket_info.index,
						ndp_seq_ntoh(pb->hdr.ndp.sequence_number), tsc, from_main_thread);
			#endif

			#if LOG204_MSG
				log_msg("%s -> sidx %d timeout for seq %lu (isn %lu)", __func__, d->socket_info.index, ndp_seq_ntoh(pb->hdr.ndp.sequence_number), d->send_isn);
			#endif

			lctrs.to_rtx++;

			//pb->flags.in_to_rtx_ring = 1;

			//if(UNLIKELY(!produce_at_most(d->socket_info.to_rtx_ring, &j, sizeof(j), 1)))
			if(LIKELY (PRODUCER_AVAILABLE_ACQUIRE(d->socket_info.to_rtx_ring)))
				PRODUCE_RELEASE(d->socket_info.to_rtx_ring, j)
			else
			{
				LOG_FMSG(LOGLVL3, "%s -> no room in to_rtx_ring", __func__);
				exit(1);
			}

		}
	}
}


#endif /* NDP_LIB_TIMEOUT_H_ */
