// Simple TCP echo client.  Reads a single line from STDIN and
// sends to server, including null-terminator

#define _GNU_SOURCE 
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <netdb.h>	


#define MAX_MESSAGE 1000

int main( int argc, char **argv ) {

	// locals
	int sock;
	struct sockaddr_in server;
	struct hostent *hp;

	// check command-line args
	if (argc!=3)
	{
		printf("Usage: tcpechoclient <server name> <port number>\n");
		exit(0);
	}

	// create socket 
	// IP protocol family (PF_INET) 
	// UDP (SOCK_STREAM)

	if ((sock = socket( PF_INET, SOCK_STREAM, 0 )) < 0)
	{
		perror("Error creating socket");
		exit(1);
	}

	// TCP - client will be more like server than UDP example.
	// Since we are going active right away, we can skip bind
	// if we like.

	// Make a sockaddr of the server
	// address family is IP  (AF_INET)
	// server IP address is found by calling gethostbyname with the
	// name of the server (entered on the command line)
	// note, if an IP address is provided, that is OK too

	server.sin_family = AF_INET;
	
	if ((hp = gethostbyname(argv[1]))==0)
	{
		perror("Invalid or unknown host");
		exit(1);
	}

	// copy IP address into address structure
	memcpy( &server.sin_addr.s_addr, hp->h_addr, hp->h_length);

	// establish the server port number - we must use network byte order! 
	unsigned short port;

	if (sscanf(argv[2],"%hu",&port)!=1)
	{
		perror("Error parsing port");
		exit(0);
	}
	
	server.sin_port = htons(port);

	// ready to connect
	printf("Ready to connect to %s at port %hu\n",hp->h_name,port);

	if ( connect(sock, (struct sockaddr *) &server, sizeof(server) ) < 0 )
	{
		perror("Error calling connect");
		exit(-1);
    }
	
	char buffer[MAX_MESSAGE];
	int size_to_send;
	int size_sent;
	int bytes_read;

	while(1)
	{
		printf("Message to send: ");

		// get a line
		fgets (buffer, MAX_MESSAGE, stdin);
		
		// how big?
		size_to_send = strlen(buffer);
		
		if(size_to_send == 1)
		{
			// just a newline - close socket
			printf("==== Client Closing Connection ====\n");
			close(sock);
			break;
		}
		
		// send to server
		size_sent = write(sock, buffer, size_to_send); // send null

		printf("sent %d\n", size_sent);

		if (size_sent<0) 
		{
			perror("Error sending data");
			exit(1);
		}		
		
		// clear buffer
		memset(buffer, 0, MAX_MESSAGE);
		
		// copy null-terminated string into buffer
		if ((bytes_read = read(sock, buffer, MAX_MESSAGE-1))<0)
		{
			perror("Error calling read");
			exit(-1);
		}

		if (bytes_read == 0)
		{	// socket closed
			printf("====Disconnected====\n");
			close(sock);
			break;  // break the inner while loop
		}

		// make sure there is a null terminator before sending to console 
		buffer[bytes_read] = '\0';

		// echo to console
		printf("Echo: %s\n",buffer);

	}
	return(0);
}






