#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "heap-inl.h"
#include "queue.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))


#define UV_HANDLE_TYPE_MAP(XX)                                                \
  XX(ASYNC, async)                                                            \
  XX(CHECK, check)                                                            \
  XX(FS_EVENT, fs_event)                                                      \
  XX(FS_POLL, fs_poll)                                                        \
  XX(HANDLE, handle)                                                          \
  XX(IDLE, idle)                                                              \
  XX(NAMED_PIPE, pipe)                                                        \
  XX(POLL, poll)                                                              \
  XX(PREPARE, prepare)                                                        \
  XX(PROCESS, process)                                                        \
  XX(STREAM, stream)                                                          \
  XX(TCP, tcp)                                                                \
  XX(TIMER, timer)                                                            \
  XX(TTY, tty)                                                                \
  XX(UDP, udp)                                                                \
  XX(SIGNAL, signal)                                                          \

typedef enum {
	UV_UNKNOWN_HANDLE = 0,
#define XX(uc, lc) UV_##uc,
	UV_HANDLE_TYPE_MAP(XX)
#undef XX
	UV_FILE,
	UV_HANDLE_TYPE_MAX
} uv_handle_type;


typedef struct uv_loop_s uv_loop_t;
typedef struct uv_handle_s uv_handle_t;
typedef struct uv_timer_s uv_timer_t;

typedef void (*uv_close_cb)(uv_handle_t* handle);
typedef void (*uv_timer_cb)(uv_timer_t* handle);

#define UV_TIMER 1
#define UV_EINVAL -1


#define UV_HANDLE_PRIVATE_FIELDS                                              \
  uv_handle_t* endgame_next;                                                  \
  unsigned int flags;

#define UV_HANDLE_FIELDS                                                      \
  /* public */                                                                \
  void* data;                                                                 \
  /* read-only */                                                             \
  uv_loop_t* loop;                                                            \
  uv_handle_type type;                                                        \
  /* private */                                                               \
  uv_close_cb close_cb;                                                       \
  void* handle_queue[2];                                                      \
  union {                                                                     \
    int fd;                                                                   \
    void* reserved[4];                                                        \
  } u;                                                                        \
  UV_HANDLE_PRIVATE_FIELDS                                                    \

/* The abstract base class of all handles. */
struct uv_handle_s {
	UV_HANDLE_FIELDS
};

#define UV_TIMER_PRIVATE_FIELDS                                               \
  void* heap_node[3];                                                         \
  int unused;                                                                 \
  uint64_t timeout;                                                           \
  uint64_t repeat;                                                            \
  uint64_t start_id;                                                          \
  uv_timer_cb timer_cb;

/*
 * uv_timer_t is a subclass of uv_handle_t.
 *
 * Used to get woken up at a specified time in the future.
 */
struct uv_timer_s {
	UV_HANDLE_FIELDS
	UV_TIMER_PRIVATE_FIELDS
};

int uv_timer_init(uv_loop_t*, uv_timer_t* handle);
int uv_timer_start(uv_timer_t* handle,
				   uv_timer_cb cb,
				   uint64_t timeout,
				   uint64_t repeat);
int uv_timer_stop(uv_timer_t* handle);
int uv_timer_again(uv_timer_t* handle);
void uv_timer_set_repeat(uv_timer_t* handle, uint64_t repeat);
uint64_t uv_timer_get_repeat(const uv_timer_t* handle);
uint64_t uv_timer_get_due_in(const uv_timer_t* handle);

int uv__next_timeout(const uv_loop_t* loop);
void uv__run_timers(uv_loop_t* loop);
void uv__timer_close(uv_timer_t* handle);

#define UV_LOOP_PRIVATE_FIELDS                                                \
    /* The loop's I/O completion port */                                      \
  /*HANDLE iocp;*/                                                                \
  /* The current time according to the event loop. in msecs. */               \
  uint64_t time;                                                              \
  /* Tail of a single-linked circular queue of pending reqs. If the queue */  \
  /* is empty, tail_ is NULL. If there is only one item, */                   \
  /* tail_->next_req == tail_ */                                              \
  /*uv_req_t* pending_reqs_tail;    */                                                \
  /* Head of a single-linked list of closed handles */                        \
  uv_handle_t* endgame_handles;                                               \
  /* TODO(bnoordhuis) Stop heap-allocating |timer_heap| in libuv v2.x. */     \
  void* timer_heap;                                                           \
    /* Lists of active loop (prepare / check / idle) watchers */              \
  /*uv_prepare_t* prepare_handles; */                                              \
  /*uv_check_t* check_handles;*/                                                   \
  /*uv_idle_t* idle_handles; */                                                    \
  /* This pointer will refer to the prepare/check/idle handle whose */        \
  /* callback is scheduled to be called next. This is needed to allow */      \
  /* safe removal from one of the lists above while that list being */        \
  /* iterated over. */                                                        \
  /*uv_prepare_t* next_prepare_handle;*/                                          \
  /*uv_check_t* next_check_handle;*/                                              \
  /*uv_idle_t* next_idle_handle;*/                                                \
  /* This handle holds the peer sockets for the fast variant of uv_poll_t */  \
  /*SOCKET poll_peer_sockets[UV_MSAFD_PROVIDER_COUNT];*/                          \
  /* Counter to keep track of active tcp streams */                           \
  /*unsigned int active_tcp_streams;*/                                            \
  /* Counter to keep track of active udp streams */                           \
  /*unsigned int active_udp_streams;*/                                            \
  /* Counter to started timer */                                              \
  uint64_t timer_counter;                                                     \
  /* Threadpool */                                                            \
  /*void* wq[2];   */                                                              \
  /*uv_mutex_t wq_mutex;*/                                                        \
  /*uv_async_t wq_async;*/


struct uv_loop_s {
	/* User data - use this for whatever. */
	void* data;
	/* Loop reference counting. */
	unsigned int active_handles;
	void* handle_queue[2];
	union {
		void* unused;
		unsigned int count;
	} active_reqs;
	/* Internal storage for future extensions. */
	void* internal_fields;
	/* Internal flag to signal loop stop. */
	unsigned int stop_flag;
	UV_LOOP_PRIVATE_FIELDS
};


/* Handle flags. Some flags are specific to Windows or UNIX. */
enum {
	/* Used by all handles. */
	UV_HANDLE_CLOSING = 0x00000001,
	UV_HANDLE_CLOSED = 0x00000002,
	UV_HANDLE_ACTIVE = 0x00000004,
	UV_HANDLE_REF = 0x00000008,
	UV_HANDLE_INTERNAL = 0x00000010,
	UV_HANDLE_ENDGAME_QUEUED = 0x00000020,
};


#define uv__is_active(h)                                                      \
  (((h)->flags & UV_HANDLE_ACTIVE) != 0)

#define uv__is_closing(h)                                                     \
  (((h)->flags & (UV_HANDLE_CLOSING | UV_HANDLE_CLOSED)) != 0)


#define uv__has_active_handles(loop)                                          \
  ((loop)->active_handles > 0)

#define uv__active_handle_add(h)                                              \
  do {                                                                        \
    (h)->loop->active_handles++;                                              \
  }                                                                           \
  while (0)

#define uv__active_handle_rm(h)                                               \
  do {                                                                        \
    (h)->loop->active_handles--;                                              \
  }                                                                           \
  while (0)

#define uv__handle_start(h)                                                   \
  do {                                                                        \
    if (((h)->flags & UV_HANDLE_ACTIVE) != 0) break;                          \
    (h)->flags |= UV_HANDLE_ACTIVE;                                           \
    if (((h)->flags & UV_HANDLE_REF) != 0) uv__active_handle_add(h);          \
  }                                                                           \
  while (0)

#define uv__handle_stop(h)                                                    \
  do {                                                                        \
    if (((h)->flags & UV_HANDLE_ACTIVE) == 0) break;                          \
    (h)->flags &= ~UV_HANDLE_ACTIVE;                                          \
    if (((h)->flags & UV_HANDLE_REF) != 0) uv__active_handle_rm(h);           \
  }                                                                           \
  while (0)

#define uv__handle_ref(h)                                                     \
  do {                                                                        \
    if (((h)->flags & UV_HANDLE_REF) != 0) break;                             \
    (h)->flags |= UV_HANDLE_REF;                                              \
    if (((h)->flags & UV_HANDLE_CLOSING) != 0) break;                         \
    if (((h)->flags & UV_HANDLE_ACTIVE) != 0) uv__active_handle_add(h);       \
  }                                                                           \
  while (0)

#define uv__handle_unref(h)                                                   \
  do {                                                                        \
    if (((h)->flags & UV_HANDLE_REF) == 0) break;                             \
    (h)->flags &= ~UV_HANDLE_REF;                                             \
    if (((h)->flags & UV_HANDLE_CLOSING) != 0) break;                         \
    if (((h)->flags & UV_HANDLE_ACTIVE) != 0) uv__active_handle_rm(h);        \
  }                                                                           \
  while (0)


#if defined(_WIN32)
# define uv__handle_platform_init(h) ((h)->u.fd = -1)
#else
# define uv__handle_platform_init(h) ((h)->next_closing = NULL)
#endif

#define uv__handle_init(loop_, h, type_)                                      \
  do {                                                                        \
    (h)->loop = (loop_);                                                      \
    (h)->type = (type_);                                                      \
    (h)->flags = UV_HANDLE_REF;  /* Ref the loop when active. */              \
    QUEUE_INSERT_TAIL(&(loop_)->handle_queue, &(h)->handle_queue);            \
    uv__handle_platform_init(h);                                              \
  }                                                                           \
  while (0)


#define uv__handle_closing(handle)                                      \
  do {                                                                  \
    assert(!((handle)->flags & UV_HANDLE_CLOSING));                     \
                                                                        \
    if (!(((handle)->flags & UV_HANDLE_ACTIVE) &&                       \
          ((handle)->flags & UV_HANDLE_REF)))                           \
      uv__active_handle_add((uv_handle_t*) (handle));                   \
                                                                        \
    (handle)->flags |= UV_HANDLE_CLOSING;                               \
    (handle)->flags &= ~UV_HANDLE_ACTIVE;                               \
  } while (0)




static int uv_is_active(const uv_handle_t* handle) {
	return (handle->flags & UV_HANDLE_ACTIVE) &&
		!(handle->flags & UV_HANDLE_CLOSING);
}

#define uv__handle_close(handle)                                        \
  do {                                                                  \
    QUEUE_REMOVE(&(handle)->handle_queue);                              \
    uv__active_handle_rm((uv_handle_t*) (handle));                      \
                                                                        \
    (handle)->flags |= UV_HANDLE_CLOSED;                                \
                                                                        \
    if ((handle)->close_cb)                                             \
      (handle)->close_cb((uv_handle_t*) (handle));                      \
  } while (0)


static void uv_want_endgame(uv_loop_t* loop, uv_handle_t* handle) {
    if (!(handle->flags & UV_HANDLE_ENDGAME_QUEUED)) {
        handle->flags |= UV_HANDLE_ENDGAME_QUEUED;

        handle->endgame_next = loop->endgame_handles;
        loop->endgame_handles = handle;
    }
}

static void uv_close(uv_handle_t* handle, uv_close_cb cb) {
    uv_loop_t* loop = handle->loop;

    if (handle->flags & UV_HANDLE_CLOSING) {
        assert(0);
        return;
    }

    handle->close_cb = cb;

    /* Handle-specific close actions */
    switch (handle->type) {
    case UV_TIMER:
        uv_timer_stop((uv_timer_t*)handle);
        uv__handle_closing(handle);
        uv_want_endgame(loop, handle);
        return;


    default:
        /* Not supported */
        abort();
    }
}

static void uv_unref(uv_handle_t* handle) {
    uv__handle_unref(handle);
}

static uint64_t uv_now(const uv_loop_t* loop) {
    return loop->time;
}



uint64_t uv_hrtime(void);
uint64_t uv__hrtime(unsigned int scale);

static void uv_update_time(uv_loop_t* loop) {
    uint64_t new_time = uv__hrtime(1000);
    assert(new_time >= loop->time);
    loop->time = new_time;
}








