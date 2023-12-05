#include "csapp.h"
#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* -------------------------------------------------------------------------- */
/*                            Function Declarations                           */
/* -------------------------------------------------------------------------- */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void read_requesthdrs(rio_t *rp);
void handle_request_response(int connfd);

/* -------------------------------------------------------------------------- */
/*                                    main                                    */
/* -------------------------------------------------------------------------- */
int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr; /* Enough space for any address */
  char client_hostname[MAXLINE], client_port[MAXLINE];

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  // opens a lisening (looking for new conenctions) port to argv[1]
  listenfd = Open_listenfd(argv[1]);

  /* ------ loop to constantly search for connections and handle requests -----
   */
  while (1) {
    clientlen = sizeof(struct sockaddr_storage);

    // create a new soccet for communication with a specific client
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    // converts socket address to host and service
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,client_port, MAXLINE, 0);

    // print the connection
    printf("Connected to (%s, %s)\n", client_hostname, client_port);

    // handle one client/server itneraction
    handle_request_response(connfd);

    // closes the Accepted socket
    printf("closes");
    Close(connfd);
  }

  return 0;
}

/* -------------------------------------------------------------------------- */
/*                  handle_request_response helper functions                  */
/* -------------------------------------------------------------------------- */

// returns an error message to hte client
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE];

  /* Print the HTTP response headers */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* Print the HTTP response body */
  sprintf(buf, "<html><title>Tiny Error</title>");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<body bgcolor="
               "ffffff"
               ">\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
  Rio_writen(fd, buf, strlen(buf));
}

// reads the request header
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while (strcmp(buf, "\r\n")) { // line:netp:readhdrs:checkterm
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

void split_uri(int connfd, char* uri, char* hostname, char* pathname){
  char buf2[MAXLINE];
  char* host_and_path = strtok(uri, "//");
  char* host = strtok(NULL, "/");
  char* path = strtok(NULL, "");

  strcpy(hostname, host);
  strcpy(pathname, path);

}

/* -------------------------------------------------------------------------- */
/*                           handle_request_response                          */
/* -------------------------------------------------------------------------- */

// handle one client/server interaction
void handle_request_response(int connfd) {
  struct stat sbuf;
  char hostname[MAXLINE/2], pathname[MAXLINE/2];
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio_request;

  /* ---------------------- Read request line and headers ---------------------*/
  Rio_readinitb(&rio_request, connfd);
  if (!Rio_readlineb(&rio_request, buf, MAXLINE)) { // line:netp:doit:readrequest
    return;
  }
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // line:netp:doit:parserequest
  if (strcasecmp(method, "GET")) { // line:netp:doit:beginrequesterr
    clienterror(connfd, method, "501", "Not Implemented","Non GET methods are not implemented");
    return;
  }

  printf("%s", version);


  /* --------------------------- Check Http Version --------------------------- */
  if(strcmp(version, "HTTP/1.0") != 0){
    // print error message
    clienterror(connfd, method, "501", "Note Implemented", "Only HTTP/1.0 is Implemented");

    // set HTTP version
    strncpy(version, "HTTP/1.0", strlen("HTTP/1.0"));
  }

  /* ----------------------------- Parsing the URI ---------------------------- */
  split_uri(connfd, uri, hostname, pathname);
  char buf2[MAXLINE];
  sprintf(buf2, "host: %s", hostname);
  Rio_writen(connfd, buf2, strlen(buf2));
  sprintf(buf2, "\npath: %s", pathname);
  Rio_writen(connfd, buf2, strlen(buf2));

  /* -------------------------- Generate The Request -------------------------- */
  // generate_request(rio_request, method, hostname, pathname, version);


}