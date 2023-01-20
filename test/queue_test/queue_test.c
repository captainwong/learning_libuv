#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct Q {
	void* queue[2];
	int val;
}Q;

void print_q(Q* q, char* name)
{
	printf("%s: ", name);
	if (QUEUE_EMPTY(&q->queue)) {
		printf("empty\n");
	} else {
		QUEUE* p;
		Q* i;
		QUEUE_FOREACH(p, &q->queue) {
			i = QUEUE_DATA(p, Q, queue);
			printf("%d ", i->val);
		}
		printf("\n");
	}
}

void queue_add(QUEUE* h, QUEUE* n)
{
	// 原循环队列 h 的队尾的 next 指向 n 的队首
	QUEUE_PREV_NEXT(h) = QUEUE_NEXT(n);
	// n 的队首的 prev 指向 h 的队尾
	QUEUE_NEXT_PREV(n) = QUEUE_PREV(h);
	// 原队列 h 的队尾指向 n 的队尾
	QUEUE_PREV(h) = QUEUE_PREV(n);
	// 将合并后的队尾的 next 指向哨兵、容器 h，重新形成闭环队列.
	QUEUE_PREV_NEXT(h) = (h);
}

int main()
{
	Q h;
	h.val = -1;
	QUEUE_INIT(&h.queue);
	print_q(&h, "h");

	// test QUEUE_ADD
	{
		Q n;
		n.val = -1;
		QUEUE_INIT(&n.queue);
		printf("merge two empty queues\n");
		queue_add(&h.queue, &n.queue);
		print_q(&h, "h");

		Q n1; n1.val = 1;
		QUEUE_INIT(&n.queue);
		QUEUE_INSERT_TAIL(&n.queue, &n1.queue);
		print_q(&n, "n");
		printf("merge 1 elment queue to empty queue\n");
		queue_add(&h.queue, &n.queue);
		print_q(&h, "h");

		QUEUE_REMOVE(&n1.queue);
		print_q(&h, "h");
	}


	Q qs[5];
	for (int i = 0; i < 5; i++) {
		qs[i].val = i;
		QUEUE_INSERT_TAIL(&h.queue, &qs[i].queue);
	}
	print_q(&h, "h");


	// test QUEUE_SPLIT
	{
		QUEUE* q = QUEUE_HEAD(&h.queue);
		q = QUEUE_NEXT(q);
		Q n;
		QUEUE_SPLIT(&h.queue, q, &n.queue);
		printf("split 1-4 to n\n");
		print_q(&h, "h");
		print_q(&n, "n");
	}

	printf("clearing h\n");
	while (!QUEUE_EMPTY(&h.queue)) {
		QUEUE_REMOVE(QUEUE_HEAD(&h.queue));
	}
	print_q(&h, "h");

	QUEUE_REMOVE(&qs[0].queue);
	print_q(&h, "h");

	QUEUE_REMOVE(&qs[1].queue);
	print_q(&h, "h");

	QUEUE_REMOVE(&qs[2].queue);
	print_q(&h, "h");

	QUEUE_REMOVE(&qs[3].queue);
	print_q(&h, "h");

	QUEUE_REMOVE(&qs[4].queue);
	print_q(&h, "h");

	for (int i = 0; i < 5; i++) {
		qs[i].val = 4-i;
		QUEUE_INSERT_TAIL(&h.queue, &qs[i].queue);
	}
	print_q(&h, "h");

	printf("\nmove h to n\n");
	Q n;
	QUEUE_MOVE(&h.queue, &n.queue);
	print_q(&h, "h");
	print_q(&n, "n");

}
