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



// function to compute the current timestamp expressed as a double
double ComputeTimeStamp ()
{
    struct timeval ts;
	gettimeofday(&ts, NULL); 
	long int seconds = ts.tv_sec; // seconds
	long int useconds = ts.tv_usec; // microseconds

    int size_us = log10(useconds)+1;
    double useconds_d = (double)useconds* pow(10, -size_us);
    double seconds_d = (float)seconds;

    double t_final =  seconds_d + useconds_d;
    
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

