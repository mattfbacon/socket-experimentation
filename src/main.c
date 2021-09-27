#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>

static char const*const SOCKET_NAME = "socket";
#define MAX_CONNS 100

static socklen_t /* const */ SOCKET_SIZE = sizeof(struct sockaddr_un);

#define DATA_SIZE 1000
static char data[DATA_SIZE];

int main(int const argc, char const*const argv[]) {
	(void)argc;
	(void)argv;

	int const sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Making socket connection");
		exit(EXIT_FAILURE);
	}
	struct sockaddr_un local;
	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, SOCKET_NAME);
	if (unlink(SOCKET_NAME) == -1) {
		if (errno != ENOENT) {
			perror("Unlinking socket");
			exit(EXIT_FAILURE);
		}
	}
	if (bind(sockfd, (struct sockaddr*)&local, strlen(SOCKET_NAME) + sizeof(local.sun_family)) == -1) {
		perror("Binding socket");
		exit(EXIT_FAILURE);
	}
	if (listen(sockfd, MAX_CONNS) == -1) {
		perror("Listening to socket");
		exit(EXIT_FAILURE);
	}
	// socket connection loop
	for (;;) {
		struct sockaddr_un remote;
		int remote_sockfd = accept(sockfd, (struct sockaddr*)&remote, &SOCKET_SIZE);
		if (remote_sockfd == -1) {
			perror("Accpting socket connection");
			exit(EXIT_FAILURE);
		}
		// data read/write loop
		for (;;) {
			int const bytes_read = recv(remote_sockfd, data, DATA_SIZE, 0);
			if (bytes_read <= 0) {
				if (bytes_read < 0) { perror("Reading from socket"); }
				break;
			}
			// ROT13
			for (int i = 0; i < bytes_read; i++) {
				if (data[i] < 'A') {
					continue;
				} else if (data[i] <= 'Z') {
					data[i] = ((data[i] - 'A' + 13) % 26) + 'A';
				} else if (data[i] <= 'z') {
					data[i] = ((data[i] - 'a' + 13) % 26) + 'a';
				} else {
					continue;
				}
			}
			if (send(remote_sockfd, data, DATA_SIZE, 0) == -1) {
				perror("Sending to socket");
				break;
			}
		}
		close(remote_sockfd);
	}

	return EXIT_SUCCESS;
}
