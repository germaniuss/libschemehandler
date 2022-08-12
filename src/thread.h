/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THREAD_H
#define THREAD_H

#define THREAD_VERSION "2.0.0"

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

#endif
