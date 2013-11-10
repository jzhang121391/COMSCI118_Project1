/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "http-request.h"
#include <fstream>
#include "utilities.h"

#define PROXY_HOST "localhost"
#define PROXY_PORT "12345"
#define MAXDATASIZE 100 // max number of bytes we can get at once 

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	if(argc > 2 || argv[1] == NULL)
		error("ERROR: did not provide correct arguments");

	string URL(argv[1]), lastcomp; // Initialize variables to store URL and output name

	int scount = 0; // Initialize forward slash counter

	if(URL.substr(0, 11) == "http://www.") // Temporarily removes prefixes
		URL = URL.erase(0, 11);
	else if(URL.substr(0, 7) == "http://") // to more easily parse the URL
		URL = URL.erase(0, 7);
	else if(URL.substr(0, 4) == "www.")
		URL = URL.erase(0, 4);

	if(URL.at(URL.length()-1) == '/') // If last char in string is forward slash
		lastcomp = "index.html"; // Set output file name to index.html
	else
		for(int s = URL.length()-1; s >= 0; s--) // Parse URL string from end
		{	
			if(URL.at(s) == '/') // Stop at first forward slash
			{	
				scount++; // Keep count of how many forward slashes
				break;
			}
			lastcomp = URL.substr(s, URL.length()); // Set output file name
				// to substring after last forward slash
		}

	if(scount == 0) // If no forward slashes detected
	{
		lastcomp = "index.html"; // Set output file name to index.html
		URL = URL + '/'; // Append forward slash at end
	}

	ofstream outputFile;
	outputFile.open(lastcomp.c_str()); // Create output file with lastcomp as name

	URL = "GET http://www." + URL + " HTTP/1.0\r\n\r\n"; // Format request to send

	int sockfd, portno, n; // Initialize variables for socket
	struct sockaddr_in serv_addr;
	struct hostent *server;

	portno = PORT; // Set portno

	sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket and error check
	if(sockfd < 0)
		error("ERROR opening socket");
	
	server = gethostbyname("localhost"); // Set hostname to localhost
	
	bzero((char *) &serv_addr, sizeof(serv_addr)); // Initialize sockaddr_in variables
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting"); // Connect to host and port

	char cURL[1024]; // Create buffer to store URL in character array
	
	for(int temp = 0; temp < (int) URL.length(); temp++) // Copy URL to array
		cURL[temp] = URL.at(temp);

	n = write(sockfd, URL.c_str(), URL.size()); // Send message to server
	cout<<URL<<endl;
	//shutdown(sockfd, SHUT_WR);
	
	char *recBuf = new char [BUFFERSIZE];
    int readCorrectly;
	string message;
	do
	{
		bzero((char *)recBuf, sizeof(recBuf));
		readCorrectly=read(sockfd, recBuf, sizeof(recBuf)-1);
		if (readCorrectly<0)
			error("Error on read");

		message+=recBuf;
	}while(readCorrectly>0);
	

	message=message.substr(message.find("\r\n\r\n")+4);

	for(int temp = 0; temp < (int) message.length(); temp++)
		outputFile << message.at(temp);

	outputFile.close();
	delete [] recBuf;
	close(sockfd);
	return 0;
}
