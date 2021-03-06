#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <string>
#include <iostream>

#define BUFSIZE 1024

using namespace std;

int main(int argc, char * argv[]) {

	char * server_name = NULL;
	int server_port = -1;
	char * server_path = NULL;
	char * req = NULL;
	bool ok = false;

	char  buf[BUFSIZE];
	int socket_fd;
	int ret;

	struct hostent* remote_host;
	struct sockaddr_in socket_addr;
	fd_set fds;

	string response = "";
	string header = "";
	string status;
	string::size_type pos;


	/*parse args */
	if (argc != 5) {
		fprintf(stderr, "usage: http_client k|u server port path\n");
		exit(-1);
	}

	server_name = argv[2];
	server_port = atoi(argv[3]);
	server_path = argv[4];

	req = (char *)malloc(strlen("GET  HTTP/1.0\r\n\r\n")
		+ strlen(server_path) + 1);

	/* initialize */
	if (toupper(*(argv[1])) == 'K') {
		minet_init(MINET_KERNEL);
	}
	else if (toupper(*(argv[1])) == 'U') {
		minet_init(MINET_USER);
	}
	else {
		fprintf(stderr, "First argument must be k or u\n");
		exit(-1);
	}

	/* make socket */
	socket_fd = minet_socket(SOCK_STREAM);

	if (socket_fd < 0) {
		fprintf(stderr, "Error creating socket.\n");
		free(req);
		exit(-1);
	}

	/* get host IP address  */
	/* Hint: use gethostbyname() */
	remote_host = gethostbyname(server_name);

	if (remote_host == NULL) {
		fprintf(stderr, "Error getting host IP.\n");
		free(req);
		exit(-1);
	}

	/* set address */
	socket_addr.sin_family = AF_INET;
	memcpy(&socket_addr.sin_addr.s_addr, remote_host->h_addr, remote_host->h_length);
	socket_addr.sin_port = htons(server_port);

	/* connect to the server socket */
	if (minet_connect(socket_fd, &socket_addr) < 0) {
		fprintf(stderr, "Error connecting to socket.\n");
		free(req);
		exit(-1);
	}

	/* send request message */
	sprintf(req, "GET %s HTTP/1.0\r\n\r\n", server_path);				//Need to use sprintf since we are writing to req (char*)
	if (minet_write(socket_fd, req, strlen(req)) < 0) {
		fprintf(stderr, "Error sending request message.\n");
		free(req);
		exit(-1);
	}

	/* wait till socket can be read. */
	/* Hint: use select(), and ignore timeout for now. */
	FD_ZERO(&fds);														//Initialize the fds to null set
	FD_SET(socket_fd, &fds);											//Add the socket fd to the fds

	if (minet_select(socket_fd + 1, &fds, NULL, NULL, NULL) < 0) {
		fprintf(stderr, "Error selecting socket.\n");
		free(req);
		minet_close(socket_fd);
		minet_deinit();
		exit(-1);
	}

	/* first read loop -- read headers */
	ret = minet_read(socket_fd, buf, BUFSIZE - 1);

	while (ret > 0) {
		buf[ret] = '\0';
		response += string(buf);
		pos = response.find("\r\n\r\n", 0);

		if (pos != string::npos) {
			header = response.substr(0, pos);
			response = response.substr(pos + 4);
			break;
		}

		ret = minet_read(socket_fd, buf, BUFSIZE - 1);

	}
	/* examine return code */
	status = header.substr(header.find(" ") + 1);
	status = status.substr(0, status.find(" "));

	//Skip "HTTP/1.0"
	//remove the '\0'

	// Normal reply has return code 200
	/* print first part of response: header, error code, etc. */
	if (status == "200") {
		ok = true;
		cout << response;
	}
	else {
		ok = false;
		cout << header + "\r\n\r\n" + response;
	}

	/* second read loop -- print out the rest of the response: real web content */
	while ((ret = minet_read(socket_fd, buf, BUFSIZE - 1)) > 0) {
		buf[ret] = '\0';
		if (ok) {
			printf("%s", buf);
		}
		else {
			fprintf(stderr, buf);
		}
	}

	/*close socket and deinitialize */
	minet_close(socket_fd);
	minet_deinit();
	free(req);

	if (ok) {
		return 0;
	}
	else {
		return -1;
	}
}
