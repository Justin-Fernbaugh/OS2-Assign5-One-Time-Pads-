#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(const char *msg) {
	fprintf(stderr, msg);
	// perror(msg);
	exit(1);
} 

char* generateCipher(int cipherLength) {
	char* myCipher = (char*)malloc( (sizeof(char))*(cipherLength+1) );
	char newChar;
	for(int i=0; i < cipherLength; i++) {
		newChar = 'A' + (random() % 26);
		strncat(myCipher, &newChar, 1);
		// printf("char: %c\n", myCipher[i]);
	}
	myCipher[cipherLength+1] = '\0';			//add endline char maybe as well
	return myCipher;
}


int main(int argc, char **argv) 
{
	srand(time(NULL));
	if( (argc < 2) || (argc > 2) )
		error("USAGE: ./keygen PORT\n");

	// printf("argv: %s : argc %d\n", argv[1], argc);
	int cipherLength = atoi(argv[1]);
	/*atoi error handling*/

	char* myCipher = generateCipher(cipherLength);
	printf("%s\n", myCipher);

	free(myCipher);
	return 0;	 
}