// Simple UDP echo server
// CE4961, Dr. Rothe
//
// Build with gcc

#include <stdio.h>      
#include <stdlib.h>     
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  

// Max message to echo
#define MAX_MESSAGE	1000

/* server main routine */
int main(int argc, char** argv) 
{
	// locals
	unsigned short port = 22222; // default port
	int sock; // socket descriptor

	if ((sock = socket( AF_INET, SOCK_DGRAM, 0 )) < 0) 
	{
		perror("Error on socket creation");
		exit(1);
	}
  
	char* buffer = calloc(MAX_MESSAGE,sizeof(char));

    strcpy(buffer, "Hello, world :P");
    int send_size = 16;

    struct sockaddr_in dest;
    inet_aton("192.168.24.101", &dest.sin_addr);

    dest.sin_family = AF_INET;
    dest.sin_port = htons(22222);

    sendto(sock, buffer, send_size, 0, (struct sockaddr *) &dest, sizeof(dest));
    memset(buffer, 0, sizeof(buffer));

	int bytes_read;
    struct sockaddr_in from;
    socklen_t from_len;
    from_len = sizeof(from);

    recvfrom(sock, &buffer, sizeof(buffer), 0, (struct sockaddr *) &from, sizeof(from));
	int echoed;

    // print info to console
    printf("Received message from %s port %d\n",
        inet_ntoa(from.sin_addr), ntohs(from.sin_port));

    if (bytes_read < 0)
    {
        perror("Error receiving data");
    }
    else
    {
        // make sure there is a null terminator
        if (bytes_read < MAX_MESSAGE)
            buffer[bytes_read] = '\0';
        else
            buffer[MAX_MESSAGE-1] = '\0';
            
        // put message to console
        printf("Message: %s\n",buffer);
			
    }
	// release buffer
	free(buffer);
	// close socket
	close(sock);
	// done
	return(0);
}
