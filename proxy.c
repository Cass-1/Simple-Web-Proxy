#include "csapp.h"
#include <stdio.h>
#include <string.h>

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
void handle_request_response(int connfd, char* port);

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
    handle_request_response(connfd, argv[1]);

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

// splits the uri into the hostname and the path
void split_uri(int connfd, char* uri, char* hostname, char* pathname){
  char buf2[MAXLINE];
  char* host_and_path = strtok(uri, "//");
  char* host = strtok(NULL, "/");
  char* path = strtok(NULL, "");
  char* slash_path = malloc(MAXLINE);

  // add the '/' to the front of the path
  if(path != NULL){
    strcpy(slash_path, "/");
    strcat(slash_path, path);
  }
  

  // copy the local strings to their external counterparts
  strcpy(hostname, host);
  strcpy(pathname, slash_path);

  free(slash_path);
}

// generates the proxy's GET request
void generate_request(rio_t rio_request, char* method, char* hostname, char* pathname, char* version, char* generated_request){
  char line_buf[MAXLINE];
  char temp[MAXLINE];

  int host = 0, connection = 0, proxy_connection = 0;

  // read through the rio_request from the client
  Rio_readlineb(&rio_request, line_buf, MAXLINE);
  while(strcmp(line_buf, "\r\n")) {
    // check the line for headers
    if (strstr(line_buf, "Host:")){
      sprintf(temp, "Host: %s\r\n", hostname);
      strcat(generated_request, temp);
      host = 1;
    }
    // else if (strstr(line_buf, "User-Agent:")){
    //   strcat(generated_request, );
    // }
    else if (strstr(line_buf, "Connection:")){
      strcat(generated_request, "Connection: close\r\n");
      connection = 1;
    }
    else if (strstr(line_buf, "Proxy-Connection:")) {
      strcat(generated_request, "Proxy-Connection: close\r\n");
      proxy_connection = 1;
    }
    else{
      strcat(generated_request, line_buf);
    }

    Rio_readlineb(&rio_request, line_buf, MAXLINE);
  }

  if(host == 0){
    sprintf(temp, "Host: %s\r\n", hostname);
    strcat(generated_request, temp);
  }

  if(connection == 0){
    strcat(generated_request, "Connection: close\r\n");
  }

  if(proxy_connection == 0){
    strcat(generated_request, "Proxy-Connection: close\r\n");
  }

  strcat(generated_request, "\r\n");
  
}

/* -------------------------------------------------------------------------- */
/*                           handle_request_response                          */
/* -------------------------------------------------------------------------- */

// handle one client/server interaction
void handle_request_response(int connfd, char* port) {
  struct stat sbuf;
  char hostname[MAXLINE/2], pathname[MAXLINE/2];
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio_request, rio_response;
  char generated_request[MAX_OBJECT_SIZE];
  int clientfd;

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
  char buf2[MAXLINE];

  /* ----------------------------- Parsing the URI ---------------------------- */
  split_uri(connfd, uri, hostname, pathname);


  // testing
  // char buffer[MAXLINE];
  // sprintf(buffer, "%s\n", hostname);
  // Rio_writen(connfd, buffer, strlen(buffer));
  // sprintf(buffer, "%s\n", pathname);
  // Rio_writen(connfd, buffer, strlen(buffer));

  /* -------------------------- Generate The Request -------------------------- */
  generate_request(rio_request, method, hostname, pathname, version, generated_request);

  // Rio_writen(connfd, generated_request, strlen(generated_request));

  /* ------------------------- Send Request to Server ------------------------- */
  clientfd = Open_clientfd(hostname, port);
  Rio_readinitb(&rio_response, clientfd);
  Rio_writen(clientfd, generated_request, strlen(generated_request));

  char responseBuf[MAXLINE];
    int n;
    while( (n = Rio_readnb(&rio_response, responseBuf, MAXLINE)) != 0 ){
        Rio_writen(connfd, responseBuf, n);
    }
    Close(clientfd);

  /* --------------------- Send Server Response to Client --------------------- */

}
