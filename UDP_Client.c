//UDP Client wrote by Daniel Olaya, Sept 17

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SEND_BUFF_SIZE 100
#define RECV_BUFF_SIZE 1024
#define SER_IP "176.180.226.0" 
#define FILE_NAME "received.txt"
#define SER_PORT "1212"

// Get port, IPv4 or IPv6:
in_port_t get_in_port(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }
    return (((struct sockaddr_in6*)sa)->sin6_port);
}

int main(int argc, char *argv[]){
    int sock, rv, bytesSent, bytesRecv;
	struct addrinfo hints, *servinfo;
	char sendBuff[SEND_BUFF_SIZE], recvBuff[RECV_BUFF_SIZE]; //need to change buffer[] and MAXBUFF, MAXBUF
	FILE *fp;


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	rv = getaddrinfo("127.0.0.1", SER_PORT, &hints, &servinfo);
	if (rv != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	//Creating socket
	sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (sock == -1){
		perror("Socket: Error while creating socket");
	}
	
	//Sending filename
	strncpy(sendBuff, argv[1], SEND_BUFF_SIZE);
	if ((bytesSent = sendto(sock, sendBuff, strlen(sendBuff)+1, 0, 
			servinfo->ai_addr, servinfo->ai_addrlen)) == -1){
		printf("Sendto: Error sending the file name, %s\n", strerror(errno));
		exit(1);
	}
    printf("File name sent:%s - %d bytes to IP:%s \n", argv[1], bytesSent, inet_ntoa(((struct sockaddr_in *)servinfo->ai_addr)->sin_addr)) ;

	freeaddrinfo(servinfo); 

    //Receive ack message
	struct sockaddr_in addr_in; // connector's address information
	socklen_t addr_len = sizeof addr_in;
    bytesRecv = recvfrom(sock, recvBuff, RECV_BUFF_SIZE-1, 0, (struct sockaddr *)&addr_in, &addr_len);
	recvBuff[bytesRecv] = '\0';

	if (bytesRecv == -1 ){
		perror("ERROR: recvfrom ack message");
	}
	printf ("INFO - Message rcvd:%s, %s\n", inet_ntoa((&addr_in)->sin_addr), recvBuff);
	
	// File found
	if ((strcmp(recvBuff, "100 - File found")) == 0){ 
		

		//Create file to store data
		if ((fp = fopen(FILE_NAME, "w")) == NULL){
			printf("Error creating file");
			exit(1);
		}
		printf("Receving file...\n");
		while (1){
			bytesRecv = recvfrom(sock, recvBuff, RECV_BUFF_SIZE-1,0 ,(struct sockaddr *)&addr_in, &addr_len);
			recvBuff[bytesRecv] = '\0';

			printf ("Message rcvd is %d bytes \n", bytesRecv);


			if (bytesRecv == -1 ){
				perror("ERROR: recvfrom file data\n");
				break;
			}

			if (!strcmp(recvBuff, "300 - EOF")){
				break;
			}
			
			if (fwrite(recvBuff,sizeof(char), bytesRecv,fp)<0){
				printf("Error writting the file\n");
				break;
			}
			memset(recvBuff,'\0',strlen(recvBuff));
		}		
		printf("Finished recieving file %s\n",FILE_NAME);


	// File not found by server
	}else{
		printf("ERROR - File not found, check file name and try again\n");
    	close(sock);
		exit(1);

	}


	fclose(fp);
    close(sock);

    return 0;

}