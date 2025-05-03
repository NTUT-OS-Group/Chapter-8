#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CUSTOMERS_COUNT 5
#define RESOURCES_TYPE_COUNT 4

int available[RESOURCES_TYPE_COUNT];
int maximum[CUSTOMERS_COUNT][RESOURCES_TYPE_COUNT];
int allocation[CUSTOMERS_COUNT][RESOURCES_TYPE_COUNT];
int need[CUSTOMERS_COUNT][RESOURCES_TYPE_COUNT];

static pthread_mutex_t request_mutex = PTHREAD_MUTEX_INITIALIZER;

bool is_safe() {
    int current[RESOURCES_TYPE_COUNT];
    bool finish[CUSTOMERS_COUNT];
    for (int i = 0; i < RESOURCES_TYPE_COUNT; i++)
        current[i] = available[i];
    for (int i = 0; i < CUSTOMERS_COUNT; i++)
        finish[i] = false;
    int count = 0;
    while (count < CUSTOMERS_COUNT) {
        bool found = false;
        for (int i = 0; i < CUSTOMERS_COUNT; i++) {
            if (finish[i])
                continue;
            bool possible = true;
            for (int j = 0; j < RESOURCES_TYPE_COUNT; j++) {
                if (need[i][j] > current[j]) {
                    possible = false;
                    break;
                }
            }
            if (!possible)
                continue;
            for (int j = 0; j < RESOURCES_TYPE_COUNT; j++)
                current[j] += allocation[i][j];
            finish[i] = true;
            found = true;
            count++;
        }
        if (!found)
            return false;
    }
    return true;
}

bool request_resources(int customer_id, int request[]) {
    printf("Customer %d requesting: [ ", customer_id);
    for (int i = 0; i < RESOURCES_TYPE_COUNT; i++)
        printf("%d ", request[i]);
    printf("]\n");
    bool possible = true;
    for (int i = 0; i < RESOURCES_TYPE_COUNT; i++) {
        if (request[i] > need[customer_id][i] || request[i] > available[i]) {
            possible = false;
            break;
        }
    }
    if (!possible) {
        printf("Customer %d request DENIED (invalid or unavailable).\n",
               customer_id);
        return false;
    }
    for (int i = 0; i < RESOURCES_TYPE_COUNT; i++) {
        available[i] -= request[i];
        allocation[customer_id][i] += request[i];
        need[customer_id][i] -= request[i];
    }
    if (is_safe()) {
        printf("Customer %d request GRANTED.\n", customer_id);
        return true;
    }
    printf("Customer %d request DENIED (unsafe state).\n", customer_id);
    for (int i = 0; i < RESOURCES_TYPE_COUNT; i++) {
        available[i] += request[i];
        allocation[customer_id][i] -= request[i];
        need[customer_id][i] += request[i];
    }
    return false;
}

void* customer_routine(void* arg) {
    int customer_id = *(int*)arg;
    while (true) {
        usleep(100000);
        pthread_mutex_lock(&request_mutex);
        int request[RESOURCES_TYPE_COUNT];
        for (int i = 0; i < RESOURCES_TYPE_COUNT; i++)
            request[i] = rand() % (need[customer_id][i] + 1);
        request_resources(customer_id, request);
        bool finished = true;
        for (int i = 0; i < RESOURCES_TYPE_COUNT; i++) {
            if (need[customer_id][i] > 0) {
                finished = false;
                break;
            }
        }
        if (finished) {
            printf("Customer %d has finished its work.\n", customer_id);
            for (int i = 0; i < RESOURCES_TYPE_COUNT; i++) {
                available[i] += allocation[customer_id][i];
                allocation[customer_id][i] = 0;
                need[customer_id][i] = maximum[customer_id][i];
            }
            printf("Customer %d released its resources.\n", customer_id);
            pthread_mutex_unlock(&request_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&request_mutex);
    }
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != RESOURCES_TYPE_COUNT + 1) {
        fprintf(stderr, "Usage: %s <R0> <R1> ... <R%d>\n", argv[0],
                RESOURCES_TYPE_COUNT - 1);
        return 1;
    }
    srand(time(NULL));
    for (int i = 0; i < RESOURCES_TYPE_COUNT; i++) {
        available[i] = atoi(argv[i + 1]);
        if (available[i] < 0) {
            fprintf(stderr, "Invalid resource count: %d\n", available[i]);
            return 1;
        }
    }
    for (int i = 0; i < CUSTOMERS_COUNT; i++) {
        printf("Customer %d: ", i);
        for (int j = 0; j < RESOURCES_TYPE_COUNT; j++) {
            maximum[i][j] = rand() % (available[j] + 1);
            allocation[i][j] = 0;
            need[i][j] = maximum[i][j];
            printf("%d ", maximum[i][j]);
        }
        printf("\n");
    }
    pthread_t customers[CUSTOMERS_COUNT];
    int customer_ids[CUSTOMERS_COUNT];
    for (int i = 0; i < CUSTOMERS_COUNT; i++) {
        customer_ids[i] = i;
        pthread_create(&customers[i], NULL, customer_routine,
                       (void*)&customer_ids[i]);
    }
    for (int i = 0; i < CUSTOMERS_COUNT; i++)
        pthread_join(customers[i], NULL);
    printf("All customers have finished their work.\n");
    return 0;
}