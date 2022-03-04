#ifndef ENCRYPT_H
#define ENCRYPT_H

/*
 * Benjamin Schroeder
 *
 * the header class implemented in encrypt-module.c lays out needed functions, along with creating
 * datastructures to store a buffer without a pre determined length
 */

/*
 * data structure used as an item in the buffer
 */
typedef struct node {
    // c is the value
    int c;
    // next is the next node
    struct node* next;
} node;

/*
 * data structure holding general data for the overall list
 */
typedef struct node_list {
    // first entry node
    node* head;
    // maximum allowed list size
    int max;
    // current number of nodes in the list
    int current;
    // the starting from the front how many nodes have been counted
    int counted;
} node_list;

/*
 * When the function returns the encryption module is allowed to reset.
 */
void reset_requested();

/*
 * The function is called after the encryption module has finished a reset.
 */
void reset_finished();

/*
 * Prints the counts generated from both the input and output buffers
 */
void display_counts();

/*
 * Call to start encrypting doesn't return until complete
 */
void init(char *inputFileName, char *outputFileName, int inputBufferSize, int outputBufferSize);

/*
 * Given functions with no changes made
 */
int read_input();
void write_output(int c);
int caesar_encrypt(int c);
void count_input(int c);
void count_output(int c);
int get_input_count(int c);
int get_output_count(int c);
int get_input_total_count();
int get_output_total_count();

/*
 * Functions for each of the threads
 * random_reset - randomly resets the key
 * reader - reads a char and adds it to the input buffer
 * input_counter - indexes the chars being processed on the input side
 * encryptor - encrypts a char and moves the node from the input to output
 * output_counter - indexes the chars being written
 * writer - writes the char to the file and removes the node
 */
void * random_reset();
void * reader();
void * input_counter();
void * encryptor();
void * output_counter();
void * writer();

#endif // ENCRYPT_H
