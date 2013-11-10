/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <cstdlib>
#include "http-request.h"
#include "utilities.h"
#include <tr1/unordered_map>
#include <pthread.h>

using namespace std;


void* readRequest(void* sockfd);
bool foundInCache(string URL);
string fetchRequest(string request);

tr1::unordered_map<string, string> cache;
int main (int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    
    struct sockaddr_in serv_addr, cli_addr;

    // Open socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
       	error("ERROR opening socket");
    }
    portno = PORT;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Bind to socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        error("ERROR on binding");
	}

    // listen/accept (blocking call)
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    
   pthread_t thread; 
   //create counter to walk through pthreadArr
   int threadNum = 0;   

	// Server Infinite Loop
    while (1) {
		//Accept connection
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		if (newsockfd < 0){
        	cerr<<"ERROR on accept"<<endl;
        	continue;
		}
		//fork each new process	
		threadNum = threadNum + 1;	
		pthread_create(&thread, NULL, readRequest, &newsockfd);
	 }
	

	close(sockfd);
	return 0;
}
void* readRequest(void* sockfd)
{
	int* sfd = (int*) sockfd;     
	char *request=new char[BUFFERSIZE];
    
    int readCorrectly;
	string wholeRequest;
	do
	{
		bzero((char*)request,sizeof(request));
		readCorrectly=read(*sfd, request, sizeof(request)-1);
		if (readCorrectly<0){
			cerr<<"Error on read"<<endl;
			break;
		}
		wholeRequest+=request;
		if (wholeRequest.find("\r\n\r\n") != string::npos) {
            break;
        }
	}while(readCorrectly>0);
	
	string response;
	if(foundInCache(wholeRequest)==true)
	{
		response=cache[wholeRequest];
//		cout<<"already exists"<<endl;
	}
	else	
	    response=fetchRequest(wholeRequest);
	write(*sfd, response.c_str(), response.size()+1);
	delete [] request;
	close(*sfd);
	return NULL;
}

bool foundInCache(string URL)
{
	tr1::unordered_map<string, string>::const_iterator it = cache.find(URL);
	if(it==cache.end())
		return false;
	return true;
}

string fetchRequest(string request)
{
	string valid;
	HttpRequest req;
   	char *input;
   	input=new char[request.size()+1];
   	strcpy(input, request.c_str());
   	 //add CRLF
   
	//fetch request and check for bad requests 
   	valid = req.ParseRequest(input, strlen(input));
   	req.ModifyHeader ("Connection", "close"); 
   	
	if(valid == "400 - Bad Request")
	{
		return "400 - Bad Request";
	}
  	size_t reqLen = req.GetTotalLength();
	if(reqLen == 0)
		return "400 - Bad Request";
  
	char *sendBuf = new char [reqLen+1];
 
	//format the request
	valid =	req.FormatRequest (sendBuf);
    if(valid == "400 - Bad Request")
    {
        return "400 - Bad Request";
    }
   	
	//cout<<sendBuf<<endl;
//////////////////////////////////////
///////Get the address of URL/////////
///////////////////////////////////////	

	struct addrinfo hints, *res;
	int HTTPsockfd;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	string hostname=req.GetHost();
	stringstream ss;
	ss<<req.GetPort();
	string cport=ss.str();
	getaddrinfo(hostname.c_str(), cport.c_str(), &hints, &res);

//////////////////////////////////////
///////Create socket and Connect//////
//////////////////////////////////////	

	HTTPsockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (HTTPsockfd < 0){
		cerr<<"ERROR, cannot create socket"<<endl;
	}

	if (connect(HTTPsockfd,res->ai_addr, res->ai_addrlen) < 0) {
		cerr<<"ERROR, cannot connect"<<endl;
	}

//////////////////////////////////////
//////Write/send the http request/////
//////////////////////////////////////	
	char *recBuf = new char [1024];
	if(write(HTTPsockfd, sendBuf, reqLen)<0)
		cerr<<"Error on writing to socket."<<endl;
	
//////////////////////////////////////
///////Read in the http response//////
//////////////////////////////////////	
	int readCorrectly;
	string message;
	do
	{
		bzero((char *)recBuf, sizeof(recBuf));
		readCorrectly=read(HTTPsockfd, recBuf, sizeof(recBuf)-1);
		if (readCorrectly<0)
			cerr<<"Error on read"<<endl;

		message+=recBuf;

	}while(readCorrectly>0);
	
	
	
	cache[request]=message;
//	cout<<cache[request];
	
//////////////////////////////////////
/////////////Clean up/////////////////
//////////////////////////////////////	
	freeaddrinfo(res);
	delete [] input;
	delete [] sendBuf;
	delete [] recBuf;
	close(HTTPsockfd);
	
	return message;
}
