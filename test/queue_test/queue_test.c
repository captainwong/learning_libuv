#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct Q {
	void* queue[2];
	int val;
}Q;

void print_q(Q* q)
{
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

int main()
{
	Q q_container;
	q_container.val = 0;
	QUEUE_INIT(&q_container.queue);
	print_q(&q_container);

	Q qs[5];
	for (int i = 0; i < 5; i++) {
		qs[i].val = i;
		QUEUE_INSERT_TAIL(&q_container.queue, &qs[i].queue);
	}
	print_q(&q_container);

	QUEUE_REMOVE(&qs[0].queue);
	print_q(&q_container);

	QUEUE_REMOVE(&qs[1].queue);
	print_q(&q_container);

	QUEUE_REMOVE(&qs[2].queue);
	print_q(&q_container);

	QUEUE_REMOVE(&qs[3].queue);
	print_q(&q_container);

	QUEUE_REMOVE(&qs[4].queue);
	print_q(&q_container);

	for (int i = 0; i < 5; i++) {
		qs[i].val = i;
		QUEUE_INSERT_TAIL(&q_container.queue, &qs[i].queue);

	}
	print_q(&q_container);

	QUEUE Queue;


	QUEUE_MOVE(&q_container.queue, &Queue);

	print_q(&q_container);

}
