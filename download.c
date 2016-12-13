/*
Downloads a single file
Implements FTP application protocol, as described in RFC959
Adopts URL syntax, as described in RFC1738
	ftp://[<user>:<password>@]<host>/<url-path>
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define STR_MAX 256
#define SERVER_PORT 21
#define MAX_DATA_SIZE 1024

void printUsage();
int receiveData(int sockfd, char *buf);
int sendData(int sockfd, char *msg);
int connectSocket(char *ip, int port);
int saveFile(int sockfd, char *buf);

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printUsage();
		exit(1);
	}
	

	//==================================================================================================
	// Save info
	char user[STR_MAX];
	char password[STR_MAX];
	char host[STR_MAX];
	char path[STR_MAX];
	int no_user = 0; //bool to check if url has user info

	if (sscanf(argv[1], "ftp://%[^:]:%[^@]@%[^/]/%s\n", user, password, host, path) != 4) {
		no_user = 1;
		strcpy(user, "anonymous");
		strcpy(password, "default");

		if (sscanf(argv[1], "ftp://%[^/]/%s\n", host, path) != 2) {
			printUsage();
			exit(2);
		}
	}
	
	if (no_user) {
		printf("Host: %s; Path: %s\n", host, path);
	}
	else {
		printf("User: %s; Password: %s; Host: %s; Path %s\n", user, password, host, path);
	}


	//==================================================================================================
	// Get host IP
	struct hostent *h;
	if ((h=gethostbyname(host)) == NULL) {
		herror("gethostbyname");
		exit(3);
	}

	char *ip = inet_ntoa(*((struct in_addr *) h->h_addr));

	printf("\nHost name: %s\n", h->h_name);
	printf("IP Adress: %s\n", ip);


	//==================================================================================================
	// Connect to server
	int sockfd = connectSocket(ip, SERVER_PORT);

	char buf[MAX_DATA_SIZE];
	receiveData(sockfd, buf);

	printf("\n--Connected to server--\n");



	//==================================================================================================
	// Login
	memset(buf, 0, MAX_DATA_SIZE); // User
	strcpy(buf, "USER ");
	strcat(buf, user);
	sendData(sockfd, buf);
	receiveData(sockfd, buf);

	memset(buf, 0, MAX_DATA_SIZE); // Password
	strcpy(buf, "PASS ");
	strcat(buf, password);
	sendData(sockfd, buf);
	receiveData(sockfd, buf);



	//==================================================================================================
	// Enter passive mode
	memset(buf, 0, MAX_DATA_SIZE);
	strcpy(buf, "PASV ");
	sendData(sockfd, buf);
	receiveData(sockfd, buf);



	//==================================================================================================
	// Init TCP
	int ip1, ip2, ip3, ip4, port1, port2;
	if (sscanf(buf, "%*[^(](%d, %d, %d, %d, %d, %d)\n", &ip1, &ip2, &ip3, &ip4, &port1, &port2) != 6) {
		printf("Failed to read ip from server\n");
		exit(6);
	}

	char new_ip[MAX_DATA_SIZE];
	memset(new_ip, 0, MAX_DATA_SIZE);
	sprintf(new_ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
	int new_port = 256*port1+port2;

	int new_sockfd = connectSocket(new_ip, new_port);

	

	//==================================================================================================
	// Download
	memset(buf, 0, MAX_DATA_SIZE);
	strcpy(buf, "RETR ");
	strcat(buf, path);
	sendData(sockfd, buf);

	receiveData(sockfd, buf);

	saveFile(new_sockfd, path);




	//==================================================================================================
	// Exit
	close(new_sockfd);
	close(sockfd);
	exit(0);
}

void printUsage() {
	printf("Usages:\n");
	printf("   ftp://<host>/<url-path>\n");
	printf("   ftp://[<user>:<password>@]<host>/<url-path>\n");
}

int receiveData(int sockfd, char *buf) {
	memset(buf, 0, MAX_DATA_SIZE);
	recv(sockfd, buf, MAX_DATA_SIZE, 0);
	printf("Received: %s", buf);

	return 0;	
}

int sendData(int sockfd, char *buf) {
	strcat(buf, "\r\n");
	send(sockfd, buf, strlen(buf), 0);
	printf("\nSent: %s", buf);

	return 0;
}

int connectSocket(char *ip, int port) {
	int sockfd;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { // Open TCP socket
		perror("socket()");
		exit(4);
	}

	struct sockaddr_in server_addr;
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) { // Connect to server
		perror("connect()");
		exit(5);
	}

	return sockfd;
}

int saveFile(int sockfd, char *path) {
	// Get file name
	char name[STR_MAX];
	char *tmp = strtok(path, "/");
	
	while(tmp != NULL) {
		strcpy(name, tmp);
		tmp = strtok(NULL, "/");
	}

	
	int file;
	if ((file = open(name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1) {
		printf("Failed to open file\n");
		exit(7);
	}
	

	printf("\nFile %s created\n", name);
	int n;
	char buf[MAX_DATA_SIZE];
	while((n = recv(sockfd, buf, MAX_DATA_SIZE, 0)) != 0) {
		write(file, buf, n);
	}

	printf("\nFinished writing to file\n");
	close(file);
	return 0;
}


