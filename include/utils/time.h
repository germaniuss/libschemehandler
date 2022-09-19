#ifndef _UTILS_TIME_H
#define _UTILS_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Wall clock time. Gets CLOCK_REALTIME on Posix.
 * @return current timestamp in milliseconds.
 */
uint64_t time_ms();

/**
 * Wall clock time. Gets CLOCK_REALTIME on Posix.
 * @return current timestamp in nanoseconds.
 */
uint64_t time_ns();

/**
 * Monotonic timer. Gets CLOCK_MONOTONIC on Posix
 * @return current timestamp in milliseconds.
 */
uint64_t time_mono_ms();

/**
 * Monotonic timer. Gets CLOCK_MONOTONIC on Posix
 * @return Current timestamp in nanoseconds.
 */
uint64_t time_mono_ns();

/**
 * @param millis milliseconds to sleep.
 * @return '0' on success, negative on failure.
 */
int time_sleep(uint64_t millis);

#ifdef __cplusplus
}
#endif

#if	defined(_UTILS_IMPL) || defined(_UTILS_TIME_IMPL)

#if defined(_WIN32) || defined(_WIN64)
#include <assert.h>
#include <windows.h>
#else
#include <assert.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#endif

uint64_t time_ms()
{
#if defined(_WIN32) || defined(_WIN64)
	FILETIME ft;
	ULARGE_INTEGER dateTime;

	GetSystemTimeAsFileTime(&ft);
	dateTime.LowPart = ft.dwLowDateTime;
	dateTime.HighPart = ft.dwHighDateTime;

	return (dateTime.QuadPart / 10000);
#else
	int rc;
	struct timespec ts;

	rc = clock_gettime(CLOCK_REALTIME, &ts);
	assert(rc == 0);
	(void) rc;

	return ts.tv_sec * 1000 + (uint64_t) (ts.tv_nsec / 10e6);
#endif
}

uint64_t time_ns()
{
#if defined(_WIN32) || defined(_WIN64)
	FILETIME ft;
	ULARGE_INTEGER dateTime;

	GetSystemTimeAsFileTime(&ft);
	dateTime.LowPart = ft.dwLowDateTime;
	dateTime.HighPart = ft.dwHighDateTime;

	return (dateTime.QuadPart * 100);
#else
	int rc;
	struct timespec ts;

	rc = clock_gettime(CLOCK_REALTIME, &ts);
	assert(rc == 0);
	(void) rc;

	return ts.tv_sec * (uint64_t) 1000000000 + ts.tv_nsec;
#endif
}

uint64_t time_mono_ms()
{
#if defined(_WIN32) || defined(_WIN64)
	//  System frequency does not change at run-time, cache it
	static int64_t frequency = 0;
	if (frequency == 0) {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		assert(freq.QuadPart != 0);
		frequency = freq.QuadPart;
	}
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	return (int64_t) (count.QuadPart * 1000) / frequency;
#else
	int rc;
	struct timespec ts;

	rc = clock_gettime(CLOCK_MONOTONIC, &ts);
	assert(rc == 0);
	(void) rc;

	return (uint64_t) ((uint64_t) ts.tv_sec * 1000 +
			   (uint64_t) ts.tv_nsec / 1000000);
#endif
}

uint64_t time_mono_ns()
{
#if defined(_WIN32) || defined(_WIN64)
	static int64_t frequency = 0;
	if (frequency == 0) {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		assert(freq.QuadPart != 0);
		frequency = freq.QuadPart;
	}
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	return (uint64_t) (count.QuadPart * 1000000000) / frequency;
#else
	int rc;
	struct timespec ts;

	rc = clock_gettime(CLOCK_MONOTONIC, &ts);
	assert(rc == 0);
	(void) rc;

	return ((uint64_t) ts.tv_sec * 1000000000 + (uint64_t) ts.tv_nsec);
#endif
}

int time_sleep(uint64_t millis)
{
#if defined(_WIN32) || defined(_WIN64)
	Sleep((DWORD) millis);
	return 0;
#else
	int rc;
	struct timespec t, rem;

	rem.tv_sec = millis / 1000;
	rem.tv_nsec = (millis % 1000) * 1000000;

	do {
		t = rem;
		rc = nanosleep(&t, &rem);
	} while (rc != 0 && errno == EINTR);

	return rc;
#endif
}

#endif
#endif