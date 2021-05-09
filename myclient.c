#include <stdio.h>
#include <stdlib.h>
#include<unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>

char nodes_info[1000];
int getNodesInfoFromServer(int sockfd);
int connectAndFetchFromNode(char *address, char* port, char *searchFile);// storing all node info

void error(char *msg) {
	perror(msg);
	exit(1);
}

bool startsWith(const char *prefix, const char *str) {
	size_t lenpre = strlen(prefix), lenstr = strlen(str);
	return lenstr < lenpre ? false : memcmp(prefix, str, lenpre) == 0;
}


int main(int argc, char *argv[]) {
	if (argc != 3) {
		error("Please specify server IP and server port number \n");
	}

	struct sockaddr_in server_address;
	int clientfd;
	clientfd = socket(AF_INET, SOCK_STREAM, 0);
	if (clientfd < 0) {
		error("Error in opening socket\n");
	}

	int portno;
	portno = atoi(argv[2]);

	struct hostent *server;
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		error("Error in creating socket\n");
	}

	memset((char*) &server_address, '\0', sizeof(server_address));
	server_address.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
	server_address.sin_port = htons(portno);

	if (connect(clientfd, (struct sockaddr*) &server_address,
			sizeof(server_address)) < 0) {
		error("Error connecting");
	}

	printf("Connecting to the server...\n");
	char *request = "Request type: client";
	int n = write(clientfd, request, strlen(request));
	if (n < 0) {
		error("Error writing to socket");
	}

	memset(nodes_info, '\0', 1000);
	n = read(clientfd, nodes_info, 999);
	if (n < 0) {
		error("Error reading from socket");
	}
	printf("Response (Nodes info) from server = %s\n", nodes_info);		// storing nodes info

	n = shutdown(clientfd, 0);
	if (n < 0) {
		printf("Error shutting down the connection");
	}

	n = getNodesInfoFromServer(clientfd);
	if (n < 0) {
		error("Nodes info not found");
	}
	return 0;
}

int getNodesInfoFromServer(int sockfd) {
	char searchFile[50];
	printf("Enter the File name you are looking for : ");
	scanf("%s", searchFile);

	int n = 0;
	char *rest = nodes_info;
	char *token = "\0";
	char *token2 = "\0";

	// for only 3 nodes -- TODO extend functionality
	char* ip1 = "\0";
	char* port1 = "\0";
	char* ip2 = "\0";
	char* port2 = "\0";
	char* ip3 = "\0";
	char* port3 = "\0";

	// parse ips and ports
	while (token = strtok_r(rest, "\n", &rest)) {
		char *rest2 = token;
		while (token2 = strtok_r(rest2, " ", &rest2)) {
			if (ip1[0] == '\0') ip1 = token2;
			else if (port1[0] == '\0') port1 = token2;
			else if (ip2[0] == '\0') ip2 = token2;
			else if (port2[0] == '\0') port2 = token2;
			else if (ip3[0] == '\0') ip3 = token2;
			else if (port3[0] == '\0') port3 = token2;
		}
	}

	printf("Connecting to the node 1 = %s:%d...\n", ip1, atoi(port1));
	n = connectAndFetchFromNode(ip1, port1, searchFile);
	if (n == 0) {
		printf("File fetched Successfully from Node 1\n");
		goto end;
	}
	printf("Connecting to the node 2 = %s:%d...\n", ip2, atoi(port2));
	n = connectAndFetchFromNode(ip2, port2, searchFile);
	if (n == 0) {
		printf("File fetched Successfully from Node 2\n");
		goto end;
	}
	printf("Connecting to the node 3 = %s:%d...\n", ip3, atoi(port3));
	n = connectAndFetchFromNode(ip3, port3, searchFile);
	if (n == 0) {
		printf("File fetched Successfully from Node 3\n");
		goto end;
	}

	printf("Warning: The File is not present in any node!\n");
	return 0;

	end: printf("File fetched Successfully!\n");
	return 0;
}

int connectAndFetchFromNode(char *address, char* port, char *searchFile) {
	int portno = atoi(port);
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *node;
	struct in_addr ipv4;
	char buffer[1000];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("Error opening socket\n");

	inet_pton(AF_INET, address, &ipv4);
	node = gethostbyaddr(&ipv4, sizeof ipv4, AF_INET);

	if (node == NULL)
		error("Node not found\n");

	memset((char*) &serv_addr, '\0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*) node->h_addr, (char *)&serv_addr.sin_addr.s_addr, node->h_length);
	serv_addr.sin_port = htons(portno);

	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
		error("Error connecting to node");
	else
		printf("Connection to the node is successful\n");

	printf("Writing to output stream to node = %s\n", searchFile);
	n = write(sockfd, searchFile, strlen(searchFile));
	if (n < 0) {
		error("Error trying to write to socket\n");
	}

	memset(buffer, '\0', 1000);
	n = read(sockfd, buffer, 999);
	if (n < 0) {
		error("Error reading from socket\n");
	}
	printf("Received the reply : %s\n", buffer);

	if (strcmp(buffer, "NOTFOUND") == 0) {
		printf("Closing connection\n");
		n = shutdown(sockfd, 0);
		if (n < 0) {
			error("Error closing the connection\n");
                       
		}
		return -1;

	} else if (strcmp(buffer, "FOUND") == 0) {
		printf("Success: File found in the connected node\n");

		n = read(sockfd, buffer, 999);		// reading again for file content
		if (n < 0)
			error("Error reading from socket");
		printf("File has the following content = \n%s", buffer);

		n = shutdown(sockfd, 0);
		if (n < 0)
			printf("Error in connection close\n");
		return 0;

	} else {
		printf("Unknown reply from node\n");
		
		return -1;
	}
}

