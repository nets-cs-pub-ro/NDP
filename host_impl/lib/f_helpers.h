#ifndef NDP_LIB_F_HELPERS_H_
#define NDP_LIB_F_HELPERS_H_


#include "defs.h"

#include <stdio.h>

#include <sys/types.h>


ssize_t ndp_send_all(int sock, const char *buf, size_t len);
ssize_t ndp_recv_all(int sock, char *buf, size_t len);

ssize_t ndp_send_bogus(int sock, size_t len, const void *buf, size_t buf_size);
ssize_t ndp_recv_bogus(int sock, size_t len, void *buf, size_t buf_size);


static inline void print_stupid_counters()
{
	printf("in_w_already %lu ack_old_data %lu ack_no_tx_pkb %lu NACK_no_tx_pkb %lu to_rtx %lu "
			"ack_holes %lu to_pkbs %lu old_acks %lu old_nacks %lu\n"
			"data_totrx %lu acks_totrx %lu rwnd_acks %lu\n",
			lctrs.in_w_already, lctrs.ack_old_data, lctrs.ack_no_tx_pkb, lctrs.nack_no_tx_pkb, lctrs.to_rtx,
			lctrs.ack_holes, lctrs.to_pkbs, lctrs.old_acks, lctrs.old_nacks,
			lctrs.data_totrx, lctrs.acks_totrx, lctrs.rwnd_acks);
}



#endif /* NDP_LIB_F_HELPERS_H_ */
