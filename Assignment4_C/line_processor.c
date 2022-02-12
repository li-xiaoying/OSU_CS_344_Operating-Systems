/* Xiaoying Li
 * lixiaoyi@oregonstate.edu
 * CS344 Assignment 4: Multi-threaded Programming
 * C Program: Multi-threaded Producer Consumer Pipeline */



#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>



// Size of the buffer, which is an array of strings.
#define SIZE_PER_LINE 1000	// Size of every line of input.
#define LINE_SIZE 10	// Number of input lines.


// Buffer one, shared resource of input thread and line separator thread.
char bufferOne[LINE_SIZE][SIZE_PER_LINE];
// Number of items in buffer one, 
// shared resource of input thread and line separator thread.
int bufferOneCounter = 0;
// Index where the input thread as producer will put the next item.
int inputProducerIndex = 0;
// Index where the line separator thread as consumer will pick up the next item.
int lineSeparatorConsumerIndex = 0;
// Initialize the mutex one.
pthread_mutex_t mutexOne = PTHREAD_MUTEX_INITIALIZER;


// Buffer, shared resource of line separator thread and plus sign thread.
char bufferTwo[LINE_SIZE][SIZE_PER_LINE];
// Number of items in buffer two, 
// shared resource of line separator thread and plus sign thread.
int bufferTwoCounter = 0;
// Index where the line separator thread as producer will put the next item.
int lineSeperatorProducerIndex = 0;
// Index where the plus sign thread as consumer will pick up the next item.
int plusSignConsumerIndex = 0;
// Initialize the mutex two.
pthread_mutex_t mutexTwo = PTHREAD_MUTEX_INITIALIZER;


// Buffer, shared resource of plus sign thread and output thread.
char bufferThree[LINE_SIZE][SIZE_PER_LINE];
// Number of items in buffer three,
// shared resource of plus sign thread and output thread.
int bufferThreeCounter = 0;
// Index where the plus sign thread as producer will put the next item.
int plusSignProducerIndex = 0;
// Index where the output thread as consumer will pick up the next item.
int outputConsumerIndex = 0;
// Initialize the mutex three.
pthread_mutex_t mutexThree = PTHREAD_MUTEX_INITIALIZER;


// Array to hold all characters waiting to be output.
char outputBuffer[SIZE_PER_LINE * LINE_SIZE];
int outputCounter = 0;	// Initialize the number of characters already output.


// Initialize the condition variables.
pthread_cond_t fullOne = PTHREAD_COND_INITIALIZER;
pthread_cond_t fullTwo = PTHREAD_COND_INITIALIZER;
pthread_cond_t fullThree = PTHREAD_COND_INITIALIZER;
pthread_cond_t emptyOne = PTHREAD_COND_INITIALIZER;
pthread_cond_t emptyTwo = PTHREAD_COND_INITIALIZER;
pthread_cond_t emptyThree = PTHREAD_COND_INITIALIZER;



/*
 * Read in a line of input, which is the processing to be done by input thread.
 * Return the read in line of input.
 */
char* input()
{
	// Array to hold an input line. An input line will never be longer than 1000
	// characters (including the line separator).
	char* inputLine = malloc(1000);

	// Get standard input.
	size_t length = 0;
	ssize_t nread;
	nread = getline(&inputLine, &length, stdin);
	if (nread == -1) {
		clearerr(stdin);
	}

	return inputLine;
}



/*
 * Function that the input thread as producer thread will run. This thread performs 
 * input on a line-by-line basis from standard input. And put the input line in
 * buffer one only if there is space in buffer one. If buffer one is full, then wait
 * until there is space in buffer one.
*/
void* inputThread(void* args)
{
	while (1) {
		char* currentLine = input();	// Get an line of input.
		// Lock mutex one before checking whether there is space in buffer one.
		pthread_mutex_lock(&mutexOne);

		while (bufferOneCounter == LINE_SIZE) {
			// Buffer one is full. Wait for the line separator thread as consumer to
			// signal that buffer one has space.
			pthread_cond_wait(&emptyOne, &mutexOne);
		}

		// Put the read in line in buffer one.
		strcpy(bufferOne[inputProducerIndex], currentLine);
		// Increment the index where the next item will be put. Roll over to the
		// start of buffer one if the item was placed in the last slot in buffer
		// one.
		inputProducerIndex = (inputProducerIndex + 1) % LINE_SIZE;
		bufferOneCounter++;	// Increment the number of items in buffer one.
		// Signal to the line separator thread as consumer that buffer one is no
		// longer empty.
		pthread_cond_signal(&fullOne);
		pthread_mutex_unlock(&mutexOne);	// Unlock mutex one.

		// Use an infinite loop to put items to buffer one until it receives an
		// input line that contains only the characters DONE (followed immediately
		// by the line separator), at which point the program should terminate.
		if (strcmp(currentLine, "DONE\n") == 0) {
			break;
		}
	}

	return NULL;
}



/*
 * Replace every line separator in the input by a space, which is the processing to
 * be done by line separator thread. Return the updated line of input.
*/
char* lineSeparator(char* inputLine)
{
	inputLine[strcspn(inputLine, "\n")] = ' ';
	return inputLine;
}



/*
 * Function that the line separator thread as both producer thread and consumer 
 * thread will run.
 * As consumer, this thread get an item from buffer one if buffer one is not empty. 
 * If buffer one is empty then wait until there is data in buffer one.
 * As profucer, this thread replaces line separators with blanks. And put the 
 * updated line in buffer two only if there is space in buffer two. If buffer two is
 * full, then wait until there is space in buffer two.
*/
void* lineSeparatorThread(void* args)
{
	while (1) {
		// Lock mutex one before checking if buffer one has data.      
		pthread_mutex_lock(&mutexOne);

		while (bufferOneCounter == 0) {
			// Buffer one is empty. Wait for the input thread as producer to signal
			// that buffer one has data.
			pthread_cond_wait(&fullOne, &mutexOne);
		}

		// Get a line of input from buffer one.
		char* currentLine = bufferOne[lineSeparatorConsumerIndex];
		// Replace line separator in the input by a space.
		currentLine = lineSeparator(currentLine);
		// Increment the index from which the item will be picked up, rolling over
		// to the start of buffer one if currently at the end of buffer one.
		lineSeparatorConsumerIndex = (lineSeparatorConsumerIndex + 1) % LINE_SIZE;
		bufferOneCounter--;	// Decrement the number of items in buffer one.
		// Signal to the input thread as producer that buffer one has space.
		pthread_cond_signal(&emptyOne);
		pthread_mutex_unlock(&mutexOne);	// Unlock the mutex one.


		// Lock the mutex two before checking whether there is space in buffer two.
		pthread_mutex_lock(&mutexTwo);

		while (bufferTwoCounter == LINE_SIZE) {
			// Buffer two is full. Wait for the plus sign thread as consumer to
			// signal that buffer two has space.
			pthread_cond_wait(&emptyTwo, &mutexTwo);
		}

		// Put the updated line in buffer two.
		strcpy(bufferTwo[lineSeperatorProducerIndex], currentLine);
		// Increment the index where the next item will be put. Roll over to the
		// start of buffer two if the item was placed in the last slot in buffer
		// two.
		lineSeperatorProducerIndex = (lineSeperatorProducerIndex + 1) % LINE_SIZE;
		bufferTwoCounter++;	// Increment the number of items in buffer two.
		// Signal to the plus sign thread as consumer that buffer two is no longer
		// empty.
		pthread_cond_signal(&fullTwo);
		pthread_mutex_unlock(&mutexTwo);	// Unlock mutex two.


		// Use an infinite loop to pick up items from buffer one and put items to
		// buffer two until it receives an input line that contains only the
		// characters DONE (followed immediately by the a space), at which point the
		// program should terminate.
		if (strcmp(currentLine, "DONE ") == 0) {
			break;
		}
	}

	return NULL;
}



/*
 * Replace every pair of plus signs by the caret symbol, which is the processing to
 * be done by plus sign thread. Return the updated line of input.
*/
char* plusSign(char* inputLine)
{
	// Get the number of characters in the input line.
	size_t inputLength = strlen(inputLine);

	int i;
	// Loop through every character in the input line to find every pair of plus
	// signs.
	for (i = 0; i < inputLength; i++) {
		char currentCharacter = inputLine[i];	// Get the current character.

		if (currentCharacter == '+') {	// If the current character is a plus,
			if (inputLine[i + 1] == '+') { // and the next character is also a plus,
				// remove the second plus sign,
				memmove(&inputLine[i], &inputLine[i + 1], inputLength - i);
				inputLine[i] = '^';	// and set the current plus sign as the caret.
			}
		}
	}

	return inputLine;
}



/*
 * Function that the plus sign thread as both producer thread and consumer thread
 * will run.
 * As consumer, this thread get an item from buffer two if buffer two is not empty.
 * If buffer two is empty then wait until there is data in buffer two.
 * As profucer, this thread replace every pair of plus signs by the caret symbol.
 * And put the updated line in buffer three only if there is space in buffer three.
 * If buffer three is full, then wait until there is space in buffer three.
*/
void* plusSignThread(void* args)
{ 
	while (1) {
		// Lock mutex two before checking if buffer two has data.      
		pthread_mutex_lock(&mutexTwo);

		while (bufferTwoCounter == 0) {
			// Buffer two is empty. Wait for the line separator thread as producer
			// to signal that buffer two has data.
			pthread_cond_wait(&fullTwo, &mutexTwo);
		}

		// Get a line of input from buffer two.
		char* currentLine = bufferTwo[plusSignConsumerIndex];
		// Replace every pair of plus signs by the caret symbol in the input line.
		currentLine = plusSign(currentLine);
		// Increment the index from which the item will be picked up, rolling over
		// to the start of buffer two if currently at the end of buffer two.
		plusSignConsumerIndex = (plusSignConsumerIndex + 1) % LINE_SIZE;
		bufferTwoCounter--;	// Decrement the number of items in buffer two.
		// Signal to line separator thread as producer that buffer two has space.
		pthread_cond_signal(&emptyTwo);
		pthread_mutex_unlock(&mutexTwo);	// Unlock mutex two.


		// Lock mutex three before checking whether there is space in buffer three.
		pthread_mutex_lock(&mutexThree);

		while (bufferThreeCounter == LINE_SIZE) {
			// Buffer three is full. Wait for the output thread as consumer to 
			// signal that buffer three has space.
			pthread_cond_wait(&emptyThree, &mutexThree);
		}

		// Put the updated line in buffer three.
		strcpy(bufferThree[plusSignProducerIndex], currentLine);
		// Increment the index where the next item will be put. Roll over to the
		// start of buffer two if the item was placed in the last slot in buffer
		// three.
		plusSignProducerIndex = (plusSignProducerIndex + 1) % LINE_SIZE;
		bufferThreeCounter++;	// Increment the number of items in buffer three.
		// Signal to output thread as consumer that buffer three is no longer empty.
		pthread_cond_signal(&fullThree);
		pthread_mutex_unlock(&mutexThree);	// Unlock mutex three.

		// Use an infinite loop to pick up items from buffer two and put items to
		// buffer three until it receives an input line that contains only the
		// characters DONE (followed immediately by the a space), at which point the
		// program should terminate.
		if (strcmp(currentLine, "DONE ") == 0) {
			break;
		}
	}

	return NULL;
}



/*
 * Function that the output thread as consumer thread will run. Get an item from 
 * buffer three if buffer three is not empty. If buffer three is empty then wait
 * until there is data in buffer three. This thread writes output lines to the
 * standard output.
*/
void* outputThread(void* args)
{  
	while (1) {
		// Lock mutex three before checking if buffer three has data.     
		pthread_mutex_lock(&mutexThree);

		while (bufferThreeCounter == 0) {
			// Buffer three is empty. Wait for the plus sign thread as producer to
			// signal that buffer three has data.
			pthread_cond_wait(&fullThree, &mutexThree);
		}

		// Get a line of input from buffer three.
		char* currentLine = bufferThree[outputConsumerIndex];

		// Use an infinite loop to pick up items from buffer three until it receives
		// an input line that contains only the characters DONE (followed 
		// immediately by the a space), at which point the program should terminate.
		if (strcmp(currentLine, "DONE ") == 0) {
			break;
		}

		// Increment the index from which the item will be picked up, rolling over
		// to the start of buffer three if currently at the end of buffer three.
		outputConsumerIndex = (outputConsumerIndex + 1) % LINE_SIZE;
		bufferThreeCounter--;	// Decrement the number of items in buffer three.
		// Signal to the plus sign thread as producer that buffer three has space.
		pthread_cond_signal(&emptyThree);	
		pthread_mutex_unlock(&mutexThree);	// Unlock mutex three.

		// Copy the read in line into the array holding all characters waiting to be
		// output.
		strcat(outputBuffer, currentLine);

		// As long as there are sufficient data for an output line (no less than 80
		// characters) in the output array, produce output lines.
		while ((strlen(outputBuffer) - outputCounter) >= 80) {
			int i;

			// Output every 80 non line separator characters plus a line separator.
			for (i = 0; i < 80; i++) {
				printf("%c", outputBuffer[outputCounter + i]);
			}
			printf("\n");

			// Update the the number of characters already output.
			outputCounter = outputCounter + 80;
		}
	}

	return NULL;
}



int main(int argc, char* argv[])
{
	pthread_t input_thread, line_separator_thread, plus_sign_thread, output_thread;

	// Create the four threads.
	pthread_create(&input_thread, NULL, inputThread, NULL);
	pthread_create(&line_separator_thread, NULL, lineSeparatorThread, NULL);
	pthread_create(&plus_sign_thread, NULL, plusSignThread, NULL);
	pthread_create(&output_thread, NULL, outputThread, NULL);

	// Compile and link the four threads.
	pthread_join(input_thread, NULL);
	pthread_join(line_separator_thread, NULL);
	pthread_join(plus_sign_thread, NULL);
	pthread_join(output_thread, NULL);

	return 0;
}