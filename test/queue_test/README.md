- [libuv QUEUE Note](#libuv-queue-note)
  - [include](#include)
  - [QUEUE](#queue)
  - [Private macros](#private-macros)
  - [Public macros](#public-macros)
    - [QUEUE\_DATA](#queue_data)
    - [QUEUE\_FOREACH](#queue_foreach)
    - [QUEUE\_EMPTY](#queue_empty)
    - [QUEUE\_HEAD](#queue_head)
    - [QUEUE\_INIT](#queue_init)
    - [QUEUE\_ADD](#queue_add)
    - [QUEUE\_SPLIT](#queue_split)
    - [QUEUE\_MOVE](#queue_move)
    - [QUEUE\_INSERT\_HEAD QUEUE\_INSERT\_TAIL](#queue_insert_head-queue_insert_tail)
    - [QUEUE\_REMOVE](#queue_remove)


# libuv QUEUE Note

## include

```c
#include <stddef.h>
```

全部用宏实现，只有 `offsetof` 宏需要引入 `stddef.h`.

## QUEUE

```c
typedef void *QUEUE[2];
```

`QUEUE` 是一个数组结构，包含两个 `void*` 成员，分别存储 `next, prev` 的地址.

当队列为空时，`next/prev` 都指向自身地址；非空时，`next` 指向队首，`prev` 指向队尾.

因此这是一个循环队列.

## Private macros

```c
/* Private macros. */
#define QUEUE_NEXT(q)       (*(QUEUE **) &((*(q))[0]))
#define QUEUE_PREV(q)       (*(QUEUE **) &((*(q))[1]))
#define QUEUE_PREV_NEXT(q)  (QUEUE_NEXT(QUEUE_PREV(q)))
#define QUEUE_NEXT_PREV(q)  (QUEUE_PREV(QUEUE_NEXT(q)))
```

之所以用宏定义实现，一个是减少函数调用，一个是可以直接作为左值使用.

## Public macros

### QUEUE_DATA

```c
/* Public macros. */
#define QUEUE_DATA(ptr, type, field)                                          \
  ((type *) ((char *) (ptr) - offsetof(type, field)))
```

结构体 `type` 有 `field` 成员，`field` 必须是 `QUEUE` 或内存布局与 `QUEUE` 相同的定义如 

```c
struct uv_loop_s {
    void* queue_handle[2];
};
```

`offsetof(type, field)` 用来获取结构体 `type` 的成员 `field` 相对于 `type` 本身的偏移量.

`ptr` 是 `QUEUE*` 类型的一个变量，因此 `QUEUE_DATA(ptr, type, field)` 可以获取存储 `ptr` 的结构体 `type` 的地址.


### QUEUE_FOREACH

```c
/* Important note: mutating the list while QUEUE_FOREACH is
iterating over its elements results in undefined behavior.
 */
#define QUEUE_FOREACH(q, h)                                                   \
  for ((q) = QUEUE_NEXT(h); (q) != (h); (q) = QUEUE_NEXT(q))
```

遍历队列 `h`，每次进入循环体时 `q` 都指向队列成员.

如果 `h` 非空，进入 `for` 循环时 `q` 就指向了队首，后续循环依次后移直到指向队尾.

循环中可以用 QUEUE_DATA(q, type, field) 来获取包含 q 的结构体 type 的指针，如

```c
struct uv_handle_s {
   void* handle_queue[2];
};
```

来看一个实例 `libuv/src/uv-common.c`

```c
int uv_loop_close(uv_loop_t* loop) {
  QUEUE* q;
  uv_handle_t* h;
  QUEUE_FOREACH(q, &loop->handle_queue) {
  h = QUEUE_DATA(q, uv_handle_t, handle_queue);
    if (!(h->flags & UV_HANDLE_INTERNAL))
      return UV_EBUSY;
  }
  ...
}
```

此处，`uv_loop_t` 的成员 `queue_data` 作为队列入口，`uv_handle_t` 的成员 `queue_data` 作为队列成员. 程序运行中不断将 `uv_handle_t::handle_queue` 入队到 `uv_loop_t::handle_queue` 内，此时就可以使用 `QUEUE_FOREACH` 将每个队员取出，并使用 `QUEUE_DATA` 获取 `uv_handle_t` 的地址.

`侵入式结构` 名不虚传.

### QUEUE_EMPTY

```c
#define QUEUE_EMPTY(q)                                                        \
  ((const QUEUE *) (q) == (const QUEUE *) QUEUE_NEXT(q))
```

如上所述，如果队列 `q` 的 `next` 指向自身，就表示队空.

### QUEUE_HEAD

```c
#define QUEUE_HEAD(q)                                                         \
  (QUEUE_NEXT(q))
```

取队首

### QUEUE_INIT

```c
#define QUEUE_INIT(q)                                                         \
  do {                                                                        \
    QUEUE_NEXT(q) = (q);                                                      \
    QUEUE_PREV(q) = (q);                                                      \
  }                                                                           \
  while (0)
```

如上所述，队列初始化时，`next,prev` 都指向自身表示空队.

### QUEUE_ADD

```c
#define QUEUE_ADD(h, n)                                                       \
  do {                                                                        \
    QUEUE_PREV_NEXT(h) = QUEUE_NEXT(n);                                       \
    QUEUE_NEXT_PREV(n) = QUEUE_PREV(h);                                       \
    QUEUE_PREV(h) = QUEUE_PREV(n);                                            \
    QUEUE_PREV_NEXT(h) = (h);                                                 \
  }                                                                           \
  while (0)
```

逻辑上将队列 `n` 合并到队列 `h` 的队尾. `h,n` 都可以是空队.

必须注意到，`h,n` 都是作为容器存在的，`h,n` 非空时，它们本身既不是队首，也不是队尾，而应当看作哨兵.

```c
// 原循环队列 h 的队尾的 next 指向 n 的队首
QUEUE_PREV_NEXT(h) = QUEUE_NEXT(n); 
// n 的队首的 prev 指向 h 的队尾
QUEUE_NEXT_PREV(n) = QUEUE_PREV(h);
// 原队列 h 的队尾指向 n 的队尾
QUEUE_PREV(h) = QUEUE_PREV(n);
// 将合并后的队尾的 next 指向 h，重新形成闭环队列.
QUEUE_PREV_NEXT(h) = (h);
```

实例

假设 `h` 有 3 个队员 `h1, h2, h3`，`n` 有 2 个队员 `n1, n2`，则逻辑上 

```c
h->next = h1
h->prev = h3

h1->next = h2
h1->prev = h

h2->next = h3
h2->prev = h1

h3->next = h
h3->prev = h2


n->next = n1
n->prev = n2

n1->next = n2
n1->prev = n

n2->next = n
n2->prev = n1
```

执行 `QUEUE_PREV_NEXT(h) = QUEUE_NEXT(n);` 相当于 

```c
h3->next = n1
```

执行 `QUEUE_NEXT_PREV(n) = QUEUE_PREV(h);` 相当于

```c
n1->prev = h3
```

执行 `QUEUE_PREV(h) = QUEUE_PREV(n);` 相当于

```c
h->prev = n2
```

执行 `QUEUE_PREV_NEXT(h) = (h);` 相当于

```c
h->prev->next = h
```

而 `h->prev` 即为 `n2`，所以相当于 `n2->next = h`

综上，此时状态为

```c
h->next = h1
h->prev = n2

h1->next = h2
h1->prev = h

h2->next = h3
h2->prev = h1

h3->next = n1
h3->prev = n2

n1->next = n2
n1->prev = h3

n2->next = h
n2->prev = n1
```

从而再次形成闭环队列.

可以注意到此时 

```c
n->next = n1
n->prev = n2
```

`n` 并没有发生变化，因此此后 `n` 都不应该再被使用了.

*`n` 为空队时，执行上述逻辑后 `n->prev` 会改变，因此，无论 `n` 是否为空，后面都不应再使用 `n` 了.* 


### QUEUE_SPLIT

```c
#define QUEUE_SPLIT(h, q, n)                                                  \
  do {                                                                        \
    QUEUE_PREV(n) = QUEUE_PREV(h);                                            \
    QUEUE_PREV_NEXT(n) = (n);                                                 \
    QUEUE_NEXT(n) = (q);                                                      \
    QUEUE_PREV(h) = QUEUE_PREV(q);                                            \
    QUEUE_PREV_NEXT(h) = (h);                                                 \
    QUEUE_PREV(q) = (n);                                                      \
  }                                                                           \
  while (0)
```

将队列 `h` 从节点 `q` 开始到队尾，分裂出来存储到 `n`，`h` 只保留队首到 `q` 的前一个节点. 

`n` 须为空队，否则数据会被覆盖.

### QUEUE_MOVE

```c
#define QUEUE_MOVE(h, n)                                                      \
  do {                                                                        \
    if (QUEUE_EMPTY(h))                                                       \
      QUEUE_INIT(n);                                                          \
    else {                                                                    \
      QUEUE* q = QUEUE_HEAD(h);                                               \
      QUEUE_SPLIT(h, q, n);                                                   \
    }                                                                         \
  }                                                                           \
  while (0)
```

同 `QUEUE_SPLIT`，将 `h` 整体转移至 `n`，之后 `h` 为空队.

`n` 须为空队，否则数据会被覆盖.

### QUEUE_INSERT_HEAD QUEUE_INSERT_TAIL

```c
#define QUEUE_INSERT_HEAD(h, q)                                               \
  do {                                                                        \
    QUEUE_NEXT(q) = QUEUE_NEXT(h);                                            \
    QUEUE_PREV(q) = (h);                                                      \
    QUEUE_NEXT_PREV(q) = (q);                                                 \
    QUEUE_NEXT(h) = (q);                                                      \
  }                                                                           \
  while (0)

#define QUEUE_INSERT_TAIL(h, q)                                               \
  do {                                                                        \
    QUEUE_NEXT(q) = (h);                                                      \
    QUEUE_PREV(q) = QUEUE_PREV(h);                                            \
    QUEUE_PREV_NEXT(q) = (q);                                                 \
    QUEUE_PREV(h) = (q);                                                      \
  }                                                                           \
  while (0)
```

将节点 `q` 作为队首或队尾插入队列 `h`

### QUEUE_REMOVE

```c
#define QUEUE_REMOVE(q)                                                       \
  do {                                                                        \
    QUEUE_PREV_NEXT(q) = QUEUE_NEXT(q);                                       \
    QUEUE_NEXT_PREV(q) = QUEUE_PREV(q);                                       \
  }                                                                           \
  while (0)
```

将节点 `q` 从队列中摘除.



