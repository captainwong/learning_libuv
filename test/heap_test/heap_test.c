#include "heap-inl.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>


#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))


struct node {
	void* heap_node[3];
	int val;
};

struct node* newnode(int n)
{
	struct node* node = malloc(sizeof(*node));
	node->val = n;
	return node;
}

int cmp(const struct heap_node* ha, const struct heap_node* hb)
{
	const struct node* a;
	const struct node* b;
	a = container_of(ha, struct node, heap_node);
	b = container_of(hb, struct node, heap_node);

	return a->val < b->val;
}

struct MyQueue {
	QUEUE queue;
	struct node* node;
};

void myq_init(struct MyQueue* q) {
	QUEUE_INIT(&q->queue);
	q->node = NULL;
}

struct MyQueue* myq_new(struct node* node) {
	struct MyQueue* q = malloc(sizeof(*q));
	QUEUE_INIT(&q->queue);
	q->node = node;
	return q;
}

void myq_enqueue(struct MyQueue* q, struct node* node) {
	struct MyQueue* newnode = myq_new(node);
	QUEUE_INSERT_TAIL(&q->queue, &newnode->queue);
}

struct MyQueue* myq_dequeue(struct MyQueue* q) {
	QUEUE* head = QUEUE_NEXT(&q->queue);
	struct MyQueue* myhead = QUEUE_DATA(head, struct MyQueue, queue);
	QUEUE_REMOVE(head);
	return myhead;
}

void myq_destroy(struct MyQueue* q) {
	free(q);
}

void print_heap(struct heap* h)
{
	if (h->nelts == 0) {
		printf("heap empty\n");
		return;
	}
	printf("heap nelts=%d, nodes=", h->nelts);
	struct heap_node* node = h->min;
	struct MyQueue queue;
	myq_init(&queue);
	myq_enqueue(&queue, container_of(h->min, struct node, heap_node));
	while (!QUEUE_EMPTY(&queue.queue)) {
		struct MyQueue* x = myq_dequeue(&queue);
		printf("%d, ", container_of(x->node, struct node, heap_node)->val);
		if (((struct heap_node*)(x->node->heap_node))->left) {
			myq_enqueue(&queue, container_of(((struct heap_node*)(x->node->heap_node))->left, struct node, heap_node));
		}
		if (((struct heap_node*)(x->node->heap_node))->right) {
			myq_enqueue(&queue, container_of(((struct heap_node*)(x->node->heap_node))->right, struct node, heap_node));
		}
		myq_destroy(x);
	}
	printf("\n");
}


int main()
{
	struct heap h;
	heap_init(&h);

	struct node* nodes[7];
	for (int i = 0; i < 7; i++) {
		nodes[i] = newnode(i);
		printf("inserting %d\n", i);
		heap_insert(&h, (struct heap_node*)&nodes[i]->heap_node, cmp);
		print_heap(&h);
	}

	for (int i = 0; i < 7; i++) {
		printf("removing %d\n", i);
		heap_remove(&h, (struct heap_node*)&nodes[i]->heap_node, cmp);
		print_heap(&h);
	}
	for (int i = 0; i < 7; i++) {
		nodes[i]->val = 6 - i;
		printf("inserting %d\n", 6 - i);
		heap_insert(&h, (struct heap_node*)&nodes[i]->heap_node, cmp);
		print_heap(&h);
	}
	for (int i = 0; i < 7; i++) {
		printf("removing %d\n", i);
		struct node* node = nodes[6 - i];
		heap_remove(&h, (struct heap_node*)&node->heap_node, cmp);
		print_heap(&h);
	}
}
