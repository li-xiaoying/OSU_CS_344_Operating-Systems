/* Xiaoying Li
 * lixiaoyi@oregonstate.edu
 * CS344 Assignment 3: One-Time Pads */

/*
 * This program creates a key file of specified length. The
 * characters in the file generated will be any of the 27
 * allowed characters (26 capital letters, and the space
 * character), generated using the standard Unix
 * randomization methods. The last character this program
 * outputs will be a newline. Any error text will be output
 * to stderr.
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int main(int argc, char* argv[])
{
	// Check usage & args.
	if (argc < 2) {
		fprintf(stderr, "USAGE: %s keylength\n", argv[0]);
        exit(1);
    }

	// Convert the length string to an integer.
	int keyLength = atoi(argv[1]);
	srand(time(NULL));

	int i;
	char randomKey;
	int randomValue;

	// Fill the key array with random capital letters and 
	// the space character.
	for (i = 0; i < keyLength; i++) {
		// Generate a random number between 0-26.
		randomValue = rand() % 27;

		// If the random number is 26, set the character to
		// the space character.
		if (randomValue == 26) {
			randomKey = ' ';
		}

		// Else, set the character to the capital letter 
		// with ASCII value of random number plus 65. 
		else {
			randomKey = randomValue + 65;
		}

		printf("%c", randomKey);
	}

	// Add a newline character to the key.
	printf("\n");
	return 0;
}