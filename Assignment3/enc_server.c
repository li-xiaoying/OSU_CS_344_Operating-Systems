/* Xiaoying Li
 * lixiaoyi@oregonstate.edu
 * CS344 Assignment 3: One-Time Pads */

/*
 * This program is the encryption server and will run in the background as a daemon.
 * Its function is to perform the actual encoding.
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
// make it easier to find the wanted character when doing encryption.
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
 * Using the pass in key to encrypt the pass in plaintext, with One-Time Pad encryption technique,
 * and return the ciphertext. 
 */
char* encrypt(char plaintext[], char key[])
{
	// Initialize the ciphertext with plaintext, make sure their lengths are same.
	char* ciphertext = plaintext;

	// Encrypt every character of plaintext.
	int i = 0;
	while (plaintext[i] != '\n') {
		// Using a character's ASCII value to convert characters in plaintext to their numerical values.
		int plaintextValue = plaintext[i] - 'A';
		// Since the ASCII value of space character is less than A, set its numerical value to 26.
		if (plaintextValue < 0) {
			plaintextValue = 26;
		}

		// Do the same to characters in key.
		int keyValue = key[i] - 'A';
		if (keyValue < 0) {
			keyValue = 26;
		}

		// Using the numerical values of the ith characters of plaintext 
		// and key to produce the ith character of ciphertext.
		int ciphertextValue = (plaintextValue + keyValue) % 27;
		ciphertext[i] = keyPool[ciphertextValue];

		i++;	// Continue to the next character.
	}

	ciphertext[i] = '\0';	// OverWrite the original newline character with an endline character.
	return ciphertext;	// Return the ciphertext.
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
				char plaintext[100000];
				char key[100000];
				memset(plaintext, '\0', sizeof(plaintext));	   // Initialize the plaintext array.
				close(listenSocket);

				// Receive checking message from enc_client.
				memset(buffer, '\0', sizeof(buffer));	   // Clear out the buffer array.
				while (charsRead == 0) {
					charsRead = recv(connectionSocket, buffer, sizeof(buffer) - 1, 0);
				}

				// If the checking message is not the correct one, send 'no' to enc_client.
				if (strcmp(buffer, "enc_client") != 0) {
					charsWritten = send(connectionSocket, "no", 2, 0);
					exit(2);	// Terminate, and set the exit value to 2.
				}

				// Else, enc_server and enc_client are connected correctly.
				else {
					// Send 'yes' to enc_client.
					memset(buffer, '\0', sizeof(buffer));	// Clear out the buffer array.
					charsWritten = send(connectionSocket, "yes", 3, 0);

					// Receive the number of characters in the plaintext file from enc_client.
					memset(buffer, '\0', sizeof(buffer));	// Clear out the buffer array.
					charsRead = 0;
					while (charsRead == 0) {
						charsRead = recv(connectionSocket, buffer, sizeof(buffer) - 1, 0);
					}              
					int fileLength = atoi(buffer) + 1;	// Convert the length string to an integer.
					
					charsRead = 0;
					int tempFileLength = fileLength;
					
					// Receive the plaintext from enc_client.
					// Using a while loop to make sure all characters in plaintext are received.
					while (fileLength > 0) {
						memset(buffer, '\0', sizeof(buffer));	// Clear out the buffer array.
						// Set the size of read in characters.
						int readLength = (sizeof(buffer) - 1) > fileLength ? fileLength : (sizeof(buffer) - 1);
						int length = recv(connectionSocket, buffer, readLength, 0);
						if(length <= 0) {
							break;
						}
						charsRead += length;
						fileLength -= length;
						strcat(plaintext, buffer);	// Add the read in characters to plaintext array.
					}

					// Receive the number of characters in the key file from enc_client.
					charsRead = 0;
					long keyLength = 0;
					while (charsRead == 0) {
						charsRead = recv(connectionSocket, &keyLength, sizeof(keyLength), 0);
					}
					keyLength += 1;

					// Receive the key from enc_client.
					charsRead = 0;
					// Using a while loop to make sure all characters in key are received.
					while (keyLength > 0) {
						memset(buffer, '\0', sizeof(buffer));	// Clear out the buffer array.
						// Set the size of read in characters.
						int readlength = (sizeof(buffer) - 1) > keyLength ? keyLength : (sizeof(buffer) - 1);
						int length = recv(connectionSocket, buffer, readlength, 0);
						if(length <= 0) {
							break;
						}
						charsRead += length;
						keyLength -= length;							
						strcat(key, buffer);	// Add the read in characters to key array.
					}

					memset(buffer, '\0', sizeof(buffer));	// Clear out the buffer array.
					char* ciphertext = encrypt(plaintext, key);	// Encrypt the plaintext using the key.

					// Send ciphertext to enc_client.
					// Using a while loop to make sure all characters in ciphertext are sent.
					while (charsWritten < tempFileLength) {
						charsWritten += send(connectionSocket, ciphertext, strlen(ciphertext), 0);
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