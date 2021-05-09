#include <stdio.h>
#include <stdlib.h>
#include<unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#include <string.h>
#include <stdbool.h>
#define backlog 5

void error(char *msg) {
	perror(msg);
	exit(0);
}

// convenience function to test if a string starts with another string
bool startsWith(const char *prefix, const char *str) {
	size_t lenpre = strlen(prefix), lenstr = strlen(str);
	return lenstr < lenpre ? false : memcmp(prefix, str, lenpre) == 0;
}
void node_server(char *);

int main(int argc, char *argv[]) {
	if (argc != 3) {
		error("Please specify server IP and server port number \n");
	}

	int port = atoi(argv[2]);
	int nodefd = socket(AF_INET, SOCK_STREAM, 0);
	if (nodefd < 0) {
		error("Error in opening socket\n");
	}

	struct hostent *server;
	server = gethostbyname(argv[1]);
	struct sockaddr_in server_address;

	memset(&server_address, '\0', sizeof(server_address));
	server_address.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
	server_address.sin_port = htons(port);

	if (connect(nodefd, (struct sockaddr*) &server_address,
			sizeof(server_address)) < 0) {
		error("Error in connecting to server\n");
	}

	printf("Sending request message\n");
	char *request = "Request type: node";
	int n = write(nodefd, request, strlen(request));
	if (n < 0) {
		error("Error writing into socket\n");
	}

	char buff[256];
	memset(buff, '\0', 256);
	n = read(nodefd, buff, 255);
	if (n < 0) {
		error("Error reading socket\n");
	}
	printf("%s\n", buff);

	if (startsWith("Response : node,", buff)) {
		printf("Node is accepted\n");
		printf("Connection is closed\n");
		n = shutdown(nodefd, 0);
		printf("client port no is %s\n", &buff[17]);
		node_server(&buff[17]);
	} else
		printf("Not Accepted..Try Again");
}

// starts a node server that searches and serves file content to clients
void node_server(char *port) {
	printf("Starting node server...");
	int nodefd, clientfd, portno;
	socklen_t clintLength;
	char buffer[256];
	struct sockaddr_in nodeServer, clientAddress;
	int n;

	nodefd = socket(AF_INET, SOCK_STREAM, 0);
	if (nodefd < 0)
		error("Error opening socket");

	memset(&nodeServer, '\0', sizeof(nodeServer));
	portno = atoi(port);
	nodeServer.sin_family = AF_INET;
	nodeServer.sin_addr.s_addr = INADDR_ANY;
	nodeServer.sin_port = htons(portno);

	int binding = bind(nodefd, (struct sockaddr*) &nodeServer,
			sizeof(nodeServer));
	if (binding < 0) {
		error("Error in binding\n");
	}

	listen(nodefd, backlog);
	printf("Node started listening... \n");
	clintLength = sizeof(clientAddress);
	clientfd = accept(nodefd, (struct sockaddr*) &clientAddress, &clintLength);
	if (clientfd < 0)
		error("Error in accept");
	close(nodefd);

	memset(buffer, '\0', sizeof(buffer));
	n = read(clientfd, buffer, 255);
	if (n < 0) {
		error("Error reading from client");
	} else
		printf("Message from client: %s\n", buffer);


			printf("Received request for the file : %s\n", buffer);
			FILE *fp = fopen(buffer, "r");

			if (fp == NULL) {
				printf("Requested file not found in this node\n");
				char response[] = "NOTFOUND";
				n = write(clientfd, response, strlen(response));
				if (n < 0)
					error("Error writing to socket");

			} else {
				printf("Found file in this node\n");
				char response[] = "FOUND";
				n = write(clientfd, response, strlen(response));
				if (n < 0)
					error("Error writing to socket");

				// read content of found file
				fseek(fp, 0, SEEK_END);
				long fsize = ftell(fp);
				fseek(fp, 0, SEEK_SET);

				char *string = malloc(fsize + 1);
				fread(string, fsize, 1, fp);
				fclose(fp);
				printf("file has the following content:\n%s", string);

				n = write(clientfd, string, strlen(string));
				if (n < 0) {
					error("Error writing to socket");
				}
			}


}
