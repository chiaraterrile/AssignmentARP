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
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <netdb.h>



typedef enum { false, true } bool;

// type of message exchanged between G and P processes
typedef struct {
	double value; // value of the token
	double timestamp; // timestamp in which the token is sent from G to P
}token;

// type of message exchanged between S and P processes
typedef struct {
	char* sig_name; //signal name 
	double timestamp; // timestamp in which the token is sent from S to P
}message;


token newToken ;

//PIDs of all the processes
pid_t pid_S, pid_G, pid_L, pid_P;

		
char *signame[] = {"INVALID", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGPOLL", "SIGPWR", "SIGSYS", NULL};

// fifos declaration
const char *fifo_PS = "fifo_PS"; 
const char *fifo_PG = "fifo_PG"; 
const char *fifo_PL = "fifo_PL"; 

FILE *fp; //Configuration file

	

void error(const char *msg)
{
	perror(msg);
	exit(-1);
}


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
    


// function to write the log file 
void WriteLog(token msg, token token_rx,char *signo_ch, bool isG)
{
	FILE *file;
	file = fopen("LogFile.log", "a");
	
	// if the message is coming from G process
	if (isG == true)
	{
			fprintf(file, "timestamp : %.6f from  G  process , received message :%.3f.\n", msg.timestamp, msg.value);
			fprintf(file, "timestamp : %.6f from P process, sent token : %.3f.\n\n", token_rx.timestamp, token_rx.value);
	}
	// if the message is coming from S process
	else if ( isG == false)
	{
		
		fprintf(file, "timestamp : %.6f .\n", msg.timestamp);
		fprintf(file, "From S process signal: %s.\n\n", signo_ch);	
	}
	

	fclose(file);
}

// function to print the log file content in the shell
void PrintLogFile()
{
    
   FILE *fptr;
	char c;
	fptr = fopen("LogFile.log", "r");
    if (fptr == NULL)
    {
        printf("Cannot open file \n");
        exit(0);
    }
  
    // Read contents from file
    c = fgetc(fptr);
    while (c != EOF)
    {
        printf ("%c", c);
        c = fgetc(fptr);
    }
  
    fclose(fptr);
	
}

// signal handler to manage every signal and the relative functionality (i.e. resuming or interrupting a process) 
void signal_handler(int signo)
{	
	int fd_PS = open(fifo_PS, O_RDWR); // open pipe between P and S
	
	message msg;

	if (signo == SIGUSR1) // STOP
	{
		printf("Received SIGUSR1\n");
		msg.timestamp = ComputeTimeStamp();
		msg.sig_name = signame[(int)signo] ;
		kill(pid_P, SIGSTOP); // stop sending tokens
		kill(pid_G, SIGSTOP); // stop receiving tokens
		kill(pid_L, SIGSTOP); // stop logging
		
		int nb = write(fd_PS, &msg, sizeof(msg)); // write the message in the fifo between P and S
		
		
	}
	else if (signo == SIGUSR2) // RESUME
	{
		
		printf("Received SIGUSR2\n");
		msg.timestamp = ComputeTimeStamp();
		msg.sig_name = signame[(int)signo] ;
		kill(pid_P, SIGCONT); // resume sending tokens
		kill(pid_G, SIGCONT); // resume receiving tokens
		kill(pid_L, SIGCONT); // resume logging
		
		int nb = write(fd_PS, &msg, sizeof(msg)); // write the message in the fifo between P and S
		
		

	}
	else if (signo == SIGCONT) // DUMP LOG
	{
		kill(pid_P, SIGSTOP); //  stop sending tokens
		kill(pid_G, SIGSTOP); // stop receiving tokens
		kill(pid_L, SIGSTOP); // stop logging
		printf("Received SIGCONT\n");
		
		msg.timestamp = ComputeTimeStamp();
		msg.sig_name = signame[(int)signo] ;

		// print the content of the log file
		printf("Received SIGCONT. Printing the content of the LOG file \n");
		PrintLogFile();
		
	
		sleep(2);
		// wait a new command to go on
		printf("\n Send SIGUSR2 if you want to resume the processes \n");
	
	
	}
	

}

// function to read info in the configuration file and save them into the relative variables 
void ReadFile(char *ip, char *port, int *waitingTime, char *refFreq)
{
	fp = fopen("config.txt", "r");

	if (fp == NULL)
		error("Cannot open configuration file");

	fscanf(fp, "%s", ip);
	fscanf(fp, "%s", port);
	fscanf(fp, "%d", waitingTime);
	fscanf(fp, "%s", refFreq);

	printf("IP : %s\n", ip);
	printf("Port : %s\n", port);
	printf("Waiting time : %d\n", *waitingTime);
	printf("Reference frequency : %s\n", refFreq);

	fclose(fp);
}



int main(int argc, char *argv[])
{

	char ip[32];		// ip adress of the machine
	char port[128];		// port number of processes
	char refFreq[128];	// frequency of the token wave
	int waitingTime;	// waiting time

	// save the variables from the config fle
	ReadFile(ip, port, &waitingTime, refFreq);


	int n;			   //Return value
	struct timeval tv; //Delay for select

	char *argdata[5];  //Process G execution argument
	char *shell_G = "./G"; //Process G executable path

	float t; 

	token msg1,msg2; // Message from P to L when coming from G

	message msg3; // Message from P to L when coming from S

	
	argdata[0] = shell_G;
	argdata[1] = port;
	argdata[2] = (char *)fifo_PG;
	argdata[3] = refFreq;
	argdata[4] = NULL;

	

/*------------------- Pipes Creation -------------------*/

	if (mkfifo(fifo_PS, S_IRUSR | S_IWUSR) != 0) //crete file pipe P|S
		perror("Cannot create fifo P|S. Already existing?");

	if (mkfifo(fifo_PG, S_IRUSR | S_IWUSR) != 0) //create file pipe P|G
		perror("Cannot create fifo P|G. Already existing?");

	if (mkfifo(fifo_PL, S_IRUSR | S_IWUSR) != 0) //create file pipe P|L
		perror("Cannot create fifo P|L. Already existing?");

	int fd_PS = open(fifo_PS, O_RDWR); // open pipe between P and S

	if (fd_PS == 0)
	{	
		unlink(fifo_PS);
		error("Cannot open fifo P|S");
	}
	
	

	int fd_PG = open(fifo_PG, O_RDWR); // open pipe between P and G

	if (fd_PG == 0)
	{	
		unlink(fifo_PG);
		error("Cannot open fifo P|G");
	}

	int fd_PL = open(fifo_PL, O_RDWR); // open pipe between P and L

	if (fd_PL == 0)
	{	
		unlink(fifo_PL);
		error("Cannot open fifo P|L");
	}

	printf("\n");

	/*---------------------- Process P -------------------*/
	
	pid_P = fork();

	if (pid_P < 0)
	{
		perror("Fork P failed");
		return -1;
	}

	if (pid_P == 0) 
	{
		token G_msg ; // message from G
		message S_msg;	 // message from S
		int retval, fd;
		fd_set rfds;

		sleep(2);



/*----------------- SOCKET client -----------------*/
		int sockfd, portno;
		struct sockaddr_in serv_addr;
		struct hostent *server;
		portno = atoi(port);
		token prec_tok ;

		printf("P process with PID : %d.\n", getpid());

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
			error("ERROR opening socket");

		server = gethostbyname(ip);
		if (server == NULL)
			error("ERROR, no such host\n");

		bzero((char *)&serv_addr, sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;

		bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);

		serv_addr.sin_port = htons(portno);
		if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
			error("ERROR connecting");

		// write the value of the new token on the socket between P and G
		n = write(sockfd, &newToken.value, sizeof(newToken.value));
		
		if (n < 0)
			error("ERROR writing to socket");

		
		/* ------------ SELECT ------------------- */
		while (1) 
		{
			FD_ZERO(&rfds);
			FD_SET(fd_PS, &rfds);
			FD_SET(fd_PG, &rfds);

			tv.tv_sec = 5;
			tv.tv_usec = 0;

			retval = select(20, &rfds, NULL, NULL, &tv);

			switch (retval)
			{

			case 0:
			// No active pipes 
				printf("No data avaiable.\n");
				break;

			case 1:
				// Only one pipe active, check if it is G or S
				if (FD_ISSET(fd_PS, &rfds))
				{
					n = read(fd_PS, &S_msg, sizeof(S_msg));
					if (n < 0)
						error("ERROR reading from S");
					printf("From S message received = %s \n", S_msg.sig_name);


					int flag = 0; // flag to indicate if the message is coming from G or from S ( if 0 -> S process, if 1 -> G process)
					n = write(fd_PL, &flag, sizeof(flag));
					if (n < 0)
						error("ERROR writing to L");

					n = write(fd_PL, &S_msg, sizeof(S_msg));
					//sleep((int)S_msg);
				}
				else if (FD_ISSET(fd_PG, &rfds))
				{
			
					n = read(fd_PG, &G_msg, sizeof(G_msg));
					if (n < 0)
						error("ERROR reading from G");
					

					double t_rx = ComputeTimeStamp(); // timestamp in which the message from G is received
					
					printf("From G message received = %.3f \n", G_msg.value);
					

					prec_tok = G_msg;
					double t_tx = G_msg.timestamp; // save the timestamp in which the message has been sent from G to P
					double DT = t_rx - t_tx ; // DT is the difference between the transmission (from G) and the reception (from P) of the message
					//printf("DT  = %.6f \n", DT);
					//newToken.value = prec_tok.value + DT * (1 - pow(prec_tok.value,2)/2 ) * 2 * 3.14 * 1;
					newToken.value = prec_tok.value + DT * (double)*refFreq;
					
					printf("NEW TOKEN = %.3f \n", newToken.value);
					
					// Send old and new values to L
					
					int flag = 1; // flag to indicate if the message is coming from G or from S ( if 0 -> S process, if 1 -> G process)
					n = write(fd_PL, &flag, sizeof(flag));
					if (n < 0)
						error("ERROR writing to L");

					n = write(fd_PL, &G_msg, sizeof(G_msg));
					if (n < 0)
						error("ERROR writing to L");

					
					n = write(sockfd, &newToken.value, sizeof(newToken.value));
					
					// timestamp in which the token is sent again from P to G
					newToken.timestamp = ComputeTimeStamp();

					n = write(fd_PL, &newToken, sizeof(newToken));
					if (n < 0)
						error("ERROR writing to L");

					if (n < 0)
						error("ERROR writing to socket");
					
					usleep(waitingTime); 	//Simulate communication delay
				}

				sleep(1);
				break;

			case 2:
				// Two active pipes, choose S
				
				n = read(fd_PS, &S_msg, sizeof(S_msg));
				if (n < 0)
					error("ERROR reading from S");
				printf("From S message received = %s \n", S_msg.sig_name);


				int flag = 0; 
				n = write(fd_PL, &flag, sizeof(flag));
				if (n < 0)
					error("ERROR writing to L");

				n = write(fd_PL, &S_msg, sizeof(S_msg));
				if (n < 0)
				error("ERROR writing to P");
				
				break;

			default:
				perror("You should not be here!");
				break;
			}
		}

		close(fd_PS);
		unlink(fifo_PS);

		close(fd_PG);
		unlink(fifo_PG);

		close(fd_PL);
		unlink(fifo_PL);

		close(sockfd);
	}

	else // creation of the other processes
	{
		// L process
		pid_L = fork();

		if (pid_L < 0)
		{
			perror("Fork L falied");
			return -1;
		}

		if (pid_L == 0)
		{
			printf("L process with PID : %d.\n", getpid());

			while (1)
			{
				int flag;
				n = read(fd_PL, &flag, sizeof(flag));
				if (n < 0)
					error("ERROR receiving from P");


				if (flag == 1) //recognize the type of message coming from G
				{
					n = read(fd_PL, &msg1, sizeof(msg1));
					if (n < 0)
						error("ERROR receiving from P");

					n = read(fd_PL, &msg2, sizeof(msg2));
					

					if (n < 0)
						error("ERROR receiving from P");

					char str[] = " ";

					WriteLog( msg1, msg2,str,true); 
				}
				else if (flag == 0) // recognize the type of message coming from S
				{
						n = read(fd_PL, &msg3, sizeof(msg3));
						if (n < 0)
							error("ERROR receiving from P");

						//set the proper values to write the log file in case of S process
						msg1.value = msg2.value = 0;
					    msg2.timestamp = 0;
						msg1.timestamp = msg3.timestamp;
						
						WriteLog( msg1, msg2,msg3.sig_name,false ); 
						
						
				}
				
			}

			close(fd_PL);
			unlink(fifo_PL);
		}

		else
		{
			// G process
			pid_G = fork(); 
			if (pid_G < 0)
			{
				perror("Fork G failed");
				return -1;
			}

			if (pid_G == 0)
			{
				printf("G process with PID : %d.\n", getpid());
				// execution of G process
				if (execvp(argdata[0], argdata) < 0) {     
                printf("ERROR: exec failed\n");
                exit(1);
				}
				
				

			}

			// S process
			pid_S = getpid();

			printf("S process with PID : %d.\n", getpid());

			if (signal(SIGUSR1, signal_handler) == SIG_ERR)
				printf("Can't catch SIGUSR1\n");
			

			if (signal(SIGCONT, signal_handler) == SIG_ERR)
				printf("Can't catch SIGCONT\n");

			if (signal(SIGUSR2, signal_handler) == SIG_ERR)
				printf("Can't catch SIGUSER2\n");

			sleep(5);
			
			while (1)
			{
				
			}

			close(fd_PS);
			unlink(fifo_PS);

			close(fd_PG);
			unlink(fifo_PG);

			close(fd_PL);
			unlink(fifo_PL);
		}

		return 0;


	}
}