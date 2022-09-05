#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <string.h>


// Error function used for reporting issues
void error(const char *msg) {
//   perror(msg);
  fprintf(stderr, msg);
  exit(1);
}
/*-------------------------------------------------*/
/*
	Intializes the address struct and sets values to accept connections.
*/
// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}
/*-------------------------------------------------*/
/*
	Sends string to server
	Will also ensure data is sent before exiting.
*/
int sendSocketMsg(int socketFD, const char* buff) {
	// printf("plaintextss: %s\n", buff);
	int bytes_sent = 0, bytesTotal = 0, buffLength;
	buffLength = strlen(buff);

	if(strlen(buff) > 255)
		buffLength = 255;
	else
		buffLength = strlen(buff);

	// bytes_sent = send(socketFD, buff, buffLength, 0); 
	while(bytesTotal < buffLength) {
		bytes_sent = send(socketFD, buff, buffLength, 0); 
		bytesTotal += bytes_sent;
		if (bytes_sent < 0){
			error("CLIENT: ERROR writing to socket");
		}
	}
	return 1;
}
/*-------------------------------------------------*/
/*
	Opens txt file given in parameters and extracts data into string.
*/
void getFileString(char* file_name, char* fileContents) {
    FILE* ptr;
    char ch;
	
    ptr = fopen(file_name, "r");
 
    if (NULL == ptr) {
        error("file can't be opened \n");
    }
 
    while ((ch = fgetc(ptr)) != EOF) {
        // ch = fgetc(ptr);
		strncat(fileContents, &ch, 1);
        // printf("%c", ch);
    }

	// Remove the trailing \n that fgets adds
	fileContents[strcspn(fileContents, "\n")] = '\0';
	
    fclose(ptr);
    return;
}
//https://cboard.cprogramming.com/c-programming/127633-strange-question-mark-eof.html
//https://www.geeksforgeeks.org/c-program-to-read-contents-of-whole-file/
/*-------------------------------------------------*/
/*
	Authenticates client with server via token passed back and forth.
*/
void authSocket(int socketFD) {
	int bytes_sent;
	int charsRead;
	char clientToken[] = "client2002";
	char buffer[256];

	sendSocketMsg(socketFD, clientToken);

	// Get return message from server
	// Clear out the buffer again for reuse
	memset(buffer, '\0', sizeof(buffer));
	// Read data from the socket, leaving \0 at end
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
	if (charsRead < 0){
		error("CLIENT: ERROR reading from socket");
	}

	//check auth
	if(strcmp(buffer, clientToken) == 0) {
		// printf("CLIENT: authenticated\n");
	} else {
		// printf("CLIENT: authentication failed\n");
		error("auth failed");
	}
}
/*-------------------------------------------------*/
/*
	Ensures the values given are within the ASCII range for the alphabet.
*/
int checkBufferInput(const char* buffer) {
	char curr;
	for(int i=0; i < strlen(buffer); i++) {
		if(buffer[i] < 65 || buffer[i] > 90) {
			if(buffer[i] == 32)
				continue;
			else
				error("Non alphabet input\n");
		}
	}
}
/*-------------------------------------------------*/
/*
	Sends the msg and key to the server
	calls checkBuffer prior to it being sent.
*/
void sendEncryptParts(int socketFD, char* plaintext, char* cipherKey) {
	checkBufferInput(plaintext);
	checkBufferInput(cipherKey);

	//check if encryptKey is less than the plaintext
	if(strlen(plaintext) > strlen(cipherKey))
		error("Key is shorter than message\n");

	// printf("plaintexts: %s\n", plaintext);
	sendSocketMsg(socketFD, plaintext);
	sendSocketMsg(socketFD, cipherKey);
		
	// printf("plaintext %s\n", plaintext);
	// printf("encryptKey %s\n", cipherKey);	
}
/*-------------------------------------------------*/
int main(int argc, char *argv[]) {
	//
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	char buffer[256];
	char host[] = "localhost";

	// Check usage & args
	if (argc < 4) { 
		fprintf(stderr,"USAGE: %s msg key port\n", argv[0]); 
		exit(0); 
	}

	char* plaintextContents = malloc(sizeof(char)*(80000));
	char* keyContents = malloc(sizeof(char)*(80000));
	getFileString(argv[1], plaintextContents);
	getFileString(argv[2], keyContents);

	// printf("plaintextContents: %s\n", plaintextContents);
	// printf("keyContents: %s\n", keyContents);

	// Create a socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); 
	if (socketFD < 0){
		error("CLIENT: ERROR opening socket");
	}

	// Set up the server address struct
	setupAddressStruct(&serverAddress, atoi(argv[3]), host);

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
		error("CLIENT: ERROR connecting");
	}

	//authenticate client with server
	authSocket(socketFD);

	//get user inputs
	sendEncryptParts(socketFD, plaintextContents, keyContents);

	// Get return message from server
	// Clear out the buffer again for reuse
	memset(buffer, '\0', sizeof(buffer));
	// Read data from the socket, leaving \0 at end
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
	if (charsRead < 0){
		error("CLIENT: ERROR reading from socket");
	}
	// printf("\"%s\"\n", buffer);
	printf("%s\n", buffer);

	// Close the socket
	close(socketFD); 
	free(plaintextContents);
	free(keyContents);
  	return 0;
}