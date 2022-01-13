// Simple TCP echo server

#include <stdio.h>      
#include <stdlib.h>     
#include <unistd.h>  
#include <string.h>	
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <pthread.h>


// Max message to echo
#define MAX_MESSAGE	1000

/* server main routine */




void*  tcpThread(void * arg)
{

	int connection = *((int *) arg);

	char* buffer = calloc(MAX_MESSAGE,sizeof(char));
	int bytes_read;
	int echoed;

	while(1)
	{
		buffer[0] = '\0'; 	// guarantee a null here to break out on a
							// disconnect

		// read message								
		bytes_read = read(connection,buffer,MAX_MESSAGE-1);
					
		if (bytes_read == 0)
		{	// socket closed
			printf("====Client Disconnected====\n");
			close(connection);
			break;  // break the inner while loop
		}

		// make sure buffer has null temrinator
		buffer[bytes_read] = '\0';
					
		// see if client wants us to disconnect
		// if (strncmp(buffer,"quit",4)==0)
		// {
		// 	printf("====Server Disconnecting====\n");
		// 	close(connection);
		// 	break;  // break the inner while loop
		// }
		
		// print info to console
		// printf("Received message\n");

		// put message to console
		// printf("Message: %s\n", buffer);


		char verb[5], path[64], version[64], headers[256], body[256]; //todo malloc these


		sscanf(buffer, "%s %s %s\n%s\n\n%s", verb, path, version, headers, body);


		// printf("Verb: %s\n Path: %s\n", verb, path);

		//default
		if (strcmp(path, "/") == 0)
		{
			strcpy(path, "/index.html");
		}


		printf("getting file %s..\n\n", &path[1]);
		FILE * fd = fopen(&path[1], "r");

		if (fd == NULL)
		{
			//404
			perror("404");
		}
		else 
		{

			fseek(fd, 0, SEEK_END);
			long fsize = ftell(fd);
			fseek(fd, 0, SEEK_SET);  /* same as rewind(f); */

			char *string = malloc(fsize + 1);
			fread(string, fsize, 1, fd);
			fclose(fd);

			string[fsize] = 0;

			// send it back to client
			if ( (echoed = write(connection, string, fsize + 1)) < 0 )
			{
				perror("Error sending response");
				exit(-1);
			}
			else
			{			
				printf("Bytes sent: %d\n",echoed);
			}
		}

	}  // end of accept inner-while
	// the connection is closed

	pthread_exit(NULL);

}



int main(int argc, char** argv) {

	// locals
	unsigned short port = 22222; // default port
	int sock; // socket descriptor

	// Was help requested?  Print usage statement
	if (argc > 1 && ((!strcmp(argv[1],"-?"))||(!strcmp(argv[1],"-h"))))
	{
		printf("\nUsage: tcpechoserver [-p port] port is the requested \
 port that the server monitors.  If no port is provided, the server \
 listens on port 22222.\n\n");
		exit(0);
	}
	
	// get the port from ARGV
	if (argc > 1 && !strcmp(argv[1],"-p"))
	{
		if (sscanf(argv[2],"%hu",&port)!=1)
		{
			perror("Error parsing port option");
			exit(0);
		}
	}
	
	// ready to go
	printf("tcp echo server configuring on port: %d\n",port);
	
	// for TCP, we want IP protocol domain (PF_INET)
	// and TCP transport type (SOCK_STREAM)
	// no alternate protocol - 0, since we have already specified IP
	
	if ((sock = socket( PF_INET, SOCK_STREAM, 0 )) < 0) 
	{
		perror("Error on socket creation");
		exit(1);
	}
  
  	// lose the pesky "Address already in use" error message
	int yes = 1;

	if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) 
	{
		perror("setsockopt");
		exit(1);
	}

	// establish address - this is the server and will
	// only be listening on the specified port
	struct sockaddr_in sock_address;
	
	// address family is AF_INET
	// our IP address is INADDR_ANY (any of our IP addresses)
    // the port number is per default or option above

	sock_address.sin_family = AF_INET;
	sock_address.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_address.sin_port = htons(port);

	// we must now bind the socket descriptor to the address info
	if (bind(sock, (struct sockaddr *) &sock_address, sizeof(sock_address))<0)
	{
		perror("Problem binding");
		exit(-1);
	}
	
	// extra step to TCP - listen on the port for a connection
	// willing to queue 5 connection requests
	if ( listen(sock, 5) < 0 ) 
	{
		perror("Error calling listen()");
		exit(-1);
	}
	

	// go into forever loop and echo whatever message is received
	// to console and back to source

	int connection;
	
	pthread_t dontcare;

    while (1) {
				
		// hang in accept and wait for connection
		printf("====Waiting====\n");
		if ( (connection = accept(sock, NULL, NULL) ) < 0 ) 
		{
			perror("Error calling accept");
			exit(-1);
		}		
		
		pthread_create(&dontcare, NULL, tcpThread, &connection);
		// ready to r/w - another loop - it will be broken when
		
    }	// end of outer loop

	// will never get here
	return(0);
}








