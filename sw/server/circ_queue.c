/* 
 * A lock-free single-producer circular queue implementation modeled after the
 * more elaborate C++ version from Faustino Frechilla available at:
 * http://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular
 */

#include "circ_queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

circ_queue * init_circ_queue(int len)
{
  int i;
  circ_queue * q;

  q = malloc(sizeof(circ_queue));
  if (q == NULL) {
    printf("Not enough memory to allocate circ_queue\n");
    return NULL;
  }

  q->writeIndex = 0;
  q->readIndex = 0;
  q->len = len;
  q->vals = (int*) malloc(len*sizeof(int));  
  if (q->vals == NULL) {
    printf("Not enough memory to allocate circ_queue array\n");
    return NULL;
  }
  return q;
}

/**
 * Internal function to help count. Returns the queue size normalized position.
 */
unsigned int queue_count_to_index(unsigned int count, unsigned int len)
{
	return (count % len);
}

int push_circ_queue(circ_queue * q, int val)
{
	unsigned int currReadIndex;
	unsigned int currWriteIndex;

	currWriteIndex = q->writeIndex;
	currReadIndex  = q->readIndex;
	if (queue_count_to_index(currWriteIndex+1, q->len) == queue_count_to_index(currReadIndex, q->len)) {
		// The queue is full
		return 1;
	}

	// Save the data into the queue
	q->vals[queue_count_to_index(currWriteIndex, q->len)] = val;
        //printf("%d\n",q->vals[queue_count_to_index(currWriteIndex, q->len)]);
	// Increment atomically write index. Now a consumer thread can read
	// the piece of data that was just stored.
	q->writeIndex = currWriteIndex+1;

	return 0;
}

int pop_circ_queue(circ_queue * q, int * val)
{
	unsigned int currReadIndex;
	unsigned int currMaxReadIndex;

	currReadIndex = q->readIndex;
	currMaxReadIndex = q->writeIndex;
	if (queue_count_to_index(currReadIndex, q->len) == queue_count_to_index(currMaxReadIndex, q->len)) {
		// The queue is empty or a producer thread has allocate space in the queue
		// but is waiting to commit the data into it
		return 1;
	}
	// Retrieve the data from the queue
	*val = q->vals[queue_count_to_index(currReadIndex, q->len)];
        q->readIndex = currReadIndex+1;
	return 0;

}

void free_circ_queue(circ_queue * q)
{
	int i;

	if (q == NULL)
          return;
	free(q->vals);
	free(q);
}

