#ifndef _UTILS_THREAD_H
#define _UTILS_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

struct thread {
	HANDLE id;
	void *(*fn)(void *);
	void *arg;
	void *ret;
	char err[64];
};

#else

#include <pthread.h>

struct thread {
	pthread_t id;
	char err[128];
};

#endif

/**
 * @param t thread
 */
void thread_init(struct thread *t);

/**
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error, call 'thread_err()' for error string.
 */
int thread_term(struct thread *t);

/**
 * @param t thread
 * @return  last error message
 */
const char *thread_err(struct thread *t);

/**
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error, call 'thread_err()' for error string.
 */
int thread_start(struct thread *t, void *(*fn)(void *), void *arg);

/**
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error, call 'thread_err()' for error string.
 */
int thread_join(struct thread *t, void **ret);

#ifdef __cplusplus
}
#endif

#if defined(_UTILS_IMPL) || defined(_UTILS_THREAD_IMPL)

#include <string.h>

void thread_init(struct thread *t)
{
	t->id = 0;
}

#if defined(_WIN32) || defined(_WIN64)
#pragma warning(disable : 4996)

#include <process.h>

static void thread_errstr(struct thread *t)
{
	int rc;
	DWORD err = GetLastError();
	LPSTR errstr = 0;

	rc = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
				    FORMAT_MESSAGE_FROM_SYSTEM,
			    NULL, err, 0, (LPSTR) &errstr, 0, NULL);
	if (rc != 0) {
		strncpy(t->err, errstr, sizeof(t->err) - 1);
		LocalFree(errstr);
	}
}

unsigned int __stdcall thread_fn(void *arg)
{
	struct thread *t = arg;

	t->ret = t->fn(t->arg);
	return 0;
}

int thread_start(struct thread *t, void *(*fn)(void *), void *arg)
{
	int rc = 0;

	t->fn = fn;
	t->arg = arg;

	t->id = (HANDLE) _beginthreadex(NULL, 0, thread_fn, t, 0, NULL);
	if (t->id == 0) {
		thread_errstr(t);
		rc = -1;
	}

	return rc;
}

int thread_join(struct thread *t, void **ret)
{
	int rc = 0;
	void *val = NULL;
	DWORD rv;
	BOOL brc;

	if (t->id == 0) {
		goto out;
	}

	rv = WaitForSingleObject(t->id, INFINITE);
	if (rv == WAIT_FAILED) {
		thread_errstr(t);
		rc = -1;
	}

	brc = CloseHandle(t->id);
	if (!brc) {
		thread_errstr(t);
		rc = -1;
	}

	val = t->ret;
	t->id = 0;

out:
	if (ret != NULL) {
		*ret = t->ret;
	}

	return rc;
}
#else

int thread_start(struct thread *t, void *(*fn)(void *), void *arg)
{
	int rc;
	pthread_attr_t attr;

	rc = pthread_attr_init(&attr);
	if (rc != 0) {
		strncpy(t->err, strerror(rc), sizeof(t->err) - 1);
		return -1;
	}

	// This may only fail with EINVAL.
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	rc = pthread_create(&t->id, &attr, fn, arg);
	if (rc != 0) {
		strncpy(t->err, strerror(rc), sizeof(t->err) - 1);
	}

	// This may only fail with EINVAL.
	pthread_attr_destroy(&attr);

	return rc;
}

int thread_join(struct thread *t, void **ret)
{
	int rc = 0;
	void *val = NULL;

	if (t->id == 0) {
		goto out;
	}

	rc = pthread_join(t->id, &val);
	if (rc != 0) {
		strncpy(t->err, strerror(rc), sizeof(t->err) - 1);
	}

	t->id = 0;

out:
	if (ret != NULL) {
		*ret = val;
	}

	return rc;
}

#endif

int thread_term(struct thread *t)
{
	return thread_join(t, NULL);
}

const char *thread_err(struct thread *t)
{
	return t->err;
}

#endif
#endif