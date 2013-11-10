all:	
	g++ -Wall -pedantic http-get.cc http-request.cc http-headers.cc utilities.cpp -o http-get
	
	g++ -Wall -pedantic -lpthread http-proxy.cc http-response.cc http-request.cc http-headers.cc utilities.cpp -o http-proxy
	
get:

	g++ -Wall -pedantic http-get.cc http-request.cc http-headers.cc utilities.cpp -o http-get

proxy:

	g++ -Wall -pedantic -lpthread http-proxy.cc http-response.cc http-headers.cc utilities.cpp -o http-proxy

clean:
	rm http-get
	rm http-proxy
