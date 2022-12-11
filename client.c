#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define USAGE  fprintf(stderr,"Usage: client [-p n <text>] [-r n < pr1=value1 pr2=value2 ...>] <URL>\n")


int UrlToString(char *, int, char **, int *, char **);

char *ArgsToString(char **, int);

int socket_connect(char *, int);

void Request(int, char *);

int main(int argc, char *argv[]) {
    int i = 1, request_size = 0;
    char *par = NULL, **arg = NULL;
    int parLen = 0, argLen = 0;
    char *Url;
    while (i < argc) {
        request_size += (int)strlen(argv[i]);
        if (argv[i][0] == '-') {
            int n = atoi(argv[i + 1]);
            if (n == 0 && argv[i + 1][0] != '0') {
                USAGE;
                exit(EXIT_FAILURE);
            }
            switch (argv[i][1]) {
                case 'p':
                    par = malloc(sizeof(char) * n + 1);
                    if (par == NULL) {
                        fprintf(stderr, "malloc failed\n");
                        exit(EXIT_FAILURE);
                    }
                    strcat(par, argv[i + 2]);
                    par[n] = '\0';
                    parLen = n;
                    request_size  += n;
                    i = i + 2;
                    break;
                case 'r':
                    arg = (char **) malloc(sizeof(char *) * n);
                    if (arg == NULL) {
                        fprintf(stderr, "malloc failed\n");
                        exit(EXIT_FAILURE);
                    }
                    for (int j = 0; j < n; j++) {
                        arg[j] = argv[i + j + 2];
                    }
                    request_size += n;
                    argLen = n;
                    i = i + n + 1;
                    break;
            }
        } else {
            Url = argv[i];
        }
        i++;
    }
    if(Url == NULL){
        USAGE;
        exit(EXIT_FAILURE);
    }
    request_size += 100;
    char *args = "";
    int port = 80;
    char *host, *path;
    if (UrlToString(Url, (int)strlen(Url), &host, &port, &path) == -1){
        USAGE;
        exit(EXIT_FAILURE);
    }

    char request[request_size];
    char type[5] = "";
    if (parLen > 0) {
        strcat(type, "POST");
    } else {
        strcat(type, "GET");
    }
    type[strlen(type)] = '\0';
    if (argLen > 0) {
        args = ArgsToString(arg, argLen);
    }
    sprintf(request, "%s %s%s HTTP/1.1\r\nHost: %s\r\n", type, path, args, host);


    if (parLen > 0)
        sprintf(request, "%s %s%s HTTP/1.1\r\nHost: %s\nContent-length: %d\n%s\r\n", type, path, args, host, parLen, par);

    sprintf(request,"%sConnection: close\r\n\r\n",request);

    printf("HTTP request =\n%s\nLEN = %d\n", request, (int)strlen(request));



    int fd = socket_connect(host, port);
    if (fd < 0){
        perror("socket_connect");
        exit(EXIT_FAILURE);
    }

    Request(fd, request);

    free(args);
    free(host);
    free(path);
    free(arg);
    free(par);

    return 0;
}

void Request(int fd, char *msg) {
    int total=strlen(msg);
    int done=0;
    while(done!=total){
        int r;
        if ((r = write(fd, &msg[done], total - done)) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        done += r;
    }

    char buffer[1024];
    int size=0,r;
    bzero(buffer, sizeof(buffer));
    while ((r=read(fd, buffer, sizeof(buffer) - 1)) != 0) {
        printf("%s", buffer);
        bzero(buffer, sizeof(buffer));
        size += r;
    }

    printf("\n Total received response bytes: %d\n",size);


    shutdown(fd, SHUT_RDWR);
    close(fd);
}

int socket_connect(char *host, int port) {
    int socket_fd;
    struct sockaddr_in server_addr;
   if( (socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
       perror("create socket failed\n");
       exit(EXIT_FAILURE);
   }
    bzero(&server_addr, sizeof(server_addr));
    struct hostent *hp;
    if((hp = gethostbyname(host)) == NULL){
        herror("gethostbyname failed\n");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_addr.s_addr = ((struct in_addr *) (hp->h_addr))->s_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    return socket_fd;

}


char *ArgsToString(char **arg, int argLen) {
    int resLen = 0;
    int count = 0;
    char *res = NULL;
    for (int j = 0; j < argLen; ++j) {
        int i = 1;
        resLen += (int)strlen(arg[j]);
        while (arg[j][i] != '\0') {
            if (arg[j][i] == '=' && arg[j][i + 1] != '\0')
                count++;
            i++;
        }
    }
    if (argLen != count) {
        return res;
    }
    resLen += argLen - 1;
    res = (char *) malloc(sizeof(char) * resLen + 2);
    if(res == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }
    strcat(res, "?");
    for (int i = 0; i < argLen; ++i) {
        strcat(res, arg[i]);
        strcat(res, "&");
    }
    res[resLen] = '\0';
    return res;
}

int UrlToString(char *url, int length, char **host, int *port, char **path) {
    int i = 7;
    if (strncmp("http://", url, 7) != 0)
        return -1;
    char *por = strstr(&url[6], ":");

    if (por != NULL) {
        *port = atoi(&por[1]);
        if (atoi(&por[1]) == 0 && por[1] == '0')
            *port = 80;
    }

    if (strstr(&url[8], "/") != NULL) {
        int len = (int)strlen(strstr(&url[8], "/"));
        *path = malloc(sizeof(char) * len + 1);
        if(path == NULL){
            fprintf(stderr, "malloc failed\n");
            exit(EXIT_FAILURE);
        }
        strncpy(*path, strstr(&url[8], "/"), len);
    } else {

        *path = malloc(sizeof(char) * 2);
        if(path == NULL) {
            fprintf(stderr, "malloc failed\n");
            exit(EXIT_FAILURE);
        }
        strcpy(*path, "/");
    }

    while (i < length) {
        if (url[i] == '/' || url[i] == ':')
            break;
        i++;
    }
    *host = malloc(sizeof(char) * i - 7 + 1);
    if(*host == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }
    strncpy(*host, &url[7], i - 7);
    return 0;
}
