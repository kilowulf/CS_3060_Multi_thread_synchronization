/* Module 6: Program 5 Synchronization
 * CS 3060-X01 Spring
 * Aaron Brown
 * 03/20/23
 *
 * Program takes values from stdin, performs factorization on those values and displays them to stdout
 * - 3 threads:
 *      - main thread creates producer and consumer thread. sends passed values to producer via buffer
 *      - producer thread takes values from buffer and performs factorization. values and factors saved to second buffer
 *      - consumer thread takes values from prod-cons buffer to display back to terminal
 *
 * - 2 buffers:
 *      - main-prod buffer holds initial values passed from stdin
 *          - int *array_variables: Factorization_Buffer
 *      - prod-cons buffer holds factors of passed values
 *          - Factors *array_factors: Factorization_Buffer
 *
 * */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint-gcc.h>


#define MAXSIZE_VARS_BUFFER 10
#define MAXSIZE_FACTORS_BUFFER 10

typedef struct Factors {
    int64_t base_number;
    int64_t prime_factors[MAXSIZE_FACTORS_BUFFER];
    int sizeof_array;
} Factors;

typedef struct Factorization_Buffer {
    pthread_mutex_t mutex_vars;
    pthread_mutex_t mutex_factors;


    pthread_cond_t is_empty_vars_buffer;
    pthread_cond_t is_full_vars_buffer;

    pthread_cond_t is_empty_factors_buffer;
    pthread_cond_t is_full_factors_buffer;

    int counter_in_factors;
    int next_in_factors;
    int next_out_factors;

    int counter_in_vars;
    int next_in_vars;
    int next_out_vars;

    int next_in_args;
    int sizeof_input;

    int *array_variables;
    Factors *array_factors;
} Factorization_Buffer;

// Initialize buffers
void factorization_buffer_init(Factorization_Buffer *buffer, int argc) {
    // allocate memory for vars and factors buffer
    buffer->array_variables = calloc(1, sizeof(int64_t) * MAXSIZE_VARS_BUFFER);
    buffer->array_factors = calloc(1, sizeof(Factors) * MAXSIZE_FACTORS_BUFFER);

    // initialize mutexes and condition variables
    pthread_mutex_init(&buffer->mutex_vars, NULL);
    pthread_mutex_init(&buffer->mutex_factors, NULL);

    pthread_cond_init(&buffer->is_empty_vars_buffer, NULL);
    pthread_cond_init(&buffer->is_empty_factors_buffer, NULL);

    pthread_cond_init(&buffer->is_full_vars_buffer, NULL);
    pthread_cond_init(&buffer->is_full_factors_buffer, NULL);

    // initialize indices and counters for vars and factors buffers
    buffer->next_in_vars = 0;
    buffer->next_out_vars = 0;

    buffer->next_out_factors = 0;
    buffer->next_in_factors = 0;

    buffer->next_in_args = 1;
    buffer->counter_in_vars = 0;
    buffer->counter_in_factors = 0;

    // store the size of the input in buffer struct
    buffer->sizeof_input = argc;
}

// Cleanup, de-allocate memory used by buffers
void factors_buffer_cleanup(Factorization_Buffer *buffer) {
    free(buffer->array_variables);
    free(buffer->array_factors);
}

// Take values from array_factors to print to terminal
void *consumer_thread(void *ptr)
{
    // type cast input param as Factorization_Buffer struct
    Factorization_Buffer *buffer = (Factorization_Buffer *)ptr;

    // loop and push all variables out of buffer till empty
    while(buffer->counter_in_factors > 0 || buffer->counter_in_vars > 0)
    {
        // get lock for factors buffer
        pthread_mutex_lock(&buffer->mutex_factors);

        // if no factors in buffer, wait for producer to add more values
        if(buffer->counter_in_factors == 0)
            pthread_cond_wait(&buffer->is_full_factors_buffer, &buffer->mutex_factors);

        // print factors and vars
        printf("%lld: ", (long long int)buffer->array_factors[buffer->next_out_factors].base_number);
        for(int array_index = 0; array_index < buffer->array_factors[buffer->next_out_factors].sizeof_array; array_index++)
        {
            printf("%lld ", (long long int)buffer->array_factors[buffer->next_out_factors].prime_factors[array_index]);
        }
        printf("\n");

        // move to next factor in factor buffer
        buffer->next_out_factors = (buffer->next_out_factors + 1) % MAXSIZE_FACTORS_BUFFER;
        buffer->counter_in_factors--;

        // signal buffer is no empty
        pthread_cond_signal(&buffer->is_empty_factors_buffer);

        // release mutex lock on factors buffer
        pthread_mutex_unlock(&buffer->mutex_factors);
    }

    return 0;
}

// Perform the factorization on passed values
int factorize_operation(void *number_of_factors)
{
    // check if input pointer is not null
    if(number_of_factors)
    {
        // type cast input pointer to Factors struct
        Factors * ptr = (struct Factors *)
                number_of_factors;
        int64_t factor = 2;
        int64_t number = ptr->base_number;

        // get factors of values
        while(number > 1)
        {
            if(number % factor == 0)
            {
                // add factors to prime factors list
                (*ptr).prime_factors[(*ptr).sizeof_array] = factor;
                (*ptr).sizeof_array++;

                number /= factor;
            }
            else
                // increment if factor doesnt divide
                factor++;
        }
        return 0;
    }
    // return a default value if the input pointer is null
    return -1;
}

// taking values from array_variables buffer, factorizing, pushing results to array_factors buffer
void *producer_thread(void *ptr)
{
    // type cast input parameters to Factorization_Buffer struct
    Factorization_Buffer *buffer = (Factorization_Buffer *)ptr;


    // process loop for all variables
    while(buffer->counter_in_vars > 0 || buffer->next_in_args < buffer->sizeof_input) {

        pthread_mutex_lock(&buffer->mutex_vars);

        if(buffer->counter_in_vars == 0)
            pthread_cond_wait(&buffer->is_full_vars_buffer, &buffer->mutex_vars);

        // get lock on factors buffer
        pthread_mutex_lock(&buffer->mutex_factors);

        // check if factors buffer is full; if so, wait for consumer thread to use
        if(buffer->counter_in_factors == MAXSIZE_FACTORS_BUFFER - 1)
            pthread_cond_wait(&buffer->is_empty_factors_buffer, &buffer->mutex_factors);

        // perform factorization on values in vars buffer
        buffer->array_factors[buffer->next_in_factors].base_number = buffer->array_variables[buffer->next_out_vars];
        buffer->array_factors[buffer->next_in_factors].sizeof_array = 0;

        factorize_operation(&buffer->array_factors[buffer->next_in_factors]);
        buffer->next_in_factors = (buffer->next_in_factors + 1) % MAXSIZE_FACTORS_BUFFER;
        buffer->counter_in_factors++;

        // signal buffer is not empty
        pthread_cond_signal(&buffer->is_full_factors_buffer);

        // release lock on factors buffer
        pthread_mutex_unlock(&buffer->mutex_factors);

        // move to next variable in buffer
        buffer->next_out_vars = (buffer->next_out_vars + 1) % MAXSIZE_VARS_BUFFER;
        buffer->counter_in_vars--;

        // signal vars buffer is no longer full
        pthread_cond_signal(&buffer->is_empty_vars_buffer);

        // release lock on vars buffer
        pthread_mutex_unlock(&buffer->mutex_vars);
    }
    return 0;
}




int main(int argc, char *argv[]) {

    // check if no arguments are passed
    if (argc <= 1) {
        printf("Usage:./p4 <number to factor>...\n");
        return 1;
    }

    // Declare and initialize buffers and threads
    Factorization_Buffer buffer;
    factorization_buffer_init(&buffer, argc);

    pthread_t producer_child, consumer_child;

    // create async threads using shared buffers
    pthread_create(&producer_child, NULL, producer_thread, &buffer);
    pthread_create(&consumer_child, NULL, consumer_thread, &buffer);

    // Main thread reads in the arguments to the vars buffer
    while (buffer.next_in_args < argc) {
        // secures thread
        pthread_mutex_lock(&buffer.mutex_vars);

        if (buffer.counter_in_vars == MAXSIZE_VARS_BUFFER) {
            pthread_cond_wait(&buffer.is_empty_vars_buffer, &buffer.mutex_vars);
        } else {
            buffer.array_variables[buffer.next_in_vars] = strtoll(argv[buffer.next_in_args], NULL, 10);
            buffer.next_in_vars = (buffer.next_in_vars + 1) % MAXSIZE_VARS_BUFFER;
            buffer.counter_in_vars++;
            buffer.next_in_args++;
        }

        // checks conditional variable : if vars buffer is full
        pthread_cond_signal(&buffer.is_full_vars_buffer);
        // unlock thread: free resources
        pthread_mutex_unlock(&buffer.mutex_vars);
    }

    // wait for child threads to complete or block thread
    pthread_join(consumer_child, NULL);
    pthread_join(producer_child, NULL);

    // deallocate memory
    factors_buffer_cleanup(&buffer);

    return 0;
}