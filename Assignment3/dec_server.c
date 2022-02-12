/* Xiaoying Li
 * lixiaoyi@oregonstate.edu
 * CS344 Assignment 3: One-Time Pads */

 /*
  * This program is the decryption server, decrypts ciphertext it is given, using the passed-in
  * ciphertext and key. Thus, it returns plaintext again to dec_client.
  */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>


  // Global constant string of all valid characters,
  // make it easier to find the wanted character when doing decryption.
const char* keyPool = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";



/*
 * Error function used for reporting issues.
 */
void error(const char* msg)
{
	perror(msg);
	exit(1);
}



/*
 * Set up the address struct for the server socket.
 */
void setupAddressStruct(struct sockaddr_in* address, int portNumber)
{
	memset((char*)address, '\0', sizeof(*address)); // Clear out the address struct.
	address->sin_family = AF_INET;   // The address should be network capable.
	address->sin_port = htons(portNumber);  // Store the port number.
	address->sin_addr.s_addr = INADDR_ANY;  // Allow a client at any address to connect to this server.
}



/*
 * Using the pass in key to decrypt the pass in ciphertext, with One-Time Pad encryption technique,
 * and return the plaintext.
 */
char* decrypt(char ciphertext[], char key[])
{
	// Initialize the plaintext with ciphertext, make sure their lengths are same.
	char* plaintext = ciphertext;

	// Decrypt every character of ciphertext.
	int i = 0;
	while (ciphertext[i] != '\n') {
		// Using a character's ASCII value to convert characters in ciphertext to their numerical values.
		int ciphertextValue = ciphertext[i];
		// Set space character's numerical value to 91, which is one larger than 'Z'.
		if (ciphertextValue == 32) {
			ciphertextValue = 91;
		}

		// Do the same to characters in key.
		int keyValue = key[i];
		if (keyValue == 32) {
			keyValue = 91;
		}

		// Using the numerical values of the ith characters of ciphertext 
		// and key to produce the ith character of plaintext.
		int plaintextValue = ciphertextValue - keyValue;
		if (plaintextValue < 0) {
			plaintextValue = plaintextValue + 27;
		}
		plaintext[i] = keyPool[plaintextValue];

		i++;	// Continue to the next character.
	}

	plaintext[i] = '\0';	// OverWrite the original newline character with an endline character.
	return plaintext;	// Return the plaintext.
}



int main(int argc, char* argv[])
{
	int connectionSocket, status;
	int charsRead = 0;
	int charsWritten = 0;
	struct sockaddr_in serverAddress, clientAddress;
	socklen_t sizeOfClientInfo;

	// Check usage & args.
	if (argc < 2) {
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);
	}

	// Create the socket that will listen for connections.
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket < 0) {
		error("ERROR opening socket");
	}

	// Set the socket options to allow the program to continue to use the same port.
	int on = 1;
	if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		error("ERROR setting socket option");
	}

	setupAddressStruct(&serverAddress, atoi(argv[1]));	// Set up the address struct for the server socket.

	// Associate the socket to the port.
	if (bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		error("ERROR on binding");
	}

	listen(listenSocket, 5);	// Start listening for connetions. Allow up to 5 connections to queue up.

	// Accept a connection, blocking if one is not available until one connects.
	while (1) {
		sizeOfClientInfo = sizeof(clientAddress);
		// Accept the connection request which creates a connection socket.
		connectionSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &sizeOfClientInfo);
		if (connectionSocket < 0) {
			error("ERROR on accept");
		}

		// Create a child process with fork when a connection is made to support
		// up to five concurrent socket connections running at the same time.
		pid_t pid = fork();
		switch (pid) {
		case -1: {	// If fork failed, print an error message.
			error("ERROR on fork");
		}

		case 0: {	// Child process.
			char buffer[2048];
			char ciphertext[100000];
			char key[100000];
			memset(ciphertext, '\0', sizeof(ciphertext));	   // Initialize the ciphertext array.
			close(listenSocket);

			// Receive checking message from dec_client.
			memset(buffer, '\0', sizeof(buffer));	   // Clear out the buffer array.
			while (charsRead == 0) {
				charsRead = recv(connectionSocket, buffer, sizeof(buffer) - 1, 0);
			}

			// If the checking message is not the correct one, send 'no' to dec_client.
			if (strcmp(buffer, "dec_client") != 0) {
				charsWritten = send(connectionSocket, "no", 2, 0);
				exit(2);	// Terminate, and set the exit value to 2.
			}

			// Else, dec_server and dec_client are connected correctly.
			else {
				// Send 'yes' to dec_client.
				memset(buffer, '\0', sizeof(buffer));	// Clear out the buffer array.
				charsWritten = send(connectionSocket, "yes", 3, 0);

				// Receive the number of characters in the ciphertext file from dec_client.
				memset(buffer, '\0', sizeof(buffer));	// Clear out the buffer array.
				charsRead = 0;
				while (charsRead == 0) {
					charsRead = recv(connectionSocket, buffer, sizeof(buffer) - 1, 0);
				}
				int fileLength = atoi(buffer) + 1;	// Convert the length string to an integer.

				charsRead = 0;
				int tempFileLength = fileLength;

				// Receive the ciphertext from dec_client.
				// Using a while loop to make sure all characters in ciphertext are received.
				while (fileLength > 0) {
					memset(buffer, '\0', sizeof(buffer));	// Clear out the buffer array.
					// Set the size of read in characters.
					int readLength = (sizeof(buffer) - 1) > fileLength ? fileLength : (sizeof(buffer) - 1);
					int length = recv(connectionSocket, buffer, readLength, 0);
					if (length <= 0) {
						break;
					}
					charsRead += length;
					fileLength -= length;
					strcat(ciphertext, buffer);	// Add the read in characters to ciphertext array.
				}

				// Receive the number of characters in the key file from dec_client.
				charsRead = 0;
				long keyLength = 0;
				while (charsRead == 0) {
					charsRead = recv(connectionSocket, &keyLength, sizeof(keyLength), 0);
				}
				keyLength += 1;

				// Receive the key from dec_client.
				charsRead = 0;
				// Using a while loop to make sure all characters in key are received.
				while (keyLength > 0) {
					memset(buffer, '\0', sizeof(buffer));	// Clear out the buffer array.
					// Set the size of read in characters.
					int readlength = (sizeof(buffer) - 1) > keyLength ? keyLength : (sizeof(buffer) - 1);
					int length = recv(connectionSocket, buffer, readlength, 0);
					if (length <= 0) {
						break;
					}
					charsRead += length;
					keyLength -= length;
					strcat(key, buffer);	// Add the read in characters to key array.
				}

				memset(buffer, '\0', sizeof(buffer));	// Clear out the buffer array.
				char* plaintext = decrypt(ciphertext, key);	// Decrypt the ciphertext using the key.

				// Send plaintext to dec_client.
				// Using a while loop to make sure all characters in plaintext are sent.
				while (charsWritten < tempFileLength) {
					charsWritten += send(connectionSocket, plaintext, strlen(plaintext), 0);
				}

				// Close the connection socket for this client
				close(connectionSocket);
				exit(0);
			}
		}

		default: {	// Parent process.
			close(connectionSocket);
			pid_t ppid = waitpid(pid, &status, WNOHANG);
		}
		}
	}

	// Close the listening socket
	close(listenSocket);
	return 0;
}