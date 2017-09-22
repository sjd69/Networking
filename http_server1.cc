#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100

int handle_connection(int sock);

int main(int argc, char * argv[]) {
    int server_port = -1;
    int rc          =  0;
    int sock        = -1;
    int connect     = -1;
    struct addrinfo hints, *res;
    struct sockaddr clientaddr;
    socklen_t addrlen;

    /* parse command line args */
    if (argc != 3) {
	fprintf(stderr, "usage: http_server1 k|u port\n");
	exit(-1);
    }

    server_port = atoi(argv[2]);

    if (server_port < 1500) {
	fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", server_port);
	exit(-1);
    }

    /* get address info */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    minet_getaddrinfo(NULL, server_port, &hints, &res);

    /* initialize and make socket */

    sock = minet_socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        fprintf(stderr, "Unable to initialize socket.");
        exit(-1);
    }

    /* set server address*/
    /* bind listening socket */

    minet_bind(sock, res->ai_addr, res->addr_len);

    /* start listening */

    minet_listen(sock, 20);

    /* connection handling loop: wait to accept connection */

    while (1) {
	    /* handle connections */
        connect = minet_accept(sock);
        if (connect < 0)
            continue;

        fprintf(stderr, "Connected\n");
        //rc = handle_connection(sock);
        
        minet_close(connect);
    }
}

int handle_connection(int sock) {
    bool ok = false;

    const char * ok_response_f = "HTTP/1.0 200 OK\r\n"	\
	"Content-type: text/plain\r\n"			\
	"Content-length: %d \r\n\r\n";

    const char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"	\
	"Content-type: text/html\r\n\r\n"			\
	"<html><body bgColor=black text=white>\n"		\
	"<h2>404 FILE NOT FOUND</h2>\n"
	"</body></html>\n";

    /* first read loop -- get request and headers*/

    /* parse request to get file name */
    /* Assumption: this is a GET request and filename contains no spaces*/

    /* try opening the file */

    /* send response */
    if (ok) {
	/* send headers */

	/* send file */

    } else {
	// send error response
    }

    /* close socket and free space */

    if (ok) {
	return 0;
    } else {
	return -1;
    }
}
