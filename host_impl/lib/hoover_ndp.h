#ifndef NDP_LIB_HOOVER_NDP_H_
#define NDP_LIB_HOOVER_NDP_H_

#include <stdint.h>
#include <sys/types.h>

typedef uint32_t ndp_port_number_t;

int ndp_init(void);

int ndp_connect(const char* remote_addr, unsigned int remote_port);

int ndp_listen(ndp_port_number_t port, int backlog);

//ndp_accept flags
#define NDP_ACCEPT_DONT_BLOCK					(1 << 0)

//ndp_accept return codes
#define NDP_ACCEPT_ERR_NONE_AVAILABLE			-100

int ndp_accept(int sock, int flags);

//ndp_send flags
#define NDP_ONLY_SEND_FULL						(1 << 0)
#define NDP_SEND_DONT_BLOCK						(1 << 1)

ssize_t ndp_send(int sock, const void *src, size_t len, int flags);

//ndp_recv flags
#define NDP_RECV_DONT_BLOCK						(1 << 0)

ssize_t ndp_recv(int sock, void *dst, size_t len, int flags);

int ndp_close(int sock);


int ndp_terminate(void);


#endif /* NDP_LIB_HOOVER_NDP_H_ */
