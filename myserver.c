


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h> /*Required for Internet address family */
#include<arpa/inet.h>
#include <string.h>
#define backlog 5

void error(char *msg) {
	perror(msg);
	exit(1);
}

char nodes_info[1000] = "\0";

int main(int argc, char *argv[]) {
	if (argc != 2) {
		error("Please specify port number as an argument\n");
	}

	int port_no = atoi(argv[1]);
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("Error in opening socket\n");
	}

	struct sockaddr_in server_address;
	memset(&server_address, '\0', sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port_no);

	int binding = bind(sockfd, (struct sockaddr*) &server_address,
			sizeof(server_address));
	if (binding < 0) {
		error("Error in binding\n");                
	}
	printf("Bind successful at port %d\n", port_no);


	struct sockaddr_in new_addr;
	int serversocket;
	int newlen = sizeof(new_addr);

	while (1) {
		listen(sockfd, backlog);
		printf("Listening started on server\n");
		printf("Before accept = %d %s %d\n", sockfd, (struct sockaddr*) &new_addr, newlen);

		serversocket = accept(sockfd, (struct sockaddr*) &new_addr, &newlen);
		if (serversocket < 0) {
			error("Error in accept");
		}

		int port = ntohs(new_addr.sin_port) + 100;
		char buff[256];

		memset(buff, '\0', sizeof(buff));
		int n = read(serversocket, buff, 255);
		if (n < 0) {
			error("Error in reading socket\n");
		}
		printf("Message from node/client = %s\n", buff);

		if (strcmp(buff, "Request type: node") == 0) {
			printf("Node port = %d\n", port);
			char clientIp[INET_ADDRSTRLEN];
			if (inet_ntop(AF_INET, &new_addr.sin_addr.s_addr, clientIp,
					sizeof(clientIp)) != NULL) {
				// store client IP and port
				strcat(nodes_info, clientIp);
				strcat(nodes_info, " ");
				char s1[4];							// only ports with length 4
				sprintf(s1, "%d", port);
				strcat(nodes_info, s1);
				strcat(nodes_info, "\n");
				printf("stored nodes_info =  %s", nodes_info);

			} else {
				printf("Error in getting address\n");
			}

			char *resp = "Response : node,";
			char buff2[50];
			sprintf(buff2, "%s %d", resp, port);
			printf("Sending the message = %s\n", buff2);
			n = write(serversocket, buff2, strlen(buff2));
			if (n < 0) {
				error("Error writing into socket");
			}

		} else if (strcmp(buff, "Request type: client") == 0) {
			printf("Received client request\n");
			printf("Server has the following nodes info: %s\n", nodes_info);
			// send stored nodes info to client
			n = write(serversocket, nodes_info, strlen(nodes_info));
			if (n < 0) {
				error("Error writing nodes info into socket\n");
			}

		} else {
			printf("Unknown Request format = NOP\n");
		}

		close(serversocket);
	}
}

