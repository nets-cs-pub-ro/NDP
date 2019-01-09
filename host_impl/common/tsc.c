#include "logger.h"
#include "tsc.h"


double ndp_estimate_tsc_hz(uint64_t sleep_time_ns)
{
	struct timespec start, end;
	struct timespec sleep_time;
	uint64_t tsc_start, tsc_end, delta_ns;
	int a, b;

	sleep_time.tv_nsec = sleep_time_ns % NS_PER_SECOND;
	sleep_time.tv_sec = sleep_time_ns / NS_PER_SECOND;

	if(clock_gettime(CLOCK_MONOTONIC_RAW, &start) == 0)
	{
		tsc_start = ndp_rdtsc();
		a = nanosleep(&sleep_time, NULL);
		b = clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		tsc_end = ndp_rdtsc();

		if(a)
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> nanosleep error", __func__);

		if(b)
			LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> second clock_gettime error", __func__);

		delta_ns = end.tv_sec * NS_PER_SECOND + end.tv_nsec;
		delta_ns -= start.tv_sec * NS_PER_SECOND + start.tv_nsec;

		double result = ((double)(tsc_end - tsc_start) / delta_ns) * NS_PER_SECOND;

		return result;
	}
	else
		LOG_FMSG_DO(LOGLVL1, exit(1), "%s -> first clock_gettime error", __func__);
}
