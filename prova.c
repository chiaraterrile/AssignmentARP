#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>

typedef enum { false, true } bool;
bool f9 = true;
void error(const char *msg)
{
     perror(msg);
     exit(1);
}

void f1( int t1, int t2){
     printf("t1 = %d \n", t1);
	
}

int main(int argc, char *argv[])
{

char buffer[256];

char str[] = "ciao";

int nb = write(*buffer, &str, sizeof(str)); 
printf( "%s. \n", buffer);
    
}

