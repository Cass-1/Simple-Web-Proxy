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
/*                              Function Headers                              */
/* -------------------------------------------------------------------------- */
void handle_request_response(int connfd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int my_Open_clientfd(char *hostname, char *port);

/* -------------------------------------------------------------------------- */
/*                                    Main                                    */
/* -------------------------------------------------------------------------- */
int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr; /* Enough space for any address */
  char client_hostname[MAXLINE], client_port[MAXLINE];

  //checks that a port number was given
  if (argc != 2) {
    //fprintf(stderr, "usage: %s <port>\n", argv[0]);
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
    //fprintf(stdout, "before\n");
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

// given a uri (url) splits it into the hostname, pathname and the server port
// if no server port is given in the uri, then the server port is set to 80
// 0 if successful, -1 if failed
int split_uri(int connfd, char* uri, char* hostname, char* pathname, char* server_port){
    char* ref1, ref2;
    char host_and_port[1000];
    char uri2[1000];
    strcpy(uri2, uri);

    // if it is not an http connection
    if(!strstr(uri, "http://")){
      return -1;
    }

    // grab the domain name and the port number if there is one
    char* tok = strtok_r(uri, "//", &ref1);

    tok = strtok_r(ref1, "/", &ref1);
    strcpy(host_and_port, tok);

    // checks if there is a port number
    if(strstr(host_and_port, ":")){
        // gets the domain name
        tok = strtok_r(host_and_port, ":", &ref2);
        strcpy(hostname, tok);
        // gets the port number
        tok = strtok_r(NULL, "/", &ref2);
        strcpy(server_port, tok);
    }
    else{
        // gets the domain name
        tok = strtok_r(host_and_port, "/", &ref2);
        strcpy(hostname, tok);
        // sets the port number
        strcpy(server_port, "80");
    }
    // get the pathname
    char* tok2 = strtok(uri2, "//");
    tok2 = strtok(NULL, "/");
    tok2 = strtok(NULL, "");

    // add the '/' at the beginning
    strcpy(pathname, "/");
    if(tok2 != NULL){
      strcat(pathname, tok2); 
    }
    return 0;
}

// generates the proxy's GET request
void generate_request(int connfd, rio_t rio_request, char* method, char* hostname, char* pathname, char* version, char* server_port, char* generated_request){
  char line_buf[MAXLINE];
  char temp[MAXLINE];

  int host = 0, connection = 0, proxy_connection = 0, get = 0;

  // add the get request
  sprintf(generated_request, "GET %s %s\r\n", pathname, version);

  // if there is nothing to parse in the RIO request
  if(strcmp(rio_request.rio_bufptr, "") == 0){
    sprintf(temp, "Host: %s\r\n", hostname);
    strcat(generated_request, temp);
    strcat(generated_request, "Connection: close\r\n");
    strcat(generated_request, "Proxy-Connection: close\r\n");
    strcat(generated_request, "\r\n");
    return;
  }

  // read the first line of the rio_request
  Rio_readlineb(&rio_request, line_buf, MAXLINE);
  // while not at the end of the request
  while(strcmp(line_buf, "\r\n")) {
    // check the line for headers and replace them with what we want, don't change anything else
    if (strstr(line_buf, "Host:")){
      sprintf(temp, "Host: %s:%s\r\n", hostname, server_port);
      strcat(generated_request, temp);
      host = 1;
    }
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
    // read the next line
    Rio_readlineb(&rio_request, line_buf, MAXLINE);
  }

  // if these fields were not present in the request, add them
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

  // add the terminating sequence to the newly contructed generated_reqest
  strcat(generated_request, "\r\n");
  
}


/*
 * clienterror - returns an error message to the client
 from tiny.c
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Proxy Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>Proxy Server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}

int my_Open_clientfd(char *hostname, char *port) 
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0){
      return -1;
    }
      
    return rc;
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
  rio_t rio_request, rio_response;
  char generated_request[MAX_OBJECT_SIZE];
  int clientfd;
  char server_port[MAXLINE];
  int flag = 0;

  /* ---------------------- Read request line and headers ---------------------*/
  Rio_readinitb(&rio_request, connfd);
  // tries to read from connfd
  if (!Rio_readlineb(&rio_request, buf, MAXLINE)) { 
    return;
  }
  // prints out result
  printf("%s", buf);
  // gets extracts the information read
  sscanf(buf, "%s %s %s", method, uri, version); 
  if (strcasecmp(method, "GET")) { 
    return;
  }

  printf("%s\n", version);
  


  /* --------------------------- Check Http Version --------------------------- */
  // check if the HTTP version is 1.0
  if(strcmp(version, "HTTP/1.0") != 0){
    // set HTTP version
    strncpy(version, "HTTP/1.0", strlen("HTTP/1.0"));
  }

  /* ----------------------------- Parsing the URI ---------------------------- */
  char buf2[MAXLINE];
  // clienterror(connfd, uri, "Got here", "", "");
  //fprintf(stdout, "\nafter\n");
  flag = split_uri(connfd, uri, hostname, pathname, server_port);
  if(flag == -1){
    clienterror(connfd, method, "Bad Request", "The uri was not formatted correctly", "");
    return;
  }

  /* -------------------------- Generate The Request -------------------------- */
  generate_request(connfd, rio_request, method, hostname, pathname, version, server_port, generated_request);
  // clienterror(connfd, method, "TEsting", "", "");
  
  /* ---------------------------- Forward the Request ---------------------------- */

  // connect to the server
  clientfd = my_Open_clientfd(hostname, server_port);
  if (clientfd == -1){
    clienterror(connfd, method, "502", "ERR_CONNECTION_REFUSED", "The connection to the server was refused");
    return;
  }
  
  // forward the request
  Rio_writen(clientfd, generated_request, strlen(generated_request));

  /* ------------------------ Get the Server's Response ----------------------- */
  Rio_readinitb(&rio_response, clientfd);
  // if(rio_response.rio_bufptr)

  char buffer[MAXLINE];
  char responseBuf[MAXLINE];
  ssize_t n;

  // read the servers response line by line
  while( (n = Rio_readnb(&rio_response, responseBuf, MAXLINE)) > 0 ){
    Rio_writen(connfd, responseBuf, n);
    sprintf(buffer, "%s\n", responseBuf);
    Rio_writen(connfd, buffer, strlen(buffer));
  }
  // closes the socket for connecting to hte server
  Close(clientfd);
}
