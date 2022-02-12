/* Xiaoying Li
 * lixiaoyi@oregonstate.edu
 * Assignment 1: Adventure */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>


/* struct for room elements */
struct Room {
    char* name;
    char* type;
    char* outboundConnections[6];
    int connectionsNumber;
};


/* Adapted from https://repl.it/@cs344/34completeexamplec#main.c */
/* 
 * Go to the directory specified by path. In this direcory, look for all the directories whose name start with prefix,
 * return the name of the directory that has the latest modification time.
 */
char* findLatestDirectory(char* path, char* prefix)
{
    struct stat dirStat;
    char directoryName[256];
    char* latestDirName;

    // Open the directory
    DIR* currDir = opendir(path);
    struct dirent* aDir;
    time_t lastModifTime;
    int i = 0;

    // The data returned by readdir() may be overwritten by subsequent calls to readdir() for the same directory stream.
    // So we will copy the name of the directory to the variable directoryName.
    while ((aDir = readdir(currDir)) != NULL) {
        // Use strncmp to check if the directory name matches the prefix.
        if (strncmp(prefix, aDir->d_name, strlen(prefix)) == 0) {
            stat(aDir->d_name, &dirStat);

            // Check to see if this is the directory with the latest modification time.
            if (i == 0 || difftime(dirStat.st_mtime, lastModifTime) > 0) {
                lastModifTime = dirStat.st_mtime;
                memset(directoryName, '\0', sizeof(directoryName));
                strcpy(directoryName, aDir->d_name);
            }
            i++;
        }
    }

    latestDirName = malloc(sizeof(char) * (strlen(directoryName) + 1));
    strcpy(latestDirName, directoryName);

    closedir(currDir);
    return latestDirName;
}


/* Adapted from https://repl.it/@cs344/34completeexamplec#main.c */
/* 
 * Create and return a string for the file path by concatenating the directory name with the file name.
 */
char* getFilePath(char* directoryName, char* fileName)
{
    char* filePath = malloc(strlen(directoryName) + strlen(fileName) + 2);
    memset(filePath, '\0', strlen(directoryName) + strlen(fileName) + 2);
    strcpy(filePath, directoryName);
    strcat(filePath, "/");
    strcat(filePath, fileName);
    return filePath;
}


/* Adapted from https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input */
/*
 * Remove trailing newline character of a string from getline() input.
 */
void removeNewLine(char* string)
{
    size_t length = strlen(string);
    if ((length > 0) && (string[length - 1] == '\n'))
    {
        string[length - 1] = '\0';
    }
}


/*
 * Play the game with the provided rooms array, the current room's index, and the current step count.
 * Variables of current room's index and step count would be updated after every call of the function.
 * Return bool value which indicates if the user has reached the END ROOM.
 */
bool game(int* currentRoomIndex, struct Room* rooms, int* steps)
{
    // If the user has reached the End Room, indicate that it has been reached, print out a congratulatory message,
    // and set the return value to "true".
    if (strcmp(rooms[*currentRoomIndex].type, "END_ROOM") == 0) {
        printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
        return true;
    }

    // Else, the current room isn't END ROOM. Print out the current room's name and possible connections' names.
    int i, j, k;
    printf("CURRENT LOCATION: %s\nPOSSIBLE CONNECTIONS: ", rooms[*currentRoomIndex].name);
    for (i = 0; i < rooms[*currentRoomIndex].connectionsNumber; i++) {
        printf("%s", rooms[*currentRoomIndex].outboundConnections[i]);

        // Print out the right punctuation and prompt the user to enter the name of room to go.
        if (rooms[*currentRoomIndex].connectionsNumber - i == 1) {
            printf(".\nWHERE TO? >");
        }
        else {
            printf(", ");
        }
    }

    // Read in the user's input.
    char inputRoom[100];
    fgets(inputRoom, sizeof(inputRoom), stdin);
    printf("\n");

    // Check if the user's input is valid.
    bool validateInput = false;
    int nextRoomIndex = *currentRoomIndex;
    for (j = 0; j < rooms[*currentRoomIndex].connectionsNumber; j++) {
		int len = (strlen(rooms[*currentRoomIndex].outboundConnections[j]) > strlen(inputRoom)) ?
				  (strlen(inputRoom) - 1) : strlen(rooms[*currentRoomIndex].outboundConnections[j]);

        // If the user types in the exact name of a connection to another room, 
        // update the next room's index and the step count. 
        if (strncmp(rooms[*currentRoomIndex].outboundConnections[j], inputRoom, len) == 0) {
            validateInput = true;
            for (k = 0; k < 7; k++) {
                if (strncmp(rooms[k].name, inputRoom, len) == 0) {				
                    nextRoomIndex = k;
                }
            }
            *steps = *steps + 1;
            break;
        }
    }

    // If the user types anything but a valid room name from this location (case matters), print out an error line.
    if (!validateInput) {
        printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
    }

    // Set current room's index to next room's index. 
    // Set the return value to "false" to continue the game to the next room.
    // NOTE: Trying to go to an incorrect location does not increment the path history, 
    //       the step count and the next room's index.
    *currentRoomIndex = nextRoomIndex;
    return false;
}


int main()
{
    // Create rooms array to hold rooms data. Initialize every room elements.
    struct Room rooms[7];
    int i, j;
    for (i = 0; i < 7; i++) {
        rooms[i].name = "unknown";
        rooms[i].type = "unknown";
        rooms[i].connectionsNumber = 0;
        for (j = 0; j < 6; j++) {
            rooms[i].outboundConnections[j] = NULL;
        }
    }

    // Find the most recently created directory with prefix "lixiaoyi.rooms." in the same directory as this file,
    // and open it.
    char* latestDirectory = findLatestDirectory(".", "lixiaoyi.rooms.");
    DIR* currentDirectory = opendir(latestDirectory);

    // Prepare for going through all the entries in the directory to read rooms data back into program.
    struct dirent* entry;
    int roomsNumber = 0;
    FILE* currentFile = NULL;
    // Skip the first two "." and ".." entries.
    entry = readdir(currentDirectory);
    entry = readdir(currentDirectory);
    
    // Go through all the entries.
    while ((entry = readdir(currentDirectory)) != NULL) {
        char* currentFilePath = getFilePath(latestDirectory, entry->d_name);

        // Open the specified file for reading only. Check if the file is succeccfully open.
		if((currentFile = fopen(currentFilePath, "r")) == NULL)
		{
			free(currentFilePath);		
			perror("fopen");
			continue;
		}

        // Deallocate the memory previously allocated to avoid memory leak.
		free(currentFilePath); 
		
        char* currentLine = NULL;
        size_t length = 0;
        ssize_t nread;
        int possibleConnections = 0;

        // Read all the lines in this room file.
        while ((nread = getline(&currentLine, &length, currentFile)) != -1) {
            // Tokenize the read in line, and the delimiter is space.
            char* token = strtok(currentLine, " ");

            // If the first token is "ROOM", this line is data of this room's name or type.
            if (strcmp(token, "ROOM") == 0) {
                // Continue tokenizing the read in line, and the delimiter is still space.
                token = strtok(NULL, " ");

                // If the second token is "NAME", this line is data of this room's name.
                if (strcmp(token, "NAME:") == 0) {
                    // Continue tokenizing the read in line, and the delimiter is still space.
                    token = strtok(NULL, " ");
                    // The third token is this room's name, store it to rooms array.
                    rooms[roomsNumber].name = calloc(strlen(token) + 1, sizeof(char));
                    strcpy(rooms[roomsNumber].name, token);
                    // Remove trailing newline character from getline() input.
                    removeNewLine(rooms[roomsNumber].name);
                }

                // Else, this line is data of this room's type.
                else {
                    // Continue tokenizing the read in line, and the delimiter is still space.
                    token = strtok(NULL, " ");
                    // The third token is this room's type, store it to rooms array.
                    rooms[roomsNumber].type = calloc(strlen(token) + 1, sizeof(char));
                    strcpy(rooms[roomsNumber].type, token);
                    // Remove trailing newline character from getline() input.
                    removeNewLine(rooms[roomsNumber].type);
                }
            }

            // Else, this line is data of this room's outbound connection's name.
            else {
                // Continue tokenizing the read in line twice, and the delimiter is still space.
                token = strtok(NULL, " ");
                token = strtok(NULL, " ");
                // The third token is this room's outbound connection's name, store it to rooms array.
                rooms[roomsNumber].outboundConnections[possibleConnections] = calloc(strlen(token) + 1, sizeof(char));
                strcpy(rooms[roomsNumber].outboundConnections[possibleConnections], token);
                // Remove trailing newline character from getline() input.
                removeNewLine(rooms[roomsNumber].outboundConnections[possibleConnections]);
                // Increase the number of possible connections of this room, and store it to rooms array.
                possibleConnections++;
                rooms[roomsNumber].connectionsNumber = possibleConnections;
            }
        }
		
        // Deallocate the memory previously allocated to avoid memory leak.
		if(currentLine != NULL)
		{
			free(currentLine);
			currentLine = NULL;
		}
		
        // Increase rooms' number.
        roomsNumber++;
        // Close the current file and continue to the next file.
        fclose(currentFile);
    }
	
    // Deallocate the memory previously allocated to avoid memory leak.
	if(latestDirectory != NULL)
	{
		free(latestDirectory);
		latestDirectory = NULL;
	}

    // Close the directory.
    closedir(currentDirectory);

    // After read all the read rooms data back into program, locate the START ROOM to start the game.
    int k;
    int roomIndex;
    for (k = 0; k < 7; k++) {
        if (strcmp(rooms[k].type, "START_ROOM") == 0) {
            roomIndex = k;
        }
    }

    // Initialize the step count and path array to store the path history.
    int steps = 0;
    int path[50];
	memset(path, 0, sizeof(path));
	
    // Play game until the user win the game, which means the user reach the END ROOM.
    bool gameWin = false;
    while (!gameWin) {
        gameWin = game(&roomIndex, rooms, &steps);
        // Store path history after every successful step.
        // For unsuccessful steps, the value of step count won't change, so it just rewrite the last step again.
        path[steps - 1] = roomIndex;
    }

    // Print out the path the user has taken to get the END ROOM.
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
    int l;
    for (l = 0; l < steps; l++) {
        printf("%s\n", rooms[path[l]].name);
    }

    // Deallocate the memory previously allocated to avoid memory leak.
    int m, n;
	for (m = 0; m < 7; m++) {
		if(rooms[m].name != NULL)
		{
			if(strcmp(rooms[m].name, "unknown") != 0)
			{
				free(rooms[m].name);
				rooms[m].name = NULL;
			}
		}
		
		if(rooms[m].type != NULL)
		{			
			if(strcmp(rooms[m].type, "unknown") != 0)
			{
				free(rooms[m].type);
				rooms[m].type = NULL;
			}
		}

		for (n = 0; n < 6; n++) {
			if(rooms[m].outboundConnections[n] != NULL)
			{
				free(rooms[m].outboundConnections[n]);
				rooms[m].outboundConnections[n] = NULL;
			}
		}
	}

    // Set the exit status code to 0.
    return 0;
}