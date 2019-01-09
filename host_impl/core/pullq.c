#include "../common/logger.h"
#include <arpa/inet.h>
#include "helpers.h"


static inline void send_pull_packet(struct core_socket_descriptor *d, uint16_t queue_id)
{
	struct ndp_socket *s = d->socket_info.socket;

	//s->control_packet_template.ndp.flags = NDP_HEADER_FLAG_PULL;
	s->pull_template.ndp.pull_number = ndp_pull_hton(++d->highest_pull_sent);

	send_one_packet(&s->pull_template, sizeof(struct ndp_packet_headers), queue_id);

	#if LOG200_REC

		//if(d->socket_info.index != 2 && CONSUMER_AVAILABLE_ACQUIRE(core.pullq2))
			//LOG_MSG(1, "mda");

		core_record_event(LOG_RECORD_CORE_PULL_SENT, d->instance_info->id, d->socket_info.index,
				d->highest_pull_sent, core_read_tsc());
	#endif

	#if LOG200_MSG
	  log_msg("%s -> sent PULL "NDP_PACKET_HDRS_FORMAT" sidx %ld", __func__,
			NDP_PACKET_HDRS_VALUES(&s->control_packet_template), d->socket_info.index);
	#endif
}



#if ! NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
void check_pullq_in_core_loop(void)
{
	ndp_cycle_count_t tsc = core_read_tsc();

	while(1)
	{
		ndp_cycle_count_t diff =  core.next_pull_cycles - tsc;

		if(diff > core.cycles_per_pull)
		{
			struct core_socket_descriptor *d;

			if(LIKELY (CONSUMER_AVAILABLE(core.pullq)))
			{
				d = CONSUME(core.pullq);
				send_pull_packet(d, core.dpdk_default_queue_id);

				if(core.pullq_was_empty)
					core.next_pull_cycles = tsc + core.cycles_per_pull;
				else
					core.next_pull_cycles += core.cycles_per_pull;

				core.pullq_was_empty = 0;
			}
			else
			{
				core.next_pull_cycles = tsc;
				core.pullq_was_empty = 1;
			}
		}
		else
			break;
	}
}
#endif



#if NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
int pullq_thread_func(void *param __attribute__((unused)))
{
	core.next_pull_cycles = core_read_tsc() + core.cycles_per_pull;

	while(LIKELY(core.keep_running_pullq_core))
	{
		ndp_cycle_count_t tsc = core_read_tsc();
		ndp_cycle_count_t diff =  core.next_pull_cycles - tsc;

		if(diff > core.cycles_per_pull)
		{
			struct core_socket_descriptor *d = NULL;

			#if NDP_CORE_PRIOPULLQ_4COSTIN

				if(CONSUMER_AVAILABLE_ACQUIRE(core.pullq2))
				{
					d = CONSUME_RELEASE(core.pullq2);

					//LOG_FMSG(1, "asdf %u", d->socket_info.index);
				}
				else if(LIKELY (CONSUMER_AVAILABLE_ACQUIRE(core.pullq)))
				{
					//if(CONSUMER_AVAILABLE_ACQUIRE(core.pullq2))
						//LOG_MSG(1, "mda3");

					d = CONSUME_RELEASE(core.pullq);

					//if(CONSUMER_AVAILABLE_ACQUIRE(core.pullq2))
						//LOG_MSG(1, "mda3");

					if(UNLIKELY (d->socket_info.index == 0))
						LOG_MSG(1, "baaaaaa baaaa");
				}

			#else
				if(LIKELY (CONSUMER_AVAILABLE_ACQUIRE(core.pullq)))
					d = CONSUME_RELEASE(core.pullq);
			#endif

			if(LIKELY (d))
			{
				//d = CONSUME_RELEASE(core.pullq);
				send_pull_packet(d, core.dpdk_pull_tx_queue_id);

				if(core.pullq_was_empty)
					core.next_pull_cycles = tsc + core.cycles_per_pull;
				else
					core.next_pull_cycles += core.cycles_per_pull;

				core.pullq_was_empty = 0;
			}
			else
			{
				core.next_pull_cycles = tsc;
				core.pullq_was_empty = 1;
			}
		}
	}

	RELEASE_FENCE();

	core.pullq_core_finished = 1;

	return 0;
}
#endif
