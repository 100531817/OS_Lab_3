//SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_PRODUCTS 5
#define MAX_OPERATIONS 500

// Product Information
typedef struct {
    int purchase_cost;
    int sale_price;
} product_info;

// Operation Structure
typedef struct {
    int product_id;
    int type;  // 0 = PURCHASE, 1 = SALE
    int units;
} operation;

// Producer Argument Structure
typedef struct {
    int start;
    int end;
    queue *buffer;
    operation *operations;
} producer_arg;

// Consumer Argument Structure
typedef struct {
    queue *buffer;
    int *profits;
    int *product_stock;
    pthread_mutex_t *profits_lock;
} consumer_arg;

product_info products[MAX_PRODUCTS] = {
        {2, 3},
        {5, 10},
        {15, 20},
        {25, 40},
        {100, 125}
};

// Producer Thread Function
void *producer_thread(void *arg) {
    producer_arg *params = (producer_arg *)arg;
    for (int i = params->start; i < params->end; i++) {
        element *ele = malloc(sizeof(element));
        ele->product = params->operations[i].product_id;
        ele->op = params->operations[i].type;
        ele->units = params->operations[i].units;
        queue_put(params->buffer, ele);
    }
    pthread_exit(NULL);
}

// Consumer Thread Function
void *consumer_thread(void *arg) {
    consumer_arg *params = (consumer_arg *)arg;
    element *ele;
    while ((ele = queue_get(params->buffer)) != NULL) {
        int product_id = ele->product - 1;  // 0-based indexing
        pthread_mutex_lock(params->profits_lock);
        if (ele->op == 0) {  // PURCHASE
            params->product_stock[product_id] += ele->units;
            *(params->profits) -= products[product_id].purchase_cost * ele->units;
        } else if (ele->op == 1) {  // SALE
            params->product_stock[product_id] -= ele->units;
            *(params->profits) += products[product_id].sale_price * ele->units;
        }
        pthread_mutex_unlock(params->profits_lock);
        free(ele);
    }
    pthread_exit(NULL);
}

int main(int argc, const char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <file name> <num producers> <num consumers> <buff size>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *file_name = argv[1];
    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);
    int buff_size = atoi(argv[4]);

    if (num_producers <= 0 || num_consumers <= 0 || buff_size <= 0) {
        fprintf(stderr, "Invalid parameters. All values must be positive.\n");
        return EXIT_FAILURE;
    }

    FILE *file = fopen(file_name, "r");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    int num_operations;
    if (fscanf(file, "%d", &num_operations) != 1) {
        fprintf(stderr, "Error reading number of operations\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    operation *operations = malloc(num_operations * sizeof(operation));
    for (int i = 0; i < num_operations; i++) {
        char op_type[10];
        if (fscanf(file, "%d %s %d", &operations[i].product_id, op_type, &operations[i].units) != 3) {
            fprintf(stderr, "Error reading operation data at index %d\n", i);
            free(operations);
            fclose(file);
            return EXIT_FAILURE;
        }
        operations[i].type = (strcmp(op_type, "PURCHASE") == 0) ? 0 : 1;
    }
    fclose(file);

    queue *buffer = queue_init(buff_size);

    // Launch Producers
    pthread_t producers[num_producers];
    producer_arg producer_args[num_producers];
    int ops_per_producer = num_operations / num_producers;
    int remainder = num_operations % num_producers;

    for (int i = 0, start = 0; i < num_producers; i++) {
        int end = start + ops_per_producer + (i < remainder ? 1 : 0);
        producer_args[i].start = start;
        producer_args[i].end = end;
        producer_args[i].buffer = buffer;
        producer_args[i].operations = operations;
        pthread_create(&producers[i], NULL, producer_thread, &producer_args[i]);
        start = end;
    }

    // Launch Consumers
    int profits = 0;
    int product_stock[MAX_PRODUCTS] = {0};
    pthread_mutex_t profits_lock;
    pthread_mutex_init(&profits_lock, NULL);

    pthread_t consumers[num_consumers];
    consumer_arg consumer_args[num_consumers];

    for (int i = 0; i < num_consumers; i++) {
        consumer_args[i].buffer = buffer;
        consumer_args[i].profits = &profits;
        consumer_args[i].product_stock = product_stock;
        consumer_args[i].profits_lock = &profits_lock;
        pthread_create(&consumers[i], NULL, consumer_thread, &consumer_args[i]);
    }

    // Wait for Producers to Finish
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }

    // Signal Consumers to Finish
    for (int i = 0; i < num_consumers; i++) {
        queue_put(buffer, NULL);  // Signal consumer to finish
    }

    // Wait for Consumers to Finish
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }

    pthread_mutex_destroy(&profits_lock);
    queue_destroy(buffer);
    free(operations);

    // Output
    printf("Total: %d euros\n", profits);
    printf("Stock:\n");
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        printf("  Product %d: %d\n", i + 1, product_stock[i]);
    }

    return 0;
}
