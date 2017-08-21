/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2016 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */
#ifndef PAPI_UTIL_H_
#define PAPI_UTIL_H_

#ifdef USE_PAPI
#include "papi.h"

#define PAPI_EVENT_MAX 11
#define PAPI_GLOBAL_DECL    \
int EventSet = PAPI_NULL;   \
int event_codes[PAPI_EVENT_MAX] = { \
    [0] = PAPI_L1_TCM,              \
    [1] = PAPI_L2_TCM,              \
    [2] = PAPI_L3_TCM,              \
    [4] = PAPI_L2_DCM,              \
    [5] = PAPI_TLB_DM,              \
    [6] = PAPI_L1_STM,              \
    [7] = PAPI_L2_DCA,              \
    [8] = PAPI_L3_DCA,              \
    [9] = PAPI_L2_TCA,              \
    [10] = PAPI_L3_TCA,             \
};                                  \
int event_code = -1;                \
char event_name[PAPI_MAX_STR_LEN];  \
long long papi_val = -1;

extern int EventSet;
extern int event_codes[PAPI_EVENT_MAX];
extern int event_code;
extern char event_name[PAPI_MAX_STR_LEN];
extern long long papi_val;

static inline int papi_exit(void)
{
    int retval;

    if(event_code == -1)
        return 0;

    retval = PAPI_cleanup_eventset(EventSet);
    if (retval != PAPI_OK) {
        PAPI_perror("PAPI_cleanup_eventset");
        return -1;
    }

    retval = PAPI_destroy_eventset(&EventSet);
    if (retval != PAPI_OK) {
        PAPI_perror("PAPI_destroy_eventset");
        return -1;
    }

    PAPI_shutdown();

    return 0;
}

static inline void papi_readenv(void)
{
    int val = -1;
    char *str;
    str = getenv("PAPI_EVENT_ID");
    if(strlen(str) > 0) {
        val = atoi(str);
    }

    if(val >= 0 || val < PAPI_EVENT_MAX) {
        event_code = event_codes[val];
    }
}
static inline int papi_init(int size)
{
    int retval;

    papi_readenv();

    if(event_code == -1)
        return 0;

    if ((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
        PAPI_perror("PAPI_library_init");
        return -1;
    }

    if ((retval = PAPI_create_eventset(&EventSet)) != PAPI_OK) {
        PAPI_perror("PAPI_create_eventset");
        return -1;
    }

    if ((retval = PAPI_add_event(EventSet, event_code)) != PAPI_OK) {
        PAPI_perror("PAPI_add_env_event");
        return -1;
    }

    if ((retval = PAPI_event_code_to_name(event_code, event_name)) != PAPI_OK) {
        PAPI_perror("PAPI_event_code_to_name");
        return -1;
    }

    fprintf(stdout, "start PAPI_EVENT_ID=%d, %s\n", event_code, event_name);
    fflush(stdout);

    return 0;
}

static inline void papi_coll_report(MPI_Comm comm, size_t msg_len)
{
    long long * papi_vals = NULL;
    int rank , size, i;

    if(event_code == -1)
        return;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    papi_vals = malloc(size * sizeof(long long));
    if (papi_vals == NULL)
        MPI_Abort(MPI_COMM_WORLD, -1);

    MPI_Gather(&papi_val, 1, MPI_LONG_LONG, papi_vals, 1, MPI_LONG_LONG, 0, comm);

    if (rank == 0) {
        fprintf(stdout, " %ld     %s ", msg_len, event_name);
        for(i = 0; i< size; i++)
            fprintf(stdout, "%lld ", papi_vals[i]);
        fprintf(stdout, "\n");
        fflush(stdout);
    }

    free(papi_vals);
}

#define PAPI_STOP()  do {                                               \
        int retval;                                                     \
        if ((retval = PAPI_stop(EventSet, &papi_val)) != PAPI_OK) {   \
            PAPI_perror("PAPI_stop");                                   \
        }                                                               \
} while (0)

#define PAPI_START()  do {                                              \
        int retval;                                                     \
        if ((retval = PAPI_start(EventSet)) != PAPI_OK) {               \
            PAPI_perror("PAPI_start");                                  \
        }                                                               \
} while (0)
#else

#define PAPI_GLOBAL_DECL
#define PAPI_STOP() do {} while (0)
#define PAPI_START() do {} while (0)

static inline int papi_init(int size) { return 0; }
static inline int papi_exit(void) { return 0; }
static inline void papi_coll_report(MPI_Comm comm, size_t msg_len) {}
#endif


#endif /* PAPI_UTIL_H_ */
