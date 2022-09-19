#ifndef _UTILS_MUTEX_H
#define _UTILS_MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <pthread.h>
#endif

struct mutex {
#if defined(_WIN32) || defined(_WIN64)
	CRITICAL_SECTION mtx;
#else
	pthread_mutex_t mtx;
#endif
};

/**
 * Create mutex.
 *
 * Be warned on Windows, mutexes are recursive, on Posix default
 * mutex type is not recursive. Edit code if that bothers you. Pass
 * PTHREAD_MUTEX_RECURSIVE instead of PTHREAD_MUTEX_NORMAL.
 *
 * @param mtx mtx
 * @return    '0' on success, '-1' on error.
 */
int mutex_init(struct mutex *mtx);

/**
 * Destroy mutex
 *
 * @param mtx mtx
 * @return    '0' on success, '-1' on error.
 */
int mutex_term(struct mutex *mtx);

/**
 * @param mtx mtx
 */
void mutex_lock(struct mutex *mtx);

/**
 * @param mtx mtx
 */
void mutex_unlock(struct mutex *mtx);

#ifdef __cplusplus
}
#endif

#if defined(_UTILS_IMPL) || defined(_UTILS_MUTEX_IMPL)

#include <assert.h>

#if defined(_WIN32) || defined(_WIN64)

int mutex_init(struct mutex *mtx)
{
	InitializeCriticalSection(&mtx->mtx);
	return 0;
}

int mutex_term(struct mutex *mtx)
{
	DeleteCriticalSection(&mtx->mtx);
	return 0;
}

void mutex_lock(struct mutex *mtx)
{
	EnterCriticalSection(&mtx->mtx);
}

void mutex_unlock(struct mutex *mtx)
{
	LeaveCriticalSection(&mtx->mtx);
}

#else

int mutex_init(struct mutex *mtx)
{
	int rc, rv;
	pthread_mutexattr_t attr;
	pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

	mtx->mtx = mut;

	// May fail on OOM
	rc = pthread_mutexattr_init(&attr);
	if (rc != 0) {
		return -1;
	}

	// This won't fail as long as we pass correct params.
	rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
	assert(rc == 0);

	// May fail on OOM
	rc = pthread_mutex_init(&mtx->mtx, &attr);

	// This won't fail as long as we pass correct param.
	rv = pthread_mutexattr_destroy(&attr);
	assert(rv == 0);
	(void) rv;

	return rc != 0 ? -1 : 0;
}

int mutex_term(struct mutex *mtx)
{
	int rc;

	rc = pthread_mutex_destroy(&mtx->mtx);
	return rc != 0 ? -1 : 0;
}

void mutex_lock(struct mutex *mtx)
{
	int rc;

	// This won't fail as long as we pass correct param.
	rc = pthread_mutex_lock(&mtx->mtx);
	assert(rc == 0);
	(void) rc;
}

void mutex_unlock(struct mutex *mtx)
{
	int rc;

	// This won't fail as long as we pass correct param.
	rc = pthread_mutex_unlock(&mtx->mtx);
	assert(rc == 0);
	(void) rc;
}

#endif
#endif
#endif