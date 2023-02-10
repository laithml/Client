#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define USAGE  printf("Usage: client [-p n <text>] [-r n < pr1=value1 pr2=value2 ...>] <URL>\n")


int UrlToString(char *, int, char **, int *, char **);

char *ArgsToString(char **, int);

int socket_connect(char *, int);

void Request(int, char *);

void freeAllocated(char *args,char *host,char *path,char **arg,char *par){
    // Free allocated memory
    if(args!=NULL){
        if(strcmp(args,"")!= 0)
            free(args);
    }
    if(host!=NULL)
        free(host);
    if(path!=NULL)
        free(path);
    if(arg!=NULL)
        free(arg);
    if(par!=NULL)
        free(par);

}
int main(int argc, char *argv[]) {
    // Initialize variables
    int i = 1, request_size = 0;
    char *par = NULL, **arg = NULL;
    int parLen = 0, argLen = 0;
    char *Url;

    // Parse command line arguments
    while (i < argc) {
        request_size += (int)strlen(argv[i]);
        if (argv[i][0] == '-') {
            int n = atoi(argv[i + 1]);
            if (n == 0 && argv[i + 1][0] != '0') {
                USAGE;
                freeAllocated(NULL,NULL,NULL,arg,par);
                exit(EXIT_FAILURE);
            }
            switch (argv[i][1]) {
                case 'p':
                    if(strlen(argv[i + 2])!=n||n==0) {
                        USAGE;
                        freeAllocated(NULL,NULL,NULL,arg,par);
                        exit(EXIT_FAILURE);
                    }
                    // Allocate memory for POST request body
                    par = calloc(sizeof(char) , n + 1);
                    if (par == NULL) {
                        fprintf(stderr, "calloc failed\n");
                        exit(EXIT_FAILURE);
                    }
                    // Copy POST request body from command line argument
                    strncpy(par, argv[i + 2], strlen(argv[i + 2]));
                    parLen = n;
                    request_size  += n;
                    i = i + 2;
                    break;
                case 'r':
                    // Allocate memory for GET request query string arguments
                    arg = (char **) calloc(sizeof(char *) , n);
                    if (arg == NULL) {
                        fprintf(stderr, "calloc failed\n");
                        freeAllocated(NULL,NULL,NULL,arg,par);
                        exit(EXIT_FAILURE);
                    }
                    // Copy GET request query string arguments from command line arguments
                    for (int j = 0; j < n; j++) {
                        arg[j] = argv[i + j + 2];
                    }
                    request_size += n;
                    argLen = n;
                    i = i + n + 1;
                    break;
            }
        } else {
            // Store URL from command line argument
            Url = argv[i];
        }
        i++;
    }

    // Check that URL was provided
    if(Url == NULL){
        USAGE;
        freeAllocated(NULL,NULL,NULL,arg,par);

        exit(EXIT_FAILURE);
    }

    // Calculate size of request buffer
    request_size += 100;

    // Initialize variables for storing URL parts
    char *args = "";
    int port = 80;
    char *host=NULL, *path=NULL;

    // Parse URL into hostname, port, and path
    if (UrlToString(Url, (int)strlen(Url), &host, &port, &path) == -1){
        USAGE;
        freeAllocated(args,host,path,arg,par);
        exit(EXIT_FAILURE);
    }

    // Initialize request buffer
    char request[request_size];

    // Determine type of request (GET or POST)
    char type[5] = "";
    if (parLen > 0) {
        strcat(type, "POST");
    } else {
        strcat(type, "GET");
    }
    type[strlen(type)] = '\0';

    // Convert query string arguments into a properly formatted string
    if (argLen > 0) {
        args = ArgsToString(arg, argLen);
    }

    // Construct HTTP request
    sprintf(request, "%s %s%s HTTP/1.0\r\nHost: %s\r\n\r\n", type, path, args, host);

    // Add POST request body to HTTP request, if provided
    if (parLen > 0) {
        sprintf(request, "%s %s%s HTTP/1.0\r\nHost: %s\r\nContent-length: %d\r\n\r\n%s", type, path, args, host, parLen, par);
    }

    // Print HTTP request for debugging purposes
    printf("HTTP request =\n%s\nLEN = %d\n", request, (int)strlen(request));


    // Connect to server
    int fd = socket_connect(host, port);
    if (fd < 0){
        perror("socket_connect");
        freeAllocated(args,host,path,arg,par);
        exit(EXIT_FAILURE);
    }

    // Send HTTP request to server
    Request(fd, request);

    freeAllocated(args,host,path,arg,par);
    return 0;
}



// C
void Request(int fd, char *msg) {
    // Calculate total number of bytes to be written
    int total=(int)strlen(msg);

    // Keep track of number of bytes written
    int done=0;

    // Write to the socket until all bytes are written
    while(done!=total){
        int r;
        if ((r = write(fd, &msg[done], total - done)) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        done += r;
    }

    // Read from the socket into a buffer
    unsigned char buffer[1024];
    int size=0,r;
    bzero(buffer, sizeof(buffer));
    while ((r=read(fd, buffer, sizeof(buffer) - 1)) != 0) {
        // Print the received message
        int n=0;
        while(n!=r){
            printf("%c",buffer[n]);
            n++;
        }
        bzero(buffer, sizeof(buffer));
        size += r;
    }

    // Print the total number of bytes received
    printf("\n Total received response bytes: %d\n",size);

    // Shut down the socket for reading and writing
    if(shutdown(fd, SHUT_RDWR)!= 0){
        perror("shutdown");
        exit(EXIT_FAILURE);
    }

    // Close the socket
    if(close(fd)!=0){
        perror("close");
        exit(EXIT_FAILURE);
    }
}

int socket_connect(char *host, int port) {
    // Create a socket
    int socket_fd;
    if( (socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("create socket failed\n");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    struct hostent *hp;
    if((hp = gethostbyname(host)) == NULL){
        herror("gethostbyname failed\n");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_addr.s_addr = ((struct in_addr *) (hp->h_addr))->s_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Connect to server
    if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Return socket descriptor
    return socket_fd;
}



char *ArgsToString(char **arg, int argLen) {
    // Initialize variables
    int resLen = 0;
    int count = 0;
    char *res = NULL;

    // Calculate the length of the resulting string and the number of valid arguments
    for (int j = 0; j < argLen; ++j) {
        int i = 1;
        resLen += (int)strlen(arg[j]);
        while (arg[j][i] != '\0') {
            if (arg[j][i] == '=' && arg[j][i + 1] != '\0')
                count++;
            i++;
        }
    }

    // If the number of valid arguments is not equal to the total number of arguments, return NULL
    if (argLen != count) {
        return res;
    }

    // Allocate memory for the resulting string
    resLen += argLen ;
    res = (char *) calloc(sizeof(char) , resLen + 1);
    if(res == NULL) {
        fprintf(stderr, "calloc failed\n");
        exit(EXIT_FAILURE);
    }

    // Concatenate the arguments into the resulting string
    res[0]='?';
    for (int i = 0; i < argLen; ++i) {
        strncat(res, arg[i], strlen(arg[i]));
        if(i<argLen-1)
            strncat(res, "&",1);
    }
    res[resLen] = '\0';

    // Return the resulting string
    return res;
}


int UrlToString(char *url, int length, char **host, int *port, char **path) {
    // Check if the URL starts with "http://"
    int i = 7;
    if (strncmp("http://", url, 7) != 0)
        return -1;

    // Check if the URL contains a port number
    char *por = strstr(&url[6], ":");
    if (por != NULL) {
        // Set the port number
        *port = atoi(&por[1]);
        if(*port<0 || *port>65536 ){
            USAGE;
            exit(EXIT_FAILURE);
        }
        if (atoi(&por[1]) == 0 && por[1] == '0')
            *port = 80;
    }

    // Check if the URL contains a path
    if (strstr(&url[8], "/") != NULL) {
        // Allocate memory for the path and copy it into the path variable
        int len = (int)strlen(strstr(&url[8], "/"));
        *path = calloc(sizeof(char) , len + 1);
        if(path == NULL){
            fprintf(stderr, "calloc failed\n");
            exit(EXIT_FAILURE);
        }
        strncpy(*path, strstr(&url[8], "/"), len);
    } else {
        // Set the path to "/"
        *path = calloc(sizeof(char) , 2);
        if(path == NULL) {
            fprintf(stderr, "calloc failed\n");
            exit(EXIT_FAILURE);
        }
        strncpy(*path, "/",1);
    }

    // Find the end of the hostname
    while (i < length) {
        if (url[i] == '/' || url[i] == ':')
            break;
        i++;
    }

    // Allocate memory for the hostname and copy it into the host variable
    *host = calloc(sizeof(char) ,i - 7 + 1);
    if(*host == NULL) {
        fprintf(stderr, "calloc failed\n");
        exit(EXIT_FAILURE);
    }
    strncpy(*host, &url[7], i - 7);

    // Return success
    return 0;
}

