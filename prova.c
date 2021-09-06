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


void error(const char *msg)
{
     perror(msg);
     exit(1);
}


int main(int argc, char *argv[])
{

    struct timeval ts;
	gettimeofday(&ts, NULL); // return value can be ignored
	long int seconds = ts.tv_sec; // seconds
	long int useconds = ts.tv_usec; // microseconds

     int size_us = log10(useconds)+1;
     double useconds_float = (double)useconds* pow(10, -size_us);
     double seconds_float = (float)seconds;
	 printf( " **************** useconds FLOAT  : %.6f .\n", useconds_float);
      printf( " **************** seconds FLOAT  : %.6f .\n", seconds_float);
    double t_final =  seconds_float + useconds_float;
      printf( " **************** t final   : %lf .\n",t_final);

    
     
     return 0; 
    
}

