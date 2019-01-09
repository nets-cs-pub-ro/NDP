#include "../common/util.h"
#include "f_helpers.h"
#include "hoover_ndp.h"

//#include <stdio.h>

ssize_t ndp_send_all(int sock, const char *buf, size_t len)
{
	ssize_t sent = 0;

	while(len)
	//do
	{
		ssize_t x = ndp_send(sock, buf + sent, len, 0);

		if(UNLIKELY (x <= 0))
			return x;

		//printf("sent %ld\n", x);

		len -= x;
		sent += x;

	} //while(len);

	return sent;
}


ssize_t ndp_recv_all(int sock, char *buf, size_t len)
{
	ssize_t rcvd = 0;

	while(len)
	//do
	{
		ssize_t x = ndp_recv(sock, buf + rcvd, len, 0);

		//printf("aa %lu\n", x);

		if(UNLIKELY (x <= 0))
			return x;

		len -= x;
		rcvd += x;

	} //while(len);

	return rcvd;
}



ssize_t ndp_send_bogus(int sock, size_t len, const void *buf, size_t buf_size)
{
	const char *b = buf;

	ssize_t result = len;
	size_t buf_idx = 0;


	while(len)
	{
		size_t x = buf_size - buf_idx;

		if(len < x)
			x = len;

		ssize_t y = ndp_send(sock, b + buf_idx, x, 0);

		if(UNLIKELY (y <= 0))
			return y;

		buf_idx = (buf_idx + y) % buf_size;

		len -= y;
	}

	return result;
}
ssize_t ndp_recv_bogus(int sock, size_t len, void *buf, size_t buf_size)
{
	char *b = buf;

	ssize_t result = len;
	size_t buf_idx = 0;


	while(len)
	{
		size_t x = buf_size - buf_idx;

		if(len < x)
			x = len;

		ssize_t y = ndp_recv(sock, b + buf_idx, x, 0);

		if(UNLIKELY (y <= 0))
			return y;

		buf_idx = (buf_idx + y) % buf_size;

		len -= y;
	}

	return result;
}

