#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void split_uri(char* uri, char* hostname, char* pathname, char* server_port);

int main(void){
    char host[1000];
    char path[1000];
    char server_port[1000];
    char uri[1000] = "http://www.example.com/data/home.html";

    split_uri(uri, &host, &path, &server_port);
    printf("host: %s\n", host);
    printf("server_port: %s\n", server_port);
    printf("path: %s\n", path);


}

// given a uri (url) splits it into the hostname, pathname and the server port
// if no server port is given in the uri, then the server port is set to 80
void split_uri(char* uri, char* hostname, char* pathname, char* server_port){
    char* ref1, ref2;
    char host_and_port[1000];
    char uri2[1000];
    strcpy(uri2, uri);

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
    strcat(pathname, tok2); 
}