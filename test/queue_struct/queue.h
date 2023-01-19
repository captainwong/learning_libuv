#pragma once

#include <stddef.h>

// 头尾相接的循环队列，队列为空时 prev next 都指向自身
typedef struct QUEUE {
	struct QUEUE* next;
	struct QUEUE* prev;
}QUEUE;

static inline QUEUE* QUEUE_NEXT(QUEUE* q) {
	return q->next;
}

static inline QUEUE* QUEUE_PREV(QUEUE* q) {
	return q->prev;
}

static inline QUEUE* QUEUE_PREV_NEXT(QUEUE* q) {
	return q->prev->next;
}

static inline QUEUE* QUEUE_NEXT_PREV(QUEUE* q) {
	return q->next->prev;
}

#define QUEUE_DATA(ptr, type, field) \
	((type*) ((char*)(ptr) - offsetof(type, field)))

#define QUEUE_FOREACH(q, h) \
	for((q) = QUEUE_NEXT(h); (q) != (h); (q) = QUEUE_NEXT(q))

static inline int QUEUE_EMPTY(QUEUE* q) {
	return q == q->next;
}

static inline QUEUE* QUEUE_HEAD(QUEUE* q) {
	return q->next;
}

static inline void QUEUE_INIT(QUEUE* q) {
	q->next = q->prev = q;
}

// merge n to h
static inline void QUEUE_ADD(QUEUE* h, QUEUE* n) {
	h->prev->next = n->next;
	n->next->prev = h->prev;
	h->prev = n->prev;
	h->prev->next = h;
}

// split h from q to tail, move them to n
static inline void QUEUE_SPLIT(QUEUE* h, QUEUE* q, QUEUE* n) {
	n->prev = h->prev;
	n->prev->next = n;
	n->next = q;
	h->prev = q->prev;
	h->prev->next = h;
	q->prev = n;
}

// move h's nodes to n, h will be empty
static inline void QUEUE_MOVE(QUEUE* h, QUEUE* n) {
	if (QUEUE_EMPTY(h)) {
		QUEUE_INIT(n);
	} else {
		QUEUE* q = QUEUE_HEAD(h);
		QUEUE_SPLIT(h, q, n);
	}
}

static inline void QUEUE_INSERT_HEAD(QUEUE* h, QUEUE* q) {
	q->next = h->next;
	q->prev = h;
	q->next->prev = q;
	h->next = q;
}

static inline void QUEUE_INSERT_TAIL(QUEUE* h, QUEUE* q) {
	q->next = h;
	q->prev = h->prev;
	q->prev->next = q;
	h->prev = q;
}

static inline void QUEUE_REMOVE(QUEUE* q) {
	q->prev->next = q->next;
	q->next->prev = q->prev;
}



