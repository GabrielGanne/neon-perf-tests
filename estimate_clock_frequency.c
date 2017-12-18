#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/syscall.h>

static inline uint64_t
flt_round_nearest(double x)
{
    return (uint64_t) (x + .5);
}

/* Use 64bit floating point to represent time offset from epoch. */
static inline double
unix_time_now(void)
{
    /* clock_gettime without indirect syscall uses GLIBC wrappers which
       we don't want.  Just the bare metal, please. */
    struct timespec ts;
    syscall(SYS_clock_gettime, CLOCK_REALTIME, &ts);
    return ts.tv_sec + 1e-9 * ts.tv_nsec;
}

#if defined(__x86_64__)
static inline uint64_t
clib_cpu_time_now(void)
{
    uint32_t a, d;
    asm volatile ("rdtsc" : "=a" (a), "=d" (d));
    return (uint64_t) a + ((uint64_t) d << (uint64_t) 32);
}

#elif defined (__arm__)  /* armv7 */
#if defined(__ARM_ARCH_8A__)
static inline uint64_t
clib_cpu_time_now(void)  /* We may run arm64 in aarch32 mode, to leverage 64bit counter */
{
    uint64_t tsc;
    asm volatile ("mrrc p15, 0, %Q0, %R0, c9" : "=r" (tsc));
    return tsc;
}

#elif defined(__ARM_ARCH_7A__)
static inline uint64_t
clib_cpu_time_now(void)
{
    uint32_t tsc;
    asm volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r" (tsc));
    return (uint64_t) tsc;
}

#endif  /* __arm__ */
#elif __ARM_ARCH == 8
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
static int fddev = -1;

__attribute__((constructor)) static void
init(void)
{
    static struct perf_event_attr attr;
    attr.type = PERF_TYPE_HARDWARE;
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    fddev = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
}

__attribute__((destructor)) static void
fini(void)
{
    close(fddev);
}

static inline uint64_t
clib_cpu_time_now(void)  /* We may run arm64 in aarch32 mode, to leverage 64bit counter */
{
    ssize_t rv;
    uint64_t tsc;

    rv = read(fddev, &tsc, sizeof(tsc));
    if (rv < (ssize_t) sizeof(tsc))
        return 0;
    return tsc;
}

#else  /* __x86_64__ */
#error "clib_cpu_time_now() undefined"
#endif  /* __x86_64__ */


static double
estimate_clock_frequency(double sample_time)
{
    /* Round to nearest 100KHz. */
    const double round_to_units = 100e5;

    double time_now, time_start, time_limit, freq;
    uint64_t ifreq, t[2];

    time_start = time_now = unix_time_now();
    time_limit = time_now + sample_time;
    t[0] = clib_cpu_time_now();
    while (time_now < time_limit)
    {
        time_now = unix_time_now();
    }
    t[1] = clib_cpu_time_now();

    freq = (t[1] - t[0]) / (time_now - time_start);
    ifreq = flt_round_nearest(freq / round_to_units);
    freq = ifreq * round_to_units;

    return freq;
}

int
main()
{
    double freq = estimate_clock_frequency(1e-3);
    printf("Freq: %lf\n", freq);
    return 0;
}