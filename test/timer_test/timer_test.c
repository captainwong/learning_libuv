#include "uv.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#if defined(_MSC_VER) && _MSC_VER < 1600
# include "uv/stdint-msvc2008.h"
#else
# include <stdint.h>
#endif

#include <Windows.h>

#define uv__malloc malloc

/* The number of nanoseconds in one second. */
#define UV__NANOSEC 1000000000

#define MAKE_VALGRIND_HAPPY() 

static CRITICAL_SECTION process_title_lock;

/* Frequency of the high-resolution clock. */
static uint64_t hrtime_frequency_ = 0;


/*
 * Display an error message and abort the event loop.
 */
void uv_fatal_error(const int errorno, const char* syscall) {
    char* buf = NULL;
    const char* errmsg;

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorno,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0, NULL);

    if (buf) {
        errmsg = buf;
    } else {
        errmsg = "Unknown error";
    }

    /* FormatMessage messages include a newline character already, so don't add
     * another. */
    if (syscall) {
        fprintf(stderr, "%s: (%d) %s", syscall, errorno, errmsg);
    } else {
        fprintf(stderr, "(%d) %s", errorno, errmsg);
    }

    if (buf) {
        LocalFree(buf);
    }

    DebugBreak();
    abort();
}

/*
 * One-time initialization code for functionality defined in util.c.
 */
void uv__util_init(void) {
    LARGE_INTEGER perf_frequency;

    /* Initialize process title access mutex. */
    InitializeCriticalSection(&process_title_lock);

    /* Retrieve high-resolution timer frequency
     * and precompute its reciprocal.
     */
    if (QueryPerformanceFrequency(&perf_frequency)) {
        hrtime_frequency_ = perf_frequency.QuadPart;
    } else {
        uv_fatal_error(GetLastError(), "QueryPerformanceFrequency");
    }
}


uint64_t uv_hrtime(void) {
    return uv__hrtime(UV__NANOSEC);
}

uint64_t uv__hrtime(unsigned int scale) {
    LARGE_INTEGER counter;
    double scaled_freq;
    double result;

    assert(hrtime_frequency_ != 0);
    assert(scale != 0);
    if (!QueryPerformanceCounter(&counter)) {
        uv_fatal_error(GetLastError(), "QueryPerformanceCounter");
    }
    assert(counter.QuadPart != 0);

    /* Because we have no guarantee about the order of magnitude of the
     * performance counter interval, integer math could cause this computation
     * to overflow. Therefore we resort to floating point math.
     */
    scaled_freq = (double)hrtime_frequency_ / scale;
    result = (double)counter.QuadPart / scaled_freq;
    return (uint64_t)result;
}




static uv_loop_t default_loop_struct;
static uv_loop_t* default_loop_ptr;


int uv_loop_init(uv_loop_t* loop) {
    //uv__loop_internal_fields_t* lfields;
    struct heap* timer_heap;
    int err;

    ///* Initialize libuv itself first */
    //uv__once_init();

    ///* Create an I/O completion port */
    //loop->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    //if (loop->iocp == NULL)
    //    return uv_translate_sys_error(GetLastError());

    //lfields = (uv__loop_internal_fields_t*)uv__calloc(1, sizeof(*lfields));
    //if (lfields == NULL)
    //    return UV_ENOMEM;
    //loop->internal_fields = lfields;

    //err = uv_mutex_init(&lfields->loop_metrics.lock);
    //if (err)
    //    goto fail_metrics_mutex_init;

    /* To prevent uninitialized memory access, loop->time must be initialized
     * to zero before calling uv_update_time for the first time.
     */
    loop->time = 0;
    uv_update_time(loop);

    //QUEUE_INIT(&loop->wq);
    QUEUE_INIT(&loop->handle_queue);
    loop->active_reqs.count = 0;
    loop->active_handles = 0;

    //loop->pending_reqs_tail = NULL;

    loop->endgame_handles = NULL;

    loop->timer_heap = timer_heap = uv__malloc(sizeof(*timer_heap));
    if (timer_heap == NULL) {
        //err = UV_ENOMEM;
        //goto fail_timers_alloc;
        abort();
    }

    heap_init(timer_heap);

    //loop->check_handles = NULL;
    //loop->prepare_handles = NULL;
    //loop->idle_handles = NULL;

    //loop->next_prepare_handle = NULL;
    //loop->next_check_handle = NULL;
    //loop->next_idle_handle = NULL;

    //memset(&loop->poll_peer_sockets, 0, sizeof loop->poll_peer_sockets);

    //loop->active_tcp_streams = 0;
    //loop->active_udp_streams = 0;

    loop->timer_counter = 0;
    loop->stop_flag = 0;

    /*err = uv_mutex_init(&loop->wq_mutex);
    if (err)
        goto fail_mutex_init;

    err = uv_async_init(loop, &loop->wq_async, uv__work_done);
    if (err)
        goto fail_async_init;

    uv__handle_unref(&loop->wq_async);
    loop->wq_async.flags |= UV_HANDLE_INTERNAL;

    err = uv__loops_add(loop);
    if (err)
        goto fail_async_init;*/

    return 0;
//
//fail_async_init:
//    uv_mutex_destroy(&loop->wq_mutex);
//
//fail_mutex_init:
//    uv__free(timer_heap);
//    loop->timer_heap = NULL;
//
//fail_timers_alloc:
//    uv_mutex_destroy(&lfields->loop_metrics.lock);
//
//fail_metrics_mutex_init:
//    uv__free(lfields);
//    loop->internal_fields = NULL;
//    CloseHandle(loop->iocp);
//    loop->iocp = INVALID_HANDLE_VALUE;

    return err;
}

uv_loop_t* uv_default_loop(void) {
    if (default_loop_ptr != NULL)
        return default_loop_ptr;

    if (uv_loop_init(&default_loop_struct))
        return NULL;

    default_loop_ptr = &default_loop_struct;
    return default_loop_ptr;
}

typedef enum {
    UV_RUN_DEFAULT = 0,
    UV_RUN_ONCE,
    UV_RUN_NOWAIT
} uv_run_mode;


static int uv__loop_alive(const uv_loop_t* loop) {
    return uv__has_active_handles(loop) ||
       // uv__has_active_reqs(loop) ||
        loop->endgame_handles != NULL;
}


int uv_run(uv_loop_t* loop, uv_run_mode mode) {
    DWORD timeout;
    int r;
    //int ran_pending;

    r = uv__loop_alive(loop);
    if (!r)
        uv_update_time(loop);

    while (r != 0 && loop->stop_flag == 0) {
        uv_update_time(loop);
        uv__run_timers(loop);

        //ran_pending = uv_process_reqs(loop);
        //uv_idle_invoke(loop);
        //uv_prepare_invoke(loop);

        timeout = 0;
        //if ((mode == UV_RUN_ONCE && !ran_pending) || mode == UV_RUN_DEFAULT)
        //    timeout = uv_backend_timeout(loop);

       // if (pGetQueuedCompletionStatusEx)
       //     uv__poll(loop, timeout);
        //else
        //    uv__poll_wine(loop, timeout);
        Sleep(timeout);

        /* Run one final update on the provider_idle_time in case uv__poll*
         * returned because the timeout expired, but no events were received. This
         * call will be ignored if the provider_entry_time was either never set (if
         * the timeout == 0) or was already updated b/c an event was received.
         */
        //uv__metrics_update_idle_time(loop);

        //uv_check_invoke(loop);
        //uv_process_endgames(loop);

        if (mode == UV_RUN_ONCE) {
            /* UV_RUN_ONCE implies forward progress: at least one callback must have
             * been invoked when it returns. uv__io_poll() can return without doing
             * I/O (meaning: no callbacks) when its timeout expires - which means we
             * have pending timers that satisfy the forward progress constraint.
             *
             * UV_RUN_NOWAIT makes no guarantees about progress so it's omitted from
             * the check.
             */
            uv__run_timers(loop);
        }

        r = uv__loop_alive(loop);
        if (mode == UV_RUN_ONCE || mode == UV_RUN_NOWAIT)
            break;
    }

    /* The if statement lets the compiler compile it to a conditional store.
     * Avoids dirtying a cache line.
     */
    if (loop->stop_flag != 0)
        loop->stop_flag = 0;

    return r;
}


/* Just sugar for wrapping the main() for a task or helper. */
#define TEST_IMPL(name)                                                       \
  int run_test_##name(void);                                                  \
  int run_test_##name(void)


/* Die with fatal error. */
#define FATAL(msg)                                        \
  do {                                                    \
    fprintf(stderr,                                       \
            "Fatal error in %s on line %d: %s\n",         \
            __FILE__,                                     \
            __LINE__,                                     \
            msg);                                         \
    fflush(stderr);                                       \
    abort();                                              \
  } while (0)

/* Have our own assert, so we are sure it does not get optimized away in
 * a release build.
 */
#define ASSERT(expr)                                      \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in %s on line %d: %s\n",    \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr);                                       \
    abort();                                              \
  }                                                       \
 } while (0)

#define ASSERT_BASE(a, operator, b, type, conv)              \
 do {                                                        \
  volatile type eval_a = (type) (a);                         \
  volatile type eval_b = (type) (b);                         \
  if (!(eval_a operator eval_b)) {                           \
    fprintf(stderr,                                          \
            "Assertion failed in %s on line %d: `%s %s %s` " \
            "(%"conv" %s %"conv")\n",                        \
            __FILE__,                                        \
            __LINE__,                                        \
            #a,                                              \
            #operator,                                       \
            #b,                                              \
            eval_a,                                          \
            #operator,                                       \
            eval_b);                                         \
    abort();                                                 \
  }                                                          \
 } while (0)

#define ASSERT_UINT64_EQ(a, b) ASSERT_BASE(a, ==, b, uint64_t, PRIu64)
#define ASSERT_UINT64_LE(a, b) ASSERT_BASE(a, <=, b, uint64_t, PRIu64)


static int once_cb_called = 0;
static int once_close_cb_called = 0;
static int repeat_cb_called = 0;
static int repeat_close_cb_called = 0;
static int order_cb_called = 0;
static uint64_t start_time;
static uv_timer_t tiny_timer;
static uv_timer_t huge_timer1;
static uv_timer_t huge_timer2;


static void once_close_cb(uv_handle_t * handle) {
    printf("ONCE_CLOSE_CB\n");

    ASSERT(handle != NULL);
    ASSERT(0 == uv_is_active(handle));

    once_close_cb_called++;
}


static void once_cb(uv_timer_t* handle) {
    printf("ONCE_CB %d\n", once_cb_called);

    ASSERT(handle != NULL);
    ASSERT(0 == uv_is_active((uv_handle_t*)handle));

    once_cb_called++;

    uv_close((uv_handle_t*)handle, once_close_cb);

    /* Just call this randomly for the code coverage. */
    uv_update_time(uv_default_loop());
}


static void repeat_close_cb(uv_handle_t* handle) {
    printf("REPEAT_CLOSE_CB\n");

    ASSERT(handle != NULL);

    repeat_close_cb_called++;
}


static void repeat_cb(uv_timer_t* handle) {
    printf("REPEAT_CB\n");

    ASSERT(handle != NULL);
    ASSERT(1 == uv_is_active((uv_handle_t*)handle));

    repeat_cb_called++;

    if (repeat_cb_called == 5) {
        uv_close((uv_handle_t*)handle, repeat_close_cb);
    }
}


static void never_cb(uv_timer_t* handle) {
    FATAL("never_cb should never be called");
}



TEST_IMPL(timer) {
    uv_timer_t once_timers[10];
    uv_timer_t* once;
    uv_timer_t repeat, never;
    unsigned int i;
    int r;

    start_time = uv_now(uv_default_loop());
    ASSERT(0 < start_time);

    /* Let 10 timers time out in 500 ms total. */
    for (i = 0; i < ARRAY_SIZE(once_timers); i++) {
        once = once_timers + i;
        r = uv_timer_init(uv_default_loop(), once);
        ASSERT(r == 0);
        r = uv_timer_start(once, once_cb, i * 50, 0);
        ASSERT(r == 0);
    }

    /* The 11th timer is a repeating timer that runs 4 times */
    r = uv_timer_init(uv_default_loop(), &repeat);
    ASSERT(r == 0);
    r = uv_timer_start(&repeat, repeat_cb, 100, 100);
    ASSERT(r == 0);

    /* The 12th timer should not do anything. */
    r = uv_timer_init(uv_default_loop(), &never);
    ASSERT(r == 0);
    r = uv_timer_start(&never, never_cb, 100, 100);
    ASSERT(r == 0);
    r = uv_timer_stop(&never);
    ASSERT(r == 0);
    uv_unref((uv_handle_t*)&never);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    ASSERT(once_cb_called == 10);
    ASSERT(once_close_cb_called == 10);
    printf("repeat_cb_called %d\n", repeat_cb_called);
    ASSERT(repeat_cb_called == 5);
    ASSERT(repeat_close_cb_called == 1);

    ASSERT(500 <= uv_now(uv_default_loop()) - start_time);

    MAKE_VALGRIND_HAPPY();
    return 0;
}


TEST_IMPL(timer_start_twice) {
    uv_timer_t once;
    int r;

    r = uv_timer_init(uv_default_loop(), &once);
    ASSERT(r == 0);
    r = uv_timer_start(&once, never_cb, 86400 * 1000, 0);
    ASSERT(r == 0);
    r = uv_timer_start(&once, once_cb, 10, 0);
    ASSERT(r == 0);
    r = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    ASSERT(r == 0);

    ASSERT(once_cb_called == 1);

    MAKE_VALGRIND_HAPPY();
    return 0;
}


TEST_IMPL(timer_init) {
    uv_timer_t handle;

    ASSERT(0 == uv_timer_init(uv_default_loop(), &handle));
    ASSERT(0 == uv_timer_get_repeat(&handle));
    ASSERT_UINT64_LE(0, uv_timer_get_due_in(&handle));
    ASSERT(0 == uv_is_active((uv_handle_t*)&handle));

    MAKE_VALGRIND_HAPPY();
    return 0;
}


static void order_cb_a(uv_timer_t* handle) {
    ASSERT(order_cb_called++ == *(int*)handle->data);
}


static void order_cb_b(uv_timer_t* handle) {
    ASSERT(order_cb_called++ == *(int*)handle->data);
}


TEST_IMPL(timer_order) {
    int first;
    int second;
    uv_timer_t handle_a;
    uv_timer_t handle_b;

    first = 0;
    second = 1;
    ASSERT(0 == uv_timer_init(uv_default_loop(), &handle_a));
    ASSERT(0 == uv_timer_init(uv_default_loop(), &handle_b));

    /* Test for starting handle_a then handle_b */
    handle_a.data = &first;
    ASSERT(0 == uv_timer_start(&handle_a, order_cb_a, 0, 0));
    handle_b.data = &second;
    ASSERT(0 == uv_timer_start(&handle_b, order_cb_b, 0, 0));
    ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_DEFAULT));

    ASSERT(order_cb_called == 2);

    ASSERT(0 == uv_timer_stop(&handle_a));
    ASSERT(0 == uv_timer_stop(&handle_b));

    /* Test for starting handle_b then handle_a */
    order_cb_called = 0;
    handle_b.data = &first;
    ASSERT(0 == uv_timer_start(&handle_b, order_cb_b, 0, 0));

    handle_a.data = &second;
    ASSERT(0 == uv_timer_start(&handle_a, order_cb_a, 0, 0));
    ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_DEFAULT));

    ASSERT(order_cb_called == 2);

    MAKE_VALGRIND_HAPPY();
    return 0;
}


static void tiny_timer_cb(uv_timer_t* handle) {
    ASSERT(handle == &tiny_timer);
    uv_close((uv_handle_t*)&tiny_timer, NULL);
    uv_close((uv_handle_t*)&huge_timer1, NULL);
    uv_close((uv_handle_t*)&huge_timer2, NULL);
}


TEST_IMPL(timer_huge_timeout) {
    ASSERT(0 == uv_timer_init(uv_default_loop(), &tiny_timer));
    ASSERT(0 == uv_timer_init(uv_default_loop(), &huge_timer1));
    ASSERT(0 == uv_timer_init(uv_default_loop(), &huge_timer2));
    ASSERT(0 == uv_timer_start(&tiny_timer, tiny_timer_cb, 1, 0));
    ASSERT(0 == uv_timer_start(&huge_timer1, tiny_timer_cb, 0xffffffffffffLL, 0));
    ASSERT(0 == uv_timer_start(&huge_timer2, tiny_timer_cb, (uint64_t)-1, 0));
    ASSERT_UINT64_EQ(1, uv_timer_get_due_in(&tiny_timer));
    ASSERT_UINT64_EQ(281474976710655, uv_timer_get_due_in(&huge_timer1));
    ASSERT_UINT64_LE(0, uv_timer_get_due_in(&huge_timer2));
    ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_DEFAULT));
    MAKE_VALGRIND_HAPPY();
    return 0;
}


static void huge_repeat_cb(uv_timer_t* handle) {
    static int ncalls;

    if (ncalls == 0)
        ASSERT(handle == &huge_timer1);
    else
        ASSERT(handle == &tiny_timer);

    if (++ncalls == 10) {
        uv_close((uv_handle_t*)&tiny_timer, NULL);
        uv_close((uv_handle_t*)&huge_timer1, NULL);
    }
}


TEST_IMPL(timer_huge_repeat) {
    ASSERT(0 == uv_timer_init(uv_default_loop(), &tiny_timer));
    ASSERT(0 == uv_timer_init(uv_default_loop(), &huge_timer1));
    ASSERT(0 == uv_timer_start(&tiny_timer, huge_repeat_cb, 2, 2));
    ASSERT(0 == uv_timer_start(&huge_timer1, huge_repeat_cb, 1, (uint64_t)-1));
    ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_DEFAULT));
    MAKE_VALGRIND_HAPPY();
    return 0;
}


static unsigned int timer_run_once_timer_cb_called;


static void timer_run_once_timer_cb(uv_timer_t* handle) {
    timer_run_once_timer_cb_called++;
}


TEST_IMPL(timer_run_once) {
    uv_timer_t timer_handle;

    ASSERT(0 == uv_timer_init(uv_default_loop(), &timer_handle));
    ASSERT(0 == uv_timer_start(&timer_handle, timer_run_once_timer_cb, 0, 0));
    ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_ONCE));
    ASSERT(1 == timer_run_once_timer_cb_called);

    ASSERT(0 == uv_timer_start(&timer_handle, timer_run_once_timer_cb, 1, 0));
    ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_ONCE));
    ASSERT(2 == timer_run_once_timer_cb_called);

    uv_close((uv_handle_t*)&timer_handle, NULL);
    ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_ONCE));

    MAKE_VALGRIND_HAPPY();
    return 0;
}


TEST_IMPL(timer_is_closing) {
    uv_timer_t handle;

    ASSERT(0 == uv_timer_init(uv_default_loop(), &handle));
    uv_close((uv_handle_t*)&handle, NULL);

    ASSERT(UV_EINVAL == uv_timer_start(&handle, never_cb, 100, 100));

    MAKE_VALGRIND_HAPPY();
    return 0;
}


TEST_IMPL(timer_null_callback) {
    uv_timer_t handle;

    ASSERT(0 == uv_timer_init(uv_default_loop(), &handle));
    ASSERT(UV_EINVAL == uv_timer_start(&handle, NULL, 100, 100));

    MAKE_VALGRIND_HAPPY();
    return 0;
}


static uint64_t timer_early_check_expected_time;


static void timer_early_check_cb(uv_timer_t* handle) {
    uint64_t hrtime = uv_hrtime() / 1000000;
    ASSERT(hrtime >= timer_early_check_expected_time);
}


TEST_IMPL(timer_early_check) {
    uv_timer_t timer_handle;
    const uint64_t timeout_ms = 10;

    timer_early_check_expected_time = uv_now(uv_default_loop()) + timeout_ms;

    ASSERT(0 == uv_timer_init(uv_default_loop(), &timer_handle));
    ASSERT(0 == uv_timer_start(&timer_handle, timer_early_check_cb, timeout_ms, 0));
    ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_DEFAULT));

    uv_close((uv_handle_t*)&timer_handle, NULL);
    ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_DEFAULT));

    MAKE_VALGRIND_HAPPY();
    return 0;
}








int main()
{
    uv__util_init();

    run_test_timer();
    run_test_timer_start_twice();
    run_test_timer_init();
    run_test_timer_order();
    run_test_timer_huge_timeout();
    run_test_timer_huge_repeat();
    run_test_timer_run_once();
    run_test_timer_is_closing();
    run_test_timer_null_callback();
    run_test_timer_early_check();









}
