#include "logger.h"
#include <stdarg.h>
#include <stdio.h>


static void log_msg_aux(int is_partial, const char *format, va_list args)
{
	vprintf(format, args);

	if(!is_partial)
		printf("\n");
}


void log_msg_partial(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	log_msg_aux(1, format, args);
	va_end(args);
}


void log_msg(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	log_msg_aux(0, format, args);
	va_end(args);
}


int two_headed_buffer_init(struct two_headed_buffer *b, uint64_t size)
{
	if(!size)
		return -2;

	memset(b, 0, sizeof(struct two_headed_buffer));
	b->back_index = b->size = size;
	b->buf = malloc(size);

	if(b->buf)
		return 0;

	return -1;
}


void write_two_headed_buffer_to_file(const struct two_headed_buffer *b, FILE *f)
{
	if(!b->num_front_entries && b->back_index == b->size)
		return;

	/*
	FILE *f;

	if(append)
		f = fopen(file_name, "ab");
	else
		f = fopen(file_name, "wb");

	if(!f)
		printf("%s -> %s fopen error\n", __func__, file_name);
	*/

	size_t sz = fwrite(b->buf, 1, b->front_index, f);
	sz += fwrite(b->buf + b->back_index, 1, b->size - b->back_index, f);

	if(sz != (b->front_index + b->size - b->back_index))
		printf("%s -> there was probably an error writing to file\n", __func__);

	fclose(f);
}
