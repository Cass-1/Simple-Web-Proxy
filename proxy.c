#include "csapp.h"
#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {


  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  // have the web browser (client) connect to proxy (server)
  proxy_server(argv[1]);

  return 0;
}

// have the web browser connect to the web proxy
void proxy_server(char* port){
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr; /* Enough space for any address */
  char client_hostname[MAXLINE], client_port[MAXLINE];

  // opens a lisening (looking for new conenctions) port to argv[1]
  listenfd = Open_listenfd(port);

  while (1) {
    clientlen = sizeof(struct sockaddr_storage);

    // create a new soccet for communication with a specific client
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    // converts socket address to host and service
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);

    // print the connection
    printf("Connected to (%s, %s)\n", client_hostname, client_port);

    // parse the request sent from the web browser
    // parse_request();    

    // closes the Accepted socket
    Close(connfd);
  }
}
