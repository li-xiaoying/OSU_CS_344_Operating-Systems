/* Xiaoying Li
 * lixiaoyi@oregonstate.edu
 * CS344 Assignment 3: One-Time Pads */

 /*
  * This program connects to dec_server, and asks it to decrypt ciphertext using a passed-in
  * ciphertext and key.
  */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>



  /*
   * Set up the address struct.
   */
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname)
{
	memset((char*)address, '\0', sizeof(*address)); // Clear out the address struct.
	address->sin_family = AF_INET;  // The address should be network capable.
	address->sin_port = htons(portNumber);  // Store the port number.

	// Get the DNS entry for this host name.
	struct hostent* hostInfo = gethostbyname(hostname);
	if (hostInfo == NULL) {
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(1);
	}

	// Copy the first IP address from the DNS entry to sin_addr.s_addr.
	memcpy((char*)&address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}



/*
 * Count the characters in the file, and make sure no bad characters in the file.
 */
int fileLength(char* filename)
{
	int character;
	int characterCounter = 0;
	FILE* file = fopen(filename, "r");	// Open the file for reading.
	character = fgetc(file);	// Start from the first character in the file.

	// Keep reading through the file until the end of the file.
	while (!(character == EOF || character == '\n')) {
		// A valid character can only be the 26 capital letters, and the space character.
		// If the file contains bad characters, terminate, send error text to stderr,
		// and set the exit value to 1.
		if (!isupper(character) && character != ' ') {
			fprintf(stderr, "enc_client ERROR: input contains bad characters\n");
			exit(1);
		}

		character = fgetc(file);	// Continue to the next character.
		characterCounter++;	// Increase the counter.
	}

	fclose(file);
	return characterCounter;	// Return the total counter of characters.
}



int main(int argc, char* argv[])
{
	int socketFD, portNumber;
	int charsWritten = 0;
	int charsToBeWritten = 0;
	int charsRead = 0;
	struct sockaddr_in serverAddress;
	char buffer[2048];
	char plaintext[100000];

	// Check usage & args.
	if (argc < 4) {
		fprintf(stderr, "USAGE: %s ciphertext key port\n", argv[0]);
		exit(1);
	}

	// Create a socket.
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD < 0) {
		fprintf(stderr, "CLIENT: ERROR opening socket\n");
		exit(1);
	}

	// Set up the server address struct.
	setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

	// Connect to server.
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		fprintf(stderr, "Error: could not contact enc_server on port %s\n", argv[3]);
		exit(2);
	}

	// Get the number of characters in the ciphertext file.
	long ciphertextLength = fileLength(argv[1]);
	// Get the number of characters in the key file.
	long keyLength = fileLength(argv[2]);

	// If the key file is shorter than the ciphertext, terminate, send error text to stderr,
	// and set the exit value to 1.
	if (ciphertextLength > keyLength) {
		fprintf(stderr, "ERROR: key '%s' is too short\n", argv[2]);
		exit(1);
	}


	// Send checking message to dec_server, 
	// make sure it does connect to the right server program.
	char* checkMessage = "dec_client";
	charsWritten = send(socketFD, checkMessage, strlen(checkMessage), 0);

	// Receive checking message back from dec_server.
	memset(buffer, '\0', sizeof(buffer));   // Clear out the buffer array.
	while (charsRead == 0) {
		charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
	}

	// If the return message is "no", then it doesn't connect to the correct server program.
	// Report the rejection to stderr with the attempted port, terminate itself, and set the
	// exit value to 2.
	if (strcmp(buffer, "no") == 0) {
		fprintf(stderr, "Error: could not contact dec_server on port %s\n", argv[3]);
		exit(2);
	}


	// Send the number of characters in the ciphertext file to dec_server.
	memset(buffer, '\0', sizeof(buffer));   // Clear out the buffer array.
	sprintf(buffer, "%ld", ciphertextLength);
	charsWritten = send(socketFD, buffer, strlen(buffer), 0);


	// Send the plaintext to dec_server.
	memset(buffer, '\0', sizeof(buffer));   // Clear out the buffer array.
	charsWritten = 0;
	int ciphertext = open(argv[1], 'r');	// Open the plaintext file as a file descriptor.

	// Using a while loop to make sure all characters in ciphertext are sent.
	while (charsWritten < ciphertextLength) {
		// Read characters from ciphertext file.
		charsToBeWritten = read(ciphertext, buffer, sizeof(buffer) - 1);
		if (charsToBeWritten < 0) {
			break;
		}
		// Send characters to dec_server.
		charsWritten += send(socketFD, buffer, charsToBeWritten, 0);
	}
	close(ciphertext);


	// Send the number of characters in the key file to dec_server.
	charsWritten = 0;
	charsWritten = send(socketFD, &keyLength, sizeof(keyLength), 0);


	// Send the key to dec_server.
	charsWritten = 0;
	charsToBeWritten = 0;
	int key = open(argv[2], 'r');	// Open the key file as a file descriptor.

	// Using a while loop to make sure all characters in key are sent.
	while (charsWritten < keyLength) {
		memset(buffer, '\0', sizeof(buffer));   // Clear out the buffer array.
		// Read characters from key file.
		charsToBeWritten = read(key, buffer, sizeof(buffer) - 1);
		// Send characters to dec_server.
		charsWritten += send(socketFD, buffer, charsToBeWritten, 0);
	}
	close(key);


	// Read plaintext from dec_server.
	charsRead = 0;
	// Using a while loop to make sure all characters of plaintext are read.
	while (charsRead < ciphertextLength) {
		memset(buffer, '\0', sizeof(buffer));   // Clear out the buffer array.
		charsRead += recv(socketFD, buffer, sizeof(buffer) - 1, 0);
		strcat(plaintext, buffer);
	}
	strcat(plaintext, "\n");
	printf("%s", plaintext);


	// Close the socket
	close(socketFD);
	return 0;
}