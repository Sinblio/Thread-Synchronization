#include <stdio.h>
#include "encrypt-module.h"

/*
 * Benjamin Schroeder
 *
 * This is the main.c class in terms of functionality it has one role, Taking user inputs then using them
 * to initialize the encryption sequence.
 */

int main(int argc, char*argv[]) {
    //Checks for the right number of arguments
    if (argc == 3) {
        int input_buffer_size, output_buffer_size;

        //Prompts the user for both the input and output buffer sizes
        printf("Enter input buffer size: ");
        scanf ("%d",&input_buffer_size);

        printf("Enter output buffer size: ");
        scanf ("%d",&output_buffer_size);

        //initializes the encryption based on given arguments and the buffer sizes
        init(argv[1], argv[2], input_buffer_size, output_buffer_size);

        //Upon returning prints a finishing message than prints the final counts
        printf("End of file reached.\n");
        display_counts();
    } else {
        //Prints a response if the wrong number of arguments are provided
        printf("Wrong # of arguments supplied, expected 2 additional");
    }
    return 0;
}
