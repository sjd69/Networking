#include "minet_socket.h"
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100
#define CONNECTION_INIT 16;

int handle_connection(int sock);
void setConnections(fd_set* set, int* connections, int* count);
void addConnection(int connect, int** connections, int* count, int* max);
void removeConnection(int connect, int* connections, int* count);

int main(int argc, char * argv[]) {
    int server_port = -1;
    int rc          =  0;
    int sock        = -1;
    int socketMax   =  0;
    struct sockaddr_in my_addr;
    struct sockaddr_in clientaddr;
    int i;
    int* connections;
    int connectionCount = 0;
    int connectionsMax = CONNECTION_INIT;
    socklen_t addrlen;
    fd_set set;
    char buffer[BUFSIZE];

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

    if (argv[1][0] != 'k' && argv[1][0] != 'u') {
        fprintf(stderr, "usage: http_server1 k|u port\n");
        exit(-1);
    }

    /* initialize and make socket */

    if (argv[1][0] == 'k')
        minet_init(MINET_KERNEL);
    else
        minet_init(MINET_USER);


    sock = minet_socket(SOCK_STREAM);
    if (sock < 0) {
        fprintf(stderr, "Unable to initialize socket.\n");
        exit(-1);
    }

    /* set server address*/

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(server_port);     // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

    /* bind listening socket */

    minet_bind(sock, &my_addr);

    /* start listening */

    minet_listen(sock, 20);

    /* connection handling loop: wait to accept connection */
    connections = malloc(sizeof int * CONNECTIONS_INIT);

    while (1) {

	    /* create read list */

        FD_ZERO(set);
        FD_SET(sock);
        setConnections(&set, connections, *connectionsCount);

	    /* do a select */

        int ready = minet_select(socketMax, &set, NULL, &aSet, NULL);

	    /* process sockets that are ready */

	    /* for the accept socket, add accepted connection to connections */

        if (FD_ISSET(sock, &set)) {
            int connect = minet_accept(sock, &clientaddr);
            if (connect >= socketMax)
                socketMax = connect + 1;
            addConnection(connect, connections, *connectionsCount, *connectionsMax);
        }

	    /* for a connection socket, handle the connection */
        for (i = 0; i < connectionsCount; i++) {
            if (FD_ISSET(connections[i], &set)) {
	            rc = handle_connection(sock, buffer);
                removeConnection(connections[i], connections, &connectionsCount);
            }
        }

    }
    free(connections);
}

int handle_connection(int sock) {
    bool ok = false;

    const char * ok_response_f = "HTTP/1.0 200 OK\r\n"	\
	"Content-type: text/plain\r\n"			\
	"Content-length: %d \r\n\r\n";

    const char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"	\
	"Content-type: text/html\r\n\r\n"			\
	"<html><body bgColor=black text=white>\n"		\
	"<h2>404 FILE NOT FOUND</h2>\n"				\
	"</body></html>\n";

    /* first read loop -- get request and headers*/

    int in = minet_read(sock, buffer, BUFSIZE);
    fprintf(stderr, "Diagnostic Output:\r\n%s\r\n", buffer);

    /* parse request to get file name */
    /* Assumption: this is a GET request and filename contains no spaces*/

    int start = 4, end, size;
    char* path;
    for (end = start; buffer[end] != ' '; end++);
    path = buffer + 4;
    size = end - start;
    path[size] = '\0';

    /* try opening the file */

    int file = open(path, size, O_RDONLY);
    ok = file != -1;

    /* send response */
    if (ok) {
	    /* send headers */
        minet_write(sock, (char*) ok_response_f, 66);
	    /* send file */
        size = read(file, buffer, BUFSIZE);
        while (size > 0) {
            minet_write(sock, buffer, size);
            size = read(file, buffer, BUFSIZE);
        }
    } else {
	    // send error response
        minet_write(sock,(char*) notok_response, 137);
    }

    /* close socket and free space */
    minet_close(sock);

    if (ok) {
	return 0;
    } else {
	return -1;
    }
}

void setConnections(fd_set* set, int* connections, int* max) {
    int i;
    for (i = 0; i < max; i++)
        FD_SET(connections[i], set);
}

void addConnection(int connect, int** connections, int* count, int* max) {
    if (*count == *max) {
        *max = 2 * *max;
        *connections = realloc(*connections, *max * sizeof int);
    }

    *(*connections + *count) = connect;
    *count = *count + 1;
}

void removeConnection(int connect, int* connections, int* count) {
    int i;
    for (i = 0; connections[i] != connect; i++);
    for (i++, i < *count; i++)
        connection[i - 1] = connections[i];
    *count = *count - 1;
}
