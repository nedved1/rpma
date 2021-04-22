/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021, Intel Corporation */

/*
 * mtt.h -- a multithreaded tests' API
 *
 * For an example of how to use this API please see already existing
 * multithreaded tests especially the example test.
 */

#ifndef MTT
#define MTT

#include <stddef.h>

/* arguments coming from the command line */
struct mtt_args {
	unsigned threads_num;
	char *addr;
};

int mtt_parse_args(int argc, char *argv[], struct mtt_args *args);

#define MTT_ERRMSG_MAX 512

/* a store for any thread error message and the return value */
struct mtt_result {
	int ret;
	char errmsg[MTT_ERRMSG_MAX];
};

/*
 * mtt_base_file_name -- extract the exact file name from a file name with path
 */
static const char *
mtt_base_file_name(const char *file_name)
{
	const char *base_file_name = strrchr(file_name, '/');
	if (!base_file_name)
		base_file_name = file_name;
	else
		/* skip '/' */
		base_file_name++;

	return base_file_name;
}

/* on error populate the result and the error string */
#define MTT_ERR(result, func, err) \
	do { \
		(result)->ret = err; \
		snprintf((result)->errmsg, MTT_ERRMSG_MAX - 1, \
			"%s:%d %s() -> %s() failed: %s", \
			mtt_base_file_name(__FILE__), __LINE__, __func__, \
			func, strerror(err)); \
	} while (0)

/* on librpma error populate the result and the error string */
#define MTT_RPMA_ERR(result, func, err) \
	do { \
		(result)->ret = err; \
		snprintf((result)->errmsg, MTT_ERRMSG_MAX - 1, \
			"%s:%d %s() -> %s() failed: %s", \
			mtt_base_file_name(__FILE__), __LINE__, __func__, \
			func, rpma_err_2str(err)); \
	} while (0)

/*
 * mtt_thread_init_fini_func -- a function type used for all initialization and
 * cleanup steps
 *
 * Arguments:
 * - id        - a thread identifier. It is constant for the whole life of
 *               the thread including sequential initialization and sequential
 *               cleanup.
 * - prestate  - a pointer to test-provided data passed to all threads in all
 *               steps. It is shared in a non-thread-safe way.
 * - state_ptr - a pointer to thread-related data. The test can allocate and
 *               store their specific data here at any point. Accessing it is
 *               always thread-safe. Once the data is stored the test is also
 *               responsible for freeing it.
 * - result    - the result. On error the test is responsible for providing
 *               the error details (using e.g. MTT_ERR or MTT_RPMA_ERR macros),
 *               the test should not print anything to stdout nor stderr during
 *               parallel steps of the test (thread_init_func, thread_func,
 *               and thread_fini_func).
 */
typedef void (*mtt_thread_init_fini_func)(unsigned id, void *prestate,
		void **state_ptr, struct mtt_result *result);

/*
 * mtt_thread_func -- a function type used for the main execution step
 *
 * Arguments:
 * - id       - a thread identifier. It is constant for the whole life of
 *              the thread including sequential initialization and sequential
 *              cleanup.
 * - prestate - a pointer to test-provided data passed to all threads in all
 *              steps. It is shared in a non-thread-safe way.
 * - state    - a pointer to thread-related data. At this point, it is available
 *              as long as it was prepared during one of the initialization
 *              steps. Note it should not be freed during this step. For tips
 *              on how to allocate/free the thread-related data please see
 *              mtt_thread_init_fini_func.
 * - result   - the result. On error the test is responsible for providing
 *              the error details (using e.g. MTT_ERR or MTT_RPMA_ERR macros),
 *              the test should not print anything to stdout nor stderr during
 *              parallel steps of the test (thread_init_func, thread_func,
 *              and thread_fini_func).
 */
typedef void (*mtt_thread_func)(unsigned id, void *prestate, void *state,
		struct mtt_result *result);

struct mtt_test {
	/*
	 * a pointer to test-provided data passed on all initialization steps
	 * (both sequential and parallel) and also on thread_func
	 */
	void *prestate;

	/*
	 * a function called for each of threads before spawning it (sequential)
	 */
	mtt_thread_init_fini_func thread_seq_init_func;

	/*
	 * a function called at the beginning of each thread
	 * (parallel but before synchronizing all threads)
	 */
	mtt_thread_init_fini_func thread_init_func;

	/*
	 * a thread main function (parallel and after synchronizing all threads)
	 */
	mtt_thread_func thread_func;

	/* a function called at the end of each thread (parallel) */
	mtt_thread_init_fini_func thread_fini_func;

	/*
	 * a function called for each of threads after its termination
	 * (sequential)
	 */
	mtt_thread_init_fini_func thread_seq_fini_func;
};

int mtt_run(struct mtt_test *test, unsigned threads_num);

#endif /* MTT */
