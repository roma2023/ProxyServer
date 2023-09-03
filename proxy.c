/*
 * Starter code for proxy lab.
 * This code serves as the foundation for implementing a proxy server with caching functionality.
 * Author: Raman Saparkhan rsaparkh@andrew.cmu
 */

/* Some useful includes to help you get started */

#include "csapp.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "http_parser.h"

/*
 * Debug macros, which can be enabled by adding -DDEBUG in the Makefile
 * Use these if you find them useful, or delete them if not
 */
#ifdef DEBUG
#define dbg_assert(...) assert(__VA_ARGS__)
#define dbg_printf(...) fprintf(stderr, __VA_ARGS__)
#else
#define dbg_assert(...)
#define dbg_printf(...)
#endif

/*
 * Max cache and object sizes
 * You might want to move these to the file containing your cache implementation
 */
#define MAX_CACHE_SIZE (1024 * 1024)
#define MAX_OBJECT_SIZE (100 * 1024)

/*
 * String to use for the User-Agent header.
 * Don't forget to terminate with \r\n
 */
static const char *header_user_agent = "Mozilla/5.0"
                                       " (X11; Linux x86_64; rv:3.10.0)"
                                       " Gecko/20230411 Firefox/63.0.1";




/* Typedef for convenience */
typedef struct sockaddr SA;

// Function prototypes
void handle_client(int connfd);
bool forward_request(int serverfd, int connfd, const char *method, const char *uri, const char *version, const char* host, const char* port, rio_t* client_rio, parser_t* parser_ptr);
int parser(char *uri, char *host, int *port, char *path);
void clienterror(int fd, const char *errnum, const char *shortmsg,
                 const char *longmsg);
void *thread(void *vargp);


/**
 * @brief Main function for the proxy server.
 *
 * The `main` function initializes the cache, parses the command-line arguments
 * to get the port number, creates a listening socket, and continuously accepts
 * incoming client connections. For each client connection, a new thread is created
 * to handle the client's request.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return 0 on successful execution.
 */

int main(int argc, char **argv) {
    int listenfd;
    int* connfd;
    socklen_t clientlen;
    
    struct sockaddr_storage clientaddr;
    char host[MAXLINE],port[MAXLINE];

    pthread_t tid;

    ////sio_printf("\n RECEIVED argc: %d and argv: %s\n", argc, *argv);
    // Parse command-line arguments to get the port number
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    // catch the SIGPIPE signal
    signal(SIGPIPE, SIG_IGN);

    // Create a listening socket
    listenfd = open_listenfd(argv[1]);

    while (1) {
        // Accept incoming client connection
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Malloc(sizeof(int));
        *connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);

        getnameinfo((SA*)&clientaddr,clientlen,host,MAXLINE,port,MAXLINE,0);
        sio_printf("Connected to %s %s\n", host, port);

        // Call the thread routine
        pthread_create(&tid,NULL,thread,(void *)connfd);
        
    }

    return 0;
}
/**
 * @brief Thread function to handle client connections.
 *
 * The `thread` function is responsible for handling client connections in a separate thread.
 * It takes a client socket file descriptor as an argument, detaches the thread, and then
 * calls the `handle_client` function to process the client's request. After handling the
 * client request, the connection to the client is closed, and the thread exits.
 *
 * @param vargp A pointer to the client socket file descriptor.
 * @return NULL after handling the client request.
 */
void *thread(void *vargp) {
    int connfd = *(int*)vargp;
    pthread_detach(pthread_self());

    // Handle the client request
    handle_client(connfd);
    
    // Close the connection to the client
    ////sio_printf("\n CONNECTION is closing \n");
    close(connfd);

    return NULL;
}



/**
 * @brief Handles a client request.
 *
 * The `handle_client` function reads the request line from the client, parses it,
 * and checks if the request is well-formed. If the request is a GET method, it checks
 * if the requested URL is in the cache. If it is, the cached content is returned to
 * the client. If not, the function forwards the request to the remote server, reads
 * the response from the server, and forwards it to the client. If the response is
 * smaller than the `MAX_OBJECT_SIZE`, it is also stored in the cache.
 *
 * @param connfd The file descriptor of the client connection.
 */
void handle_client(int connfd) {
    rio_t client_rio;
    rio_t server_rio;
    char buf[MAXLINE];
    int serverfd;

    // Initialize rio buffer for reading from the client
    rio_readinitb(&client_rio, connfd);

    // Read the request line from the client
    if (rio_readlineb(&client_rio, buf, MAXLINE) <= 0) {
        return;
    }
    ////sio_printf("REQUEST HEADER: %s\n", buf);


    /* Parse the request line and check if it's well-formed */
    parser_t *parser_ptr = parser_new();

    parser_state parse_state = parser_parse_line(parser_ptr, buf);

    if (parse_state != REQUEST) {
        parser_free(parser_ptr);
	    clienterror(connfd, "400", "Bad Request",
		    "Tiny received a malformed request");
	    return;
    }
    const char *method, *uri, *version;
    parser_retrieve(parser_ptr, METHOD, &method);
    parser_retrieve(parser_ptr, URI, &uri);
    parser_retrieve(parser_ptr, HTTP_VERSION, &version);

    /* Check that the method is GET */
    if (strcmp(method, "GET") != 0) {
	    parser_free(parser_ptr);
        clienterror(connfd, "501", "Not Implemented","This method is not supported\r\n");
        return;
    }

    // Variables to store the parsed URI components
    const char *path, *port, *host;
    parser_retrieve(parser_ptr, PATH, &path);
    parser_retrieve(parser_ptr, PORT, &port);
    parser_retrieve(parser_ptr, HOST, &host);
    
    // Open a connection to the remote server
    serverfd = open_clientfd(host, port);
    if (serverfd < 0){
        parser_free(parser_ptr);
        //sio_printf("\n*******************WE ENETERED HERE 22222\n");
        clienterror(serverfd, uri, "Connection Failed", "HTTP/1.0 Connection Lost: Try again");
        return;
    }

    // Initialize rio buffer for reading from the client
    rio_readinitb(&server_rio, serverfd);

    // Forward the request to the remote server
    //sio_printf("forwarding request to server with CLIENT FD %d\n", serverfd);
    if (forward_request(serverfd, connfd, method, path, version, host, port ,&client_rio, parser_ptr)){
        parser_free(parser_ptr);
        return;
    }


    // Read the response from the remote server and forward it to the client
    int n;
    while ((n = rio_readnb(&server_rio, buf, MAXLINE)) > 0) {
        //sio_printf("proxy received %d bytes,then send\n",(int)n);
        rio_writen(connfd, buf, n);
    }

    // Close the connection to the remote server
    //sio_printf("\n Client Read is Closing \n");
    close(serverfd);
}
/**
 * @brief Forwards a client request to the remote server.
 *
 * The `forward_request` function forwards a client request to the remote server.
 * It constructs and sends the request headers, including the request line, host,
 * user-agent, connection, proxy-connection, and other headers from the client.
 * If the request contains Request-ID or Response headers, they are included in
 * the forwarded headers. The function reads headers from the client, parses them,
 * and constructs the request headers to be sent to the remote server.
 *
 * @param serverfd The file descriptor of the remote server socket.
 * @param connfd The file descriptor of the client connection.
 * @param method The HTTP method of the request.
 * @param uri The URI of the request.
 * @param version The HTTP version of the request.
 * @param host The host of the request.
 * @param port The port of the request.
 * @param client_rio The client's RIO buffer for reading headers.
 * @param parser_ptr The parser for parsing client headers.
 * @return Returns `true` if an error occurred, otherwise `false`.
 */
bool forward_request(int serverfd, int connfd, const char *method, const char *uri, const char *version, const char* host, const char* port, rio_t *client_rio, parser_t* parser_ptr) {
    char buf[MAXLINE];
    version = "HTTP/1.0";
    char request_line[MAXLINE];
    snprintf(request_line, MAXLINE, "%s %s %s\r\n ", method, uri, version);

    // Send the request line to the remote server
    //printf("Send the request line: %s", request_line);

    // Send the Host header
    char host_header[MAXLINE];
    snprintf(host_header, MAXLINE, "Host: %s:%s\r\n", host, port);
    //printf("Send the Host header: %s", host_header);

    // Send the User-Agent header
    char user_agent_header[MAXLINE];
    snprintf(user_agent_header, MAXLINE, "User-Agent: %s\r\n", header_user_agent);
    //printf("Send the User-Agent header: %s", header_user_agent);

    // Send the Connection header
    char connection_header[MAXLINE];
    snprintf(connection_header, MAXLINE, "Connection: close\r\n");
    //printf("Send the Connection header: %s", connection_header);

    // Send the Proxy-Connection header
    char proxy_connection_header[MAXLINE];
    snprintf(proxy_connection_header, MAXLINE, "Proxy-Connection: close\r\n");
    //printf("Send the proxy Connection header: %s", proxy_connection_header);


    // Send the request-id header
    char request_id_header[MAXLINE];
    char response_header[MAXLINE];

    // Find and include the Request-ID header from the client's request, if present
    int size;
    int rq = 0;
    int rs = 0;

    while ((size = rio_readlineb(client_rio, buf, MAXLINE)) > 0) {
        if (strcmp(buf, "\r\n") == 0){
            //sio_printf("\n ------------SOSOSOSOSOSOSO ----------\n");
            break;
        }
        
        if (!strncasecmp(buf, "Request-ID", strlen("Request-ID"))) {
            // If the line contains Request-ID, include it in the forwarded header
            snprintf(request_id_header, MAXLINE,"%s", buf);
            rq = 1;
        }
        if (!strncasecmp(buf, "Response", strlen("Response"))) {
            // If the line contains Request-ID, include it in the forwarded header
            snprintf(response_header, MAXLINE,"%s", buf);
            rs = 1;
        }
        // if (!strncasecmp(buf, "Accept-Encoding", strlen("Accept-Encoding"))) {
        //     // If the line contains Request-ID, include it in the forwarded header
        //     snprintf(response_header, MAXLINE,"%s", buf);
        // }

        //sio_printf("OTHER HEADERS: %s\n", buf);
        // Parse the request header with parser
	    parser_state parse_state = parser_parse_line(parser_ptr, buf);
        if (parse_state != HEADER) {
	        clienterror(connfd, "400", "Bad Request",
			"Failed to parse");
	        return true;
	    }
    }
    if (size < 0) {
        //sio_printf("\n FIRE------------------ \n");
        return true;
    }


    // Send an empty line to terminate the headers
    char empty_line[] = "\r\n";
    //sio_printf("sending empty line\n");

    char headers[8*MAXLINE];
    if (rq == 1 && rs == 1) sprintf(headers, "%s%s%s%s%s%s%s%s",request_line,host_header,user_agent_header,
    connection_header,proxy_connection_header, request_id_header,response_header,empty_line);  
    else sprintf(headers, "%s%s%s%s%s%s",request_line,host_header,user_agent_header,
    connection_header,proxy_connection_header, empty_line);  

    //sio_printf("\n ------------------\n %s\n -------------\n", headers);
    rio_writen(serverfd, headers, strlen(headers));
    return false;
}

/**
 * @brief Generates and sends an error response to the client.
 *
 * The `clienterror` function generates and sends an error response to the client.
 * It builds the HTTP response body and headers using the provided error number,
 * short message, and long message. It ensures that the generated response does not
 * exceed the maximum buffer size to avoid overflow. The function first constructs
 * the response body and then the headers, writing them to the client socket.
 *
 * @param fd The client socket file descriptor.
 * @param errnum The error number for the response.
 * @param shortmsg The short error message for the response.
 * @param longmsg The long error message for the response.
 */
void clienterror(int fd, const char *errnum, const char *shortmsg,
                 const char *longmsg) {
    char buf[MAXLINE];
    char body[MAXBUF];
    size_t buflen;
    size_t bodylen;

    /* Build the HTTP response body */
    bodylen = snprintf(body, MAXBUF,
            "<!DOCTYPE html>\r\n" \
            "<html>\r\n" \
            "<head><title>Tiny Error</title></head>\r\n" \
            "<body bgcolor=\"ffffff\">\r\n" \
            "<h1>%s: %s</h1>\r\n" \
            "<p>%s</p>\r\n" \
            "<hr /><em>The Tiny Web server</em>\r\n" \
            "</body></html>\r\n", \
            errnum, shortmsg, longmsg);
    if (bodylen >= MAXBUF) {
        return; // Overflow!
    }

    /* Build the HTTP response headers */
    buflen = snprintf(buf, MAXLINE,
            "HTTP/1.0 %s %s\r\n" \
            "Content-Type: text/html\r\n" \
            "Content-Length: %zu\r\n\r\n", \
            errnum, shortmsg, bodylen);
    if (buflen >= MAXLINE) {
        return; // Overflow!
    }

    /* Write the headers */
    if (rio_writen(fd, buf, buflen) < 0) {
        fprintf(stderr, "Error writing error response headers to client\n");
        return;
    }

    /* Write the body */
    if (rio_writen(fd, body, bodylen) < 0) {
        fprintf(stderr, "Error writing error response body to client\n");
        return;
    }
}