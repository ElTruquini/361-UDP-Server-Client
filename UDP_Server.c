//UDP Server wrote by Daniel Olaya, Sept 17

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAXBUFLEN 1024
#define PORT "1212"
#define PATH_SIZE 100


void die(char *s){
	perror(s);
	exit(1); //unsuccessful termination
}

// get port, IPv4 or IPv6:
in_port_t get_in_port(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }
    return (((struct sockaddr_in6*)sa)->sin6_port);
}


int main(int argc, char *argv[]){
	int s, rv, numbytes, port, nread;
    char buffer[MAXBUFLEN], path[PATH_SIZE];
    socklen_t addr_len;
	struct sockaddr_storage their_addr; // connector's address information
	struct addrinfo hints, *server;
	FILE *fp;
    
    //Getting path from args
    if (argc != 2) {
        fprintf(stderr,"Error - Missing arguments, use: UDP_Server path\n");
        exit(1);
    }
    strncpy(path, argv[1], PATH_SIZE-1);
    printf("\nInitializing service w/ path: %s \n", path);

    // Setting up getaddrinfo struct
	memset(&hints, 0 ,sizeof hints);
	hints.ai_family = AF_UNSPEC; //AF_UNSPEC (both), AF_INET (IPv4) or AF_INET6 (IPv6)
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // bind to the IP of the host it's running on
	hints.ai_protocol = IPPROTO_UDP;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &server)) != 0){ //Adding port to struct, first param left as NULL because 
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(rv));
	}

	s = socket(server->ai_family, server->ai_socktype, server->ai_protocol); //create socket descriptor
	if (s < 0) {
  		die("ERROR - socket(), could not open socket");
	}
	if (bind(s, server->ai_addr, server->ai_addrlen) < 0 ) {
		die("ERROR - bind(), bind failed");
	 }

	//Printing Server IP/Port
	port = ntohs(get_in_port((struct sockaddr *)server->ai_addr));
	printf("Socket ready - IP:%s - Port:%d\n", inet_ntoa(((struct sockaddr_in *)server->ai_addr)->sin_addr), port);
    printf("Waiting for client...\n");

    //Receive request file
	addr_len = sizeof their_addr;
	numbytes = recvfrom(s, buffer, MAXBUFLEN-1, 0, (struct sockaddr *)&their_addr, &addr_len);
	if (numbytes == -1 ){
		perror("ERROR: recvfrom");
	}
	buffer[numbytes] = '\0';

	//Printing Client IP/PORT/Message
	struct sockaddr_in *sin = (struct sockaddr_in *)&their_addr;
	unsigned char *ip = (unsigned char *)&sin->sin_addr.s_addr;
	printf ("File name received:%d bytes | IP:%d %d %d %d /%d | File:%s\n", numbytes, ip[0], ip[1], ip[2], ip[3], sin->sin_port, buffer);

	//Concatenate path and open file
	strcat(path, buffer);
	printf("Opening file... %s\n", path);
	fp = fopen(path, "rb");

	//File does not exist
	if (fp == NULL) { 
	    fprintf(stderr,"ERROR - Failed to open file: %s\n", path);
	    if (sendto(s, "200 - File not found", strlen("200 - File not found"), 0, (struct sockaddr*) &their_addr, addr_len) == -1){
			perror("ERROR - sendto() coult not open file\n");
   		}	
   		close(s);
   		exit(1);    
	}

	//Send ack to client file exist
	printf("File succesfully opened\n");
    if (sendto(s, "100 - File found", strlen("100 - File found"), 0, (struct sockaddr*) &their_addr, addr_len) == -1){
        perror("ERROR: sendto()");
    }
    usleep(100000);
    while(1){
	   	nread = fread(buffer, MAXBUFLEN,sizeof(char),fp);
        // Still data to send
        if (strlen(buffer) > 0){ 
        	numbytes = sendto(s,buffer, strlen(buffer), 0,(struct sockaddr*)&their_addr, addr_len);      		
      		usleep(100000);
        	printf("Sending %d bytes\n", numbytes);
      		if (numbytes < 0 ){
      			perror("Error while sending file\n");
      			break;
      		}
        }
        memset(buffer,'\0',strlen(buffer));

        // Reaching EOF
        if (feof(fp)){
        	numbytes = sendto(s,"300 - EOF", strlen("300 - EOF"),0,(struct sockaddr*)&their_addr,addr_len);
            if (numbytes < 0 ){
      			perror("Error while sending file\n");
      			break;
      		}
      		break;
        }
    }
    printf("Finished transfering file\n");

	close(s);
	return 0;

}