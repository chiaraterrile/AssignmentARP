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


void error(const char *msg)
{
     perror(msg);
     exit(1);
}

int main(int argc, char *argv[])
{

     printf("G process execution:\n");
     // G process reads from the socket (server) the token received from P, and sends it with a pipe to P
 
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
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
    
     char *myfifo = argv[2]; //create the fifo

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

     bzero(buffer,256);
     n = read(newsockfd,buffer,255);

     if (n < 0) error("ERROR reading from socket");

     int fd = open(myfifo, O_RDWR); // opening the fifo
     while (1)
     {

          n = read(newsockfd, &buffer, sizeof(buffer)); // read message in the socket
          if (n < 0)
               error("ERROR reading from socket");

          int nb = write(fd, &buffer, sizeof(buffer)); // write the message in the fifo

          printf("Here is the message sent to P: %s\n",buffer);
     }
     
     return 0; 
    
}

