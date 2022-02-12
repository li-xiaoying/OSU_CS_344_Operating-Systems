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


 /* struct for room elements */
struct Room {
	char* name;
	char* type;
	struct Room* outboundConnections[6];
	int connectionsNumber;
};


/* array of ten different Room Names */
char* roomNames[] = { "RoomA", "RoomB", "RoomC", "RoomD", "RoomE", "RoomF", "RoomG", "RoomH", "RoomI", "RoomJ" };


/* Returns true if all rooms have 3 to 6 outbound connections, false otherwise. */
bool IsGraphFull(struct Room* rooms)
{
	int i;
	for (i = 0; i < 7; i++) {
		if (rooms[i].connectionsNumber < 3 || rooms[i].connectionsNumber > 6) {
			return false;
		}
	}

	return true;
}


/* Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise. */
bool CanAddConnectionFrom(struct Room* x)
{
	if (x->connectionsNumber < 6) {
		return true;
	}

	else {
		return false;
	}
}


/* Returns true if a connection from Room x to Room y already exists, false otherwise. */
bool ConnectionAlreadyExists(struct Room* x, struct Room* y)
{
	int i;
	for (i = 0; i < x->connectionsNumber; i++) {
		if (x->outboundConnections[i] == y) {
			return true;
		}
	}

	return false;
}


/* Returns true if Rooms x and y are the same Room, false otherwise. */
bool IsSameRoom(struct Room* x, struct Room* y)
{
	if (x == y) {
		return true;
	}

	else {
		return false;
	}
}


/* Connects Rooms x and y together, does not check if this connection is valid. */
void ConnectRoom(struct Room* x, struct Room* y)
{
	x->outboundConnections[x->connectionsNumber] = y;
	x->connectionsNumber++;
	y->outboundConnections[y->connectionsNumber] = x;
	y->connectionsNumber++;
}


/* Adds a random, valid outbound connection from a Room to another Room */
void AddRandomConnection(struct Room* rooms)
{
	struct Room* x;
	struct Room* y;

	do {
		x = &rooms[rand() % 7];	// Generate a random Room.
	} while (!CanAddConnectionFrom(x));

	do {
		y = &rooms[rand() % 7];	// Generate a random Room.
	} while (!CanAddConnectionFrom(y) || IsSameRoom(x, y) || ConnectionAlreadyExists(x, y));

	// Add this connection to the real variables, because this x and y will be destroyed when this function terminates.
	ConnectRoom(x, y);  
}


int main()
{
	srand(time(NULL));

	// Create rooms array to hold rooms data.
	struct Room rooms[7];

	// Generate a random number between 0 and 100,000.
	int randomNumber = rand() % 100000;
	// Generate the directory name using the random number: lixiaoyi.rooms.<random_number>.
	char directoryName[22];
	sprintf(directoryName, "lixiaoyi.rooms.%d", randomNumber);
	// Create the corresponding directory with specified permissions.
	mkdir(directoryName, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

	// Initialize the value of every room's elements.
	int randomIndices[7];
	// For the first room, randomly assign one of the ten names to its name.
	randomIndices[0] = rand() % 10;
	rooms[0].name = roomNames[randomIndices[0]];
	// Initialize its type as MID ROOM.
	rooms[0].type = "MID_ROOM";
	rooms[0].connectionsNumber = 0;
	int i;
	for (i = 0; i < 6; i++) {
		rooms[0].outboundConnections[i] = NULL;
	}

	// Then initialize the rest rooms.
	int roomsNumber = 1;
	while (roomsNumber < 7) {
		// Randomly pick one haven't used room name for the current room.
		int repeat = 0;
		int randomIndex = rand() % 10;
		int j;
		for (j = 0; j < roomsNumber; j++) {
			if (randomIndex == randomIndices[j]) {
				repeat++;
			}
		}
		
		// Once the name was assigned, initialize all other elements.
		if (!repeat) {
			randomIndices[roomsNumber] = randomIndex;
			rooms[roomsNumber].name = roomNames[randomIndices[roomsNumber]];
			// All rooms' types are initialized as MID ROOM.
			rooms[roomsNumber].type = "MID_ROOM";
			rooms[roomsNumber].connectionsNumber = 0;
			int k;
			for (k = 0; k < 6; k++) {
				rooms[roomsNumber].outboundConnections[k] = NULL;
			}
			// Once the current room has been initialized, continue to the next room.
			roomsNumber++;
		}
	}

	// Randomly generate the START ROOM.
	int randomStart = rand() % 7;
	rooms[randomStart].type = "START_ROOM";

	// Randomly generate the END ROOM, which can not be the START ROOM just generated.
	int randomEnd = rand() % 7;
	while (randomEnd == randomStart) {
		randomEnd = rand() % 7;
	}
	rooms[randomEnd].type = "END_ROOM";

	// Randomly assign outbound connections to each room and build up the room graph.
	// Use the manner recommended in the assignment instruction: adding outbound connections two at a time (forwards
	// and backwards), to randomly chosen room endpoints, until the map of all the rooms satisfies the requirements.
	while (!IsGraphFull(rooms)) {
		AddRandomConnection(rooms);
	}

	// Create seven files in the directory to hold seven rooms data.
	int l, m;
	for (l = 0; l < 7; l++) {
		char filename[35];
		// The file name for every room is <Room Name>File.
		sprintf(filename, "%s/%sFile", directoryName, rooms[l].name);
		// Open the file for write and update.
		FILE* roomFile = fopen(filename, "w+");

		// Print each room's data to its file.
		fprintf(roomFile, "ROOM NAME: %s\n", rooms[l].name);
		for (m = 0; m < rooms[l].connectionsNumber; m++) {
			fprintf(roomFile, "CONNECTION %d: %s\n", m + 1, rooms[l].outboundConnections[m]->name);
		}
		fprintf(roomFile, "ROOM TYPE: %s\n", rooms[l].type);

		fclose(roomFile);
	}

	return 0;
}