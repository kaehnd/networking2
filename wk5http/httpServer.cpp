/**
 * httpServer.cpp
 * Author: Daniel Kaeh (using starting code by Dr. Rothe)
 * Description: Implements main routine of simple HTTP server supporting
 * 		"GET" requests only, displaying a 404 page if the path wasn't found.
 * 
 */  

#include <unistd.h>  
#include <cstring>
#include <arpa/inet.h>  
#include <pthread.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>
#include <map>
#include <poll.h>
#include <filesystem>

// Max message to echo
#define MAX_MESSAGE	1000

/* server main routine */


static const std::map< std::string, std::string > mime_types {
  {".html", "text/html"},
  {".jpg", "image/jpg"},
  {".png", "image/png"},
  {".gif", "image/gif"}
};


// void pollHttpMessage(int connection)
// {

// }

void*  tcpThread(void * arg)
{
	int connection = *((int *) arg);

	std::vector<char> inputBuffer(MAX_MESSAGE);
	int bytes_read;
	int echoed;

	while(1)
	{
		inputBuffer[0] = '\0'; 	// guarantee a null here to break out on a
							// disconnect
		
		struct pollfd fd;
		int ret;

		fd.fd = connection;
		fd.events = POLLIN;
		ret = poll(&fd, 1, 5000); // 10 second for timeout

		if (ret == -1)
		{
			std::perror ("Error polling");
			pthread_exit(NULL);
		}
		if (ret == 0)
		{
			std::cout << "====Timeout====" << std::endl;
			close(connection);
			pthread_exit(NULL);
		}

		// read message								
		bytes_read = read(connection,inputBuffer.data(),MAX_MESSAGE-1);
					
		if (bytes_read == 0)
		{	// socket closed
			std::cout <<"====Client Disconnected====" << std::endl;
			close(connection);
			pthread_exit(NULL);  // break the inner while loop
		}

		// make sure buffer has null temrinator
		inputBuffer[bytes_read] = '\0';
		

		std::stringstream inStream(inputBuffer.data());
		std::string line;

		std::getline(inStream, line);

		std::string verb, pathStr, version;
		std::stringstream(line) >> verb >> pathStr >> version;

		//parse request headers
		std::map<std::string, std::string> inHeaders;
		std::string key, value;
		while (!inStream.eof())
		{
			std::getline (inStream, key, ':');

			if (key == "\r\n")
			{
				break;
			}
			std::getline (inStream, value);
			
			bool skipFirst = value[0] == ' ';
			bool skipLast = value[value.length()-1] == '\r';

			inHeaders[key] = value.substr( skipFirst ? 1 : 0, skipLast ? value.length()-2 : value.length() - 1 );
		}

		bool keepAlive = inHeaders["Connection"] == "keep-alive";

		std::filesystem::path path(&pathStr[1]); //ignore leading root '/'

		//default
		if (pathStr == "/")
		{
			path = "index.html";
		}

		std::ifstream file;
		file.open(path);

		std::string status = "200 OK";

		if (!file.is_open())
		{
			//File wasn't found
			perror("404");
			file.open("404.html");
			status = "404 Not Found";
		}
		std::map<std::string, std::string> outHeaders;


		std::streampos file_size;

		//in case 404.html isn't found
		if (file.is_open())
		{
			file.seekg(0, std::ios::end);
			file_size = file.tellg();
			file.seekg(0, std::ios::beg);
		}
		else 
		{
			file_size = 0;
		}

		outHeaders["Connection"] = keepAlive ? "keep-alive" : "close";
		outHeaders["Content-Length"] = std::to_string(file_size);
		
		auto it = mime_types.find(path.extension());
		outHeaders["Content-Type"] = it == mime_types.end() ? "application/unknown" : it -> second;


		//Construct output buffer
		std::vector<unsigned char> outputBuf;

		outputBuf.insert(outputBuf.end(), version.begin(), version.end());
		outputBuf.push_back(' ');
		outputBuf.insert(outputBuf.end(), status.begin(), status.end());
		outputBuf.push_back('\r');
		outputBuf.push_back('\n');

		//Add headers
		for (const auto& [outKey, outValue] : outHeaders) {
			outputBuf.insert(outputBuf.end(), outKey.begin(), outKey.end());
			outputBuf.push_back(':');
			outputBuf.push_back(' ');
			outputBuf.insert(outputBuf.end(), outValue.begin(), outValue.end());
			outputBuf.push_back('\r');
			outputBuf.push_back('\n');
		}
		outputBuf.push_back('\r');
		outputBuf.push_back('\n');

		// auto headerSize = outputBuf.size();

		//Add body

		if (file.is_open())
		{
			outputBuf.reserve(file_size);
			file.unsetf(std::ios::skipws); //otherwise skips all whitespace
			outputBuf.insert(outputBuf.end(),
				std::istream_iterator<unsigned char>(file),
				std::istream_iterator<unsigned char>());
		}


		// send it back to client
		if ( (echoed = write(connection, outputBuf.data(), outputBuf.size())) < 0 )
		{
			std::perror("Error sending response");
			pthread_exit(NULL);
		}
		else
		{			
			// std::cout << "Bytes sent: " << echoed << std::endl;
			// std::cout << "Header length: " << headerSize << std::endl;
			// std::cout << "Content-Length: " << outHeaders["Content-Length"] << std::endl;
			// std::cout << "Output buf size: " << outputBuf.size() << std::endl;
		}

		if (!keepAlive)
		{
			std::cout << "====Server Disconnecting====" << std::endl;
			close(connection);
			pthread_exit(NULL);  // break the inner while loop		
		}
		
	}  // end of accept inner-while
	// the connection is closed
	pthread_exit(NULL);
}



int main(int argc, char** argv) 
{
	// locals
	unsigned short port = 80; // default port
	int sock; // socket descriptor

	// Was help requested?  Print usage statement

	if (argc > 1)
	{
		std::string arg1(argv[1]);
		if (arg1 == "-?" || arg1 =="-h")
		{
			std::cout << std::endl << "Usage: tcpechoserver [-p port] port is the requested \
                port that the server monitors.  If no port is provided, the server \
                listens on port 80." 
                << std::endl << std::endl;
			std::exit(0);
		}
		
		// get the port from ARGV
		if (arg1 =="-p")
		{
			if (sscanf(argv[2],"%hu",&port)!=1)
			{
				std::perror("Error parsing port option");
				std::exit(0);
			}
		}
	}

	
	// ready to go
	std::cout << "tcp echo server configuring on port:"<< port << std::endl;
	
	// for TCP, we want IP protocol domain (PF_INET)
	// and TCP transport type (SOCK_STREAM)
	// no alternate protocol - 0, since we have already specified IP
	
	if ((sock = socket( PF_INET, SOCK_STREAM, 0 )) < 0) 
	{
		std::perror("Error on socket creation");
		std::exit(1);
	}
  
  	// lose the pesky "Address already in use" error message
	int yes = 1;

	if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) 
	{
		std::perror("setsockopt");
		std::exit(1);
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
		std::perror("Problem binding");
		std::exit(-1);
	}
	
	// extra step to TCP - listen on the port for a connection
	// willing to queue 5 connection requests
	if ( listen(sock, 5) < 0 ) 
	{
		std::perror("Error calling listen()");
		std::exit(-1);
	}

	// go into forever loop and echo whatever message is received
	// to console and back to source

	int connection;
	
	pthread_t dontcare;

    while (1) 
		{
				
			// hang in accept and wait for connection
			std::cout<<"====Waiting===="<<std::endl;
			if ( (connection = accept(sock, NULL, NULL) ) < 0 ) 
			{
				std::perror("Error calling accept");
				std::exit(-1);
			}		
			pthread_create(&dontcare, NULL, tcpThread, &connection);
			// ready to r/w - another loop - it will be broken when
    }	// end of outer loop

	// will never get here
	return(0);
}
