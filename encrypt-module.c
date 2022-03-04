#include "encrypt-module.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

/*
 * Benjamin Schroeder
 *
 * encrypt-module.c creates the ability to encrypt a file with a changing key. It is based off of the
 * encrypt-module.h base, and allows for additional tracking statistics during the printing process.
 */

// Given private variables
FILE *input_file;
FILE *output_file;
int input_counts[256];
int output_counts[256];
int input_total_count;
int output_total_count;
int key = 1;
int waiting_for_reset = 0;

// Additional status private variables
int input_empty = 1;
int output_empty = 1;
int reader_done = 0;
int input_counter_done = 0;
int encryptor_done = 0;
int output_counter_done = 0;

// Variables for the input and output buffers
node_list* input_buffer;
node_list* output_buffer;
pthread_mutex_t* input_mutex;
pthread_mutex_t* output_mutex;

// Private variables for used semaphores
sem_t* reset_key;
sem_t* done;
sem_t* added_input;
sem_t* counted_input;
sem_t* room_input;
sem_t* added_output;
sem_t* counted_output;
sem_t* room_output;

/* acquire_input_buffer
 *
 * the locking section for the input_buffer mutex lock
 */
void acquire_input_buffer(){
    pthread_mutex_lock(input_mutex);
}

/* acquire_output_buffer
 *
 * the locking section for the output_buffer mutex lock
 */
void acquire_output_buffer(){
    pthread_mutex_lock(output_mutex);
}

/* release_input_buffer
 *
 * the releasing ability of the mutex lock for the input buffer
 */
void release_input_buffer() {
    pthread_mutex_unlock(input_mutex);
}

/* release_output_buffer
 *
 * the releasing ability of the mutex lock for the output buffer
 */
void release_output_buffer() {
    pthread_mutex_unlock(output_mutex);
}

void clear_counts() {
    memset(input_counts, 0, sizeof(input_counts));
    memset(output_counts, 0, sizeof(output_counts));
    input_total_count = 0;
    output_total_count = 0;
}

/* random_reset
 *
 * given function that will act as the base of a thread, added functionality to exit loop when done
 */
void * random_reset() {
	while (reader_done != 1) {
		sleep(rand() % 11 + 5);
		if (reader_done != 1) {
            reset_requested();
            key = rand() % 26;
            clear_counts();
            reset_finished();
        }
	}
}

/* init
 *
 * char * inputFileName - the file name used for the file that is read from
 * char * outputFileName - the file name that the encrypted file will be outputted to
 * int inputBufferSize - the maximum number of nodes the input buffer is allowed to have
 * int outputBufferSize - the maximum number of nodes the output buffer is allowed to have
 *
 * this acts as the main thread of the encryption, it setup then waits for the encryption to finish
 */
void init(char *inputFileName, char *outputFileName, int inputBufferSize, int outputBufferSize) {

    srand(time(0));

    //files opened
	input_file = fopen(inputFileName, "r");
	output_file = fopen(outputFileName, "w");

	//buffer is set up for use
    input_buffer = malloc(sizeof(node_list));
    output_buffer = malloc(sizeof(node_list));

    input_buffer->current = 0;
    output_buffer->current = 0;

    input_buffer->counted = 0;
    output_buffer->counted = 0;

    input_buffer->max = inputBufferSize;
    output_buffer->max = outputBufferSize;

    //semaphores and mutex are allocated space then initialized
    reset_key = malloc(sizeof(sem_t));
    done = malloc(sizeof(sem_t));
    added_input = malloc(sizeof(sem_t));
    counted_input = malloc(sizeof(sem_t));
    room_input = malloc(sizeof(sem_t));
    added_output = malloc(sizeof(sem_t));
    counted_output = malloc(sizeof(sem_t));
    room_output = malloc(sizeof(sem_t));

    sem_init(reset_key, 1, 0);
    sem_init(done, 1, 0);
    sem_init(room_input, 1, 0);
    sem_init(added_input, 1, 0);
    sem_init(counted_input, 1, 0);
    sem_init(added_output, 1, 0);
    sem_init(counted_output, 1, 0);
    sem_init(room_output, 1, 0);

    input_mutex = malloc(sizeof(pthread_mutex_t));
    output_mutex = malloc(sizeof(pthread_mutex_t));

    pthread_mutex_init(input_mutex, NULL);
    pthread_mutex_init(output_mutex, NULL);

    //process threads are declared then initialized
    pthread_t reader_thread;
    pthread_t reader_counter_thread;
    pthread_t encryption_thread;
    pthread_t writer_counter_thread;
    pthread_t writer_thread;
    pthread_t random_reset_thread;

    pthread_create(&reader_thread, NULL, reader, NULL);
    pthread_create(&reader_counter_thread, NULL, input_counter, NULL);
    pthread_create(&encryption_thread, NULL, encryptor, NULL);
    pthread_create(&writer_counter_thread, NULL, output_counter, NULL);
    pthread_create(&writer_thread, NULL, writer, NULL);
    pthread_create(&random_reset_thread, NULL, random_reset, NULL);

    //waits for the encryption process to complete before cleaning up
    sem_wait(done);

    //closes files
    fclose(input_file);
    fclose(output_file);

    //destroys and frees semaphores and mutex
    sem_destroy(reset_key);
    sem_destroy(done);
    sem_destroy(added_input);
    sem_destroy(counted_input);
    sem_destroy(added_output);
    sem_destroy(counted_output);

    free(reset_key);
    free(done);
    free(added_input);
    free(counted_input);
    free(added_output);
    free(counted_output);

    pthread_mutex_destroy(input_mutex);
    pthread_mutex_destroy(output_mutex);

    free(input_mutex);
    free(output_mutex);

    //frees buffer space
    free(output_buffer);
    free(input_buffer);
}

int read_input() {
    usleep(10000);
	return fgetc(input_file);
}

void write_output(int c) {
	fputc(c, output_file);
}

int caesar_encrypt(int c) {
	if (c >= 'a' && c <= 'z') {
		c += key;
		if (c > 'z') {
			c = c - 'z' + 'a' - 1;
		}
	} else if (c >= 'A' && c <= 'Z') {
		c += key;
		if (c > 'Z') {
			c = c - 'Z' + 'A' - 1;
		}
	}
	return c;
}

void count_input(int c) {
	input_counts[toupper(c)]++;
	input_total_count++;
}

void count_output(int c) {
	output_counts[toupper(c)]++;
	output_total_count++;
}

int get_input_count(int c) {
	return input_counts[toupper(c)];
}

int get_output_count(int c) {
	return output_counts[toupper(c)];
}

int get_input_total_count() {
	return input_total_count;
}

int get_output_total_count() {
	return output_total_count;
}

/* display counts
 *
 * prints the counts of each letter to the console
 */
void display_counts() {
    printf("Total input count with current key is %d.\n",get_input_total_count());
    for (int i = 65; i <= 90; i++) {
        printf("%c:%d ", i, get_input_count(i));
    }
    printf("\nTotal output count with current key is %d.\n",get_output_total_count());
    for (int i = 65; i <= 90; i++) {
        printf("%c:%d ", i, get_output_count(i));
    }
    printf("\n\n");
}

/* reset requested
 *
 * signals that the key wants to be reset then waits for active buffers to finish processing
 */
void reset_requested() {
    waiting_for_reset = 1;

    usleep(10000);

    printf("Reset requested.\n");

    while(input_empty || output_empty) {
        usleep(10000);
    }

    display_counts();
}

/* reset finished
 *
 * resumes processing following the completion of a key reset
 */
void reset_finished() {
    waiting_for_reset = 0;
    sem_post(reset_key);
}

/* reader
 *
 * the reader function for the reader thread
 */
void * reader() {

    int c;
    // room is set to override the semaphore initially since the buffer is empty
    int room = input_buffer->max;

    while(!feof(input_file))
    {
        c = read_input();

        //acquires mutex
        acquire_input_buffer();

        //until the buffer is thought to be full the semaphore is ignored
        if (room == 0) {
            sem_wait(room_input);
        } else {
            room--;
        }

        //creates a new node and adds it to the input buffer
        node* to_add = malloc(sizeof(node));
        to_add->c = c;

        //signals there is something in the buffer
        input_empty = 1;

        //adds node to the end of the list
        if (input_buffer->current == 0) {
            input_buffer->head = to_add;
        } else {
            node* next = input_buffer->head;

            while (next->next != NULL) {
                next = next->next;
            }

            next->next = to_add;
        }

        input_buffer->current++;

        //signals input counter that something is ready
        sem_post(added_input);

        //releases the mutex
        release_input_buffer();

        //checks to see if it should halt for the key to reset
        if (waiting_for_reset == 1) {
            sem_wait(reset_key);
        }
    }
    reader_done = 1;
}

/* input counter
 *
 * function used for the input counter thread
 */
void * input_counter() {

    int count = 0;

    while(reader_done != 1 || count != 0) {
        //waits for a signal from the reader thread
        sem_wait(added_input);

        // acquires the mutex for the input buffer
        acquire_input_buffer();

        //finds the next node to be counted
        node* next = input_buffer->head;

        for(int i = 0; i < input_buffer->counted; i++) {
            next = next->next;
        }

        //counts node
        count_input(next->c);

        // updates tracker
        input_buffer->counted++;

        //releases mutex
        release_input_buffer();

        //tells the encryptor its ready
        sem_post(counted_input);

        // checks if anything else is queued (used for while logic)
        sem_getvalue(added_input, &count);
    }
    input_counter_done = 1;
}

/* encryptor
 *
 * function used for the encryptor thread
 */
void * encryptor() {

    int count = 0;
    //similar room functionality as in the reader class
    int room = output_buffer->max;

    while(input_counter_done != 1 || count != 0) {
        // waits for input counter
        sem_wait(counted_input);

        if (room == 0) {
            sem_wait(room_output);
        } else {
            room--;
        }

        //needs both buffers to work
        acquire_output_buffer();
        acquire_input_buffer();

        //removed the head from the input buffer
        node* to_encrypt = input_buffer->head;

        input_buffer->head = input_buffer->head->next;
        input_buffer->current--;
        input_buffer->counted--;
        to_encrypt->next = NULL;

        //encrypts the value of c
        to_encrypt->c = caesar_encrypt(to_encrypt->c);

        // adds the node to the back of output
        if (output_buffer->current == 0) {
            output_buffer->head = to_encrypt;
        } else {
            node* next = output_buffer->head;

            while (next->next != NULL) {
                next = next->next;
            }

            next->next = to_encrypt;
        }

        output_buffer->current++;

        //updates buffer signals
        output_empty = 1;
        if (input_buffer->current == 0) {
            input_empty = 0;
        }

        //releases buffers
        release_output_buffer();
        release_input_buffer();

        //tells output counter there is a new output
        sem_post(added_output);
        //tells the reader that there is new space in the input buffer
        sem_post(room_input);

        sem_getvalue(counted_input, &count);
    }

    encryptor_done = 1;
}

/* output counter
 *
 * function used for the output counter thread
 */
void * output_counter(){
    int count = 0;

    while(encryptor_done != 1 || count != 0) {
        // waits for a signal from the encryptor
        sem_wait(added_output);

        // gets the output buffer
        acquire_output_buffer();

        // finds the node to count
        node* next = output_buffer->head;

        for(int i = 0; i < output_buffer->counted; i++) {
            next = next->next;
        }

        // indexes the char
        count_output(next->c);

        output_buffer->counted++;

        //releases the lock oon the output buffer
        release_output_buffer();

        // tells the writer a char is ready
        sem_post(counted_output);

        sem_getvalue(added_output, &count);
    }

    output_counter_done = 1;
}

/* writer
 *
 * function used for the writer thread
 */
void * writer() {
    int count = 0;

    while (output_counter_done != 1 || count != 0) {
        // waits for output counter
        sem_wait(counted_output);

        // checks out the output buffer
        acquire_output_buffer();

        //removes the head off the output list
        node* to_remove = output_buffer->head;

        output_buffer->head = output_buffer->head->next;
        output_buffer->current--;
        output_buffer->counted--;

        //writes output
        write_output(to_remove->c);

        //frees memory
        free(to_remove);

        //checks if status variable needs to be updated
        if (output_buffer->current == 0) {
            output_empty = 0;
        }

        //releases the buffer
        release_output_buffer();

        //tells the encryptor that there is room in the output buffer
        sem_post(room_output);

        sem_getvalue(counted_output, &count);
    }

    // signals the main thread that processing is complete
    sem_post(done);
}