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

typedef struct {
	double value;
	double timestamp;
}token;

int ComputeTimeStamp1 ()
{
	time_t currentTime;
	currentTime = time(NULL);
     int t_final = (int) currentTime;
}


double ComputeTimeStamp ()
{
     struct timeval ts;
	gettimeofday(&ts, NULL); // return value can be ignored
	long int seconds = ts.tv_sec; // seconds
	long int useconds = ts.tv_usec; // microseconds

     int size_us = log10(useconds)+1;
     double useconds_float = (double)useconds* pow(10, -size_us);
     double seconds_float = (float)seconds;
	 //printf( " **************** useconds FLOAT  : %.6f .\n", useconds_float);
    //  printf( " **************** seconds FLOAT  : %.6f .\n", seconds_float);
    double t_final =  seconds_float + useconds_float;
      //printf( " **************** t final   : %lf .\n",t_final);

    
     return t_final;
} 


int main(int argc, char *argv[])
{

     printf("G process execution\n");
     // G process reads from the socket (server) the token received from P, and sends it with a pipe to P
 
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     token buffer;
     struct sockaddr_in serv_addr, cli_addr;
     int n;

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
    
     char *fifo_PG = argv[2]; //create the fifo
     printf("Fifo PG created \n");
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     
     if (newsockfd < 0) 
          error("ERROR on accept");

     bzero(&buffer,sizeof(buffer));

     
     int fd_PG = open(fifo_PG, O_RDWR); // opening the fifo
     while (1)
     {

          n = read(newsockfd, &buffer.value, sizeof(buffer.value)); // read message in the socket
          printf("Message arrived to G and sent to P : %.3f\n", buffer.value);
          if (n < 0)
               error("ERROR reading from socket");
          
          //printf("Here is the message: %.3f \n",buffer);

          
		buffer.timestamp = ComputeTimeStamp ();
          int nb = write(fd_PG, &buffer, sizeof(buffer)); // write the message in the fifo
          

         
     }
     
     return 0; 
    
}

