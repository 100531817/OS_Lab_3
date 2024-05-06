//SSOO-P3 23/24

#ifndef HEADER_FILE
#define HEADER_FILE

#include <pthread.h>

// Define the structure for the elements in the queue
typedef struct operation {
    int product;   // Product identifier
    int op;        // Operation type (0 for purchase, 1 for sale)
    int units;     // Number of units involved in the operation
} element;

typedef struct queue {
    element **buffer;
    int size;
    int front, rear, count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty, not_full;
}queue;

// Function prototypes
queue* queue_init(int num_elements);
int queue_destroy(queue* q);
int queue_put(queue* q, element *ele);
element* queue_get(queue* q);
int queue_empty(queue *q);
int queue_full(queue *q);


#endif
