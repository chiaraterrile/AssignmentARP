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
#include <time.h>
#include <sys/select.h>
#include <netdb.h>

#define max(a, b) \
	({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


double newToken ;
char *timeString;
pid_t pid_S, pid_G, pid_L, pid_P;

char *signame[] = {"INVALID", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGPOLL", "SIGPWR", "SIGSYS", NULL};

const char *fifo_PS = "fifo_PS"; //path
const char *fifo_PG = "fifo_PG"; //path
const char *fifo_PL = "fifo_PL"; //path

FILE *fp; //Configuration file

void error(const char *msg)
{
	perror(msg);
	exit(-1);
}

/* function to write the log file */
void WriteLog(pid_t PID, float msg, double token)
{
	FILE *file;
	file = fopen("LogFile.log", "a");
	time_t time_clock; //current time
	time_clock = time(NULL);
	//timeString = ctime(&currentTime);
	
	fprintf(file, "-%sPID: %d value:%.3f.\n", ctime(&time_clock), PID, msg);
	fprintf(file, "-%s token : %.3f.\n\n", ctime(&time_clock), token);

	fclose(file);
}

/* signal handler to manage every signal and the relative functionality (i.e. resuming or interrupting a process) */
void signal_handler(int signo)
{
	if (signo == SIGUSR1) // STOP
	{
		printf("Received SIGUSR1\n");
		kill(pid_P, SIGSTOP); // sending tokens
		kill(pid_G, SIGSTOP); // receiving tokens
		kill(pid_L, SIGSTOP); // logging
	}
	else if (signo == SIGUSR2) // START
	{
		printf("Received SIGUSR2\n");
		kill(pid_P, SIGCONT); // sending tokens
		kill(pid_G, SIGCONT); // receiving tokens
		kill(pid_L, SIGCONT); // logging
	}
	else if (signo == SIGCONT) // DUMP LOG
	{
		printf("Received SIGCONT\n");
		printf("Process S PID %d\n :", pid_S);
		WriteLog(pid_S, (float)signo, newToken);
		printf("-%sPID: %d value:%s.\n", timeString, pid_S, signame[(int)signo]);
		printf("-%s%.3f.\n\n", timeString, newToken);
	}
}

/* function to read info in the configuration file and save them into the relative variables  */
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
	char refFreq[128];		// frequency of the tokeen wave
	int waitingTime;	// waiting time

	// save the variables from the config fle
	ReadFile(ip, port, &waitingTime, refFreq);


	int n;			   //Return value
	struct timeval tv; //Select delay

	char *argdata[5];  //Process G execution argument
	char *shell_G = "./G"; //Process G executable path

	float msg1, msg2, t; //Message from P to L
	//char refFreqchar = refFreq ;
	
	argdata[0] = shell_G;
	argdata[1] = port;
	argdata[2] = (char *)fifo_PG;
	argdata[3] = refFreq;
	argdata[4] = NULL;



/*------------------- Pipes Creation -------------------*/

	if (mkfifo(fifo_PS, S_IRUSR | S_IWUSR) != 0) //creo file pipe P|S
		perror("Cannot create fifo P|S. Already existing?");

	if (mkfifo(fifo_PG, S_IRUSR | S_IWUSR) != 0) //creo file pipe P|G
		perror("Cannot create fifo P|G. Already existing?");

	if (mkfifo(fifo_PL, S_IRUSR | S_IWUSR) != 0) //creo file pipe P|L
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

	if (pid_P == 0) // son's code (P process execution) 
	{
		double G_msg ; // message from G
		double S_msg;	 // message from S
		int retval, fd;
		fd_set rfds;

		sleep(2);



/*----------------- SOCKET client -----------------*/
		int sockfd, portno;
		struct sockaddr_in serv_addr;
		struct hostent *server;
		portno = atoi(port);
		double prec_tok ;

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

		//n = write(sockfd, &G_msg, sizeof(G_msg));
		n = write(sockfd, &newToken, sizeof(newToken));
		if (n < 0)
			error("ERROR writing to socket");

		
		
		while (1) // Select body
		{
			FD_ZERO(&rfds);
			FD_SET(fd_PS, &rfds);
			FD_SET(fd_PG, &rfds);

			fd = max(fd_PS, fd_PG);

			tv.tv_sec = 5;
			tv.tv_usec = 0;

			retval = select(fd + 1, &rfds, NULL, NULL, &tv);

			switch (retval)
			{

			case 0:
			//Either no active pipes or the timeout has expired
				printf("No data avaiable.\n");
				break;

			case 1:
				// If only one pipe is active, check which is between S and G
				if (FD_ISSET(fd_PS, &rfds))
				{
					n = read(fd_PS, &S_msg, sizeof(S_msg));
					if (n < 0)
						error("ERROR reading from S");
					printf("From S recivedMsg = %.3f \n", S_msg);
					sleep((int)S_msg);
				}
				else if (FD_ISSET(fd_PG, &rfds))
				{
					// If G, make the computation and log the results through L
					n = read(fd_PG, &G_msg, sizeof(G_msg));
					if (n < 0)
						error("ERROR reading from G");
					/* if (G_msg < -1 || G_msg > 1)
					{
						printf("Value should be between -1 and 1!.\n");
						break;
					} */
					
					printf("From G recivedMsg = %.3f \n", G_msg);

					n = write(fd_PL, &G_msg, sizeof(G_msg));
					if (n < 0)
						error("ERROR writing to L");
					prec_tok = G_msg;
					newToken = prec_tok + 2 * (1 - pow(prec_tok,2)/2 ) * 2 * 3.14 * 1;
					printf("NEW TOKEN = %.3f \n", newToken);
					//G_msg += 1; 			////////////////////////////////////////////FORMULA////////////////////////////////////////////////

					//newToken = G_msg;
					
					// Send new value to L
					n = write(fd_PL, &newToken, sizeof(newToken));
					if (n < 0)
						error("ERROR writing to L");

					// Write new value to the socket
					//n = write(sockfd, &G_msg, sizeof(G_msg));
					n = write(sockfd, &newToken, sizeof(newToken));
					if (n < 0)
						error("ERROR writing to socket");
					
					usleep(waitingTime); 	//Simulate communication delay
				}

				sleep(1);
				break;

			case 2:
				// If two active pipes, give priority to S
				n = read(fd_PS, &S_msg, sizeof(S_msg));
				if (n < 0)
					error("ERROR reading from S");
				printf("From S recivedMsg = %.3f \n", S_msg);
				sleep((int)S_msg);
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
				n = read(fd_PL, &msg1, sizeof(msg1));
				if (n < 0)
					error("ERROR receiving from P");

				n = read(fd_PL, &msg2, sizeof(msg2));
				if (n < 0)
					error("ERROR receiving from P");

				WriteLog(getpid(), msg1, msg2);
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
				/*execvp(argdata[0], argdata);
				error("Exec failed");
				return 0;
				*/
				
				if (execvp(argdata[0], argdata) < 0) {     
                printf("ERROR: exec failed\n");
                exit(1);
				}
				
				

			}

			// process S

			pid_S = getpid();

			printf("S process with PID : %d.\n", getpid());

			if (signal(SIGUSR1, signal_handler) == SIG_ERR)
				printf("Can't catch SIGUSR1\n");

			if (signal(SIGCONT, signal_handler) == SIG_ERR)
				printf("Can't catch SIGCONT\n");

			if (signal(SIGUSR2, signal_handler) == SIG_ERR)
				printf("Can't catch SIGUSER2\n");

			//srand(time(0)); //current time as seed of random number generator
			sleep(5);

			while (1)
			{
				/* t = (rand() % (10 + 1));
				n = write(fd_PS, &t, sizeof(t));
				if (n < 0)
					error("ERROR writing to P");
				sleep(rand() % (10 + 1)); */
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