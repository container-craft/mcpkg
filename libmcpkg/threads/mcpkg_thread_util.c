#include "mcpkg_thread_util.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

void mcpkg_thread_sleep_ms(unsigned long ms)
{
#if defined(_WIN32)
	Sleep((DWORD)ms);
#else
	struct timespec ts;
	ts.tv_sec  = (time_t)(ms / 1000UL);
	ts.tv_nsec = (long)((ms % 1000UL) * 1000000UL);
	(void)nanosleep(&ts, NULL);
#endif
}

uint64_t mcpkg_thread_time_ms(void)
{
#if defined(_WIN32)
	LARGE_INTEGER freq, now;
	if (QueryPerformanceFrequency(&freq) && QueryPerformanceCounter(&now)) {
		return (uint64_t)((now.QuadPart * 1000ULL) / (uint64_t)freq.QuadPart);
	}
	return (uint64_t)GetTickCount64();
#else
	struct timespec ts;
#if defined(CLOCK_MONOTONIC)
	clock_gettime(CLOCK_MONOTONIC, &ts);
#else
	clock_gettime(CLOCK_REALTIME, &ts);
#endif
	return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
#endif
}
