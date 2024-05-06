//SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"

// Initialize the queue
queue* queue_init(int num_elements) {
    queue *q = malloc(sizeof(queue));
    q->buffer = malloc(num_elements * sizeof(element *));
    q->size = num_elements;
    q->front = q->rear = q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return q;
}

// Destroy the queue and free associated resources
int queue_destroy(queue* q) {
    if (q->count != 0) {
        return -1;
    }
    free(q->buffer);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    free(q);
    return 0;
}

// Add an element to the queue
int queue_put(queue* q, element *ele) {
    pthread_mutex_lock(&q->lock);
    while (queue_full(q)) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    q->buffer[q->rear] = ele;
    q->rear = (q->rear + 1) % q->size;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

// Remove and return an element from the queue
element* queue_get(queue* q) {
    pthread_mutex_lock(&q->lock);
    while (queue_empty(q)) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    element *ele = q->buffer[q->front];
    q->front = (q->front + 1) % q->size;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return ele;
}

// Check if the queue is empty
int queue_empty(queue* q) {
    return q->count == 0;
}

// Check if the queue is full
int queue_full(queue* q) {
    return q->count == q->size;
}
