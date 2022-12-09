#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

int UrlToString(char *, int, char **, int *, char **);
char* requestMsg(int , char **);
char *ArgsToString(char **, int);
//
int main(int argc, char *argv[]) {
       char* request=requestMsg(argc, argv);
//    printf("HTTP request =\n%s\nLEN = %d\n", request, strlen(request));


    return 0;
}
void func(int sockfd)
{
    char buff[1024];
    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        write(sockfd, buff, sizeof(buff));
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("From Server : %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
}

char* requestMsg(int argc, char *argv[]){
    int i = 1,request_size=0;
    char *par=NULL,**arg=NULL;
    int parLen ,argLen;
    char *Url;
    while (i < argc) {
        request_size+= strlen(argv[i]);
        if (argv[i][0] == '-') {
            int n = atoi(argv[i + 1]);
            if (n == 0 && argv[i + 1][0] != '0') {
                fprintf(stderr, "Usage: %s [-h]\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            switch (argv[i][1]) {
                case 'p':
                    par = malloc(sizeof(char) * n + 1);
                    strcat(par, argv[i + 2]);
                    par[n] = '\0';
                    parLen = n;
                    i=i+2;
                    break;
                case 'r':
                    arg = (char **) malloc(sizeof(char *) * n);
                    for (int j = 0; j < n; j++) {
                        arg[j] = argv[i + j + 2];
                    }
                    request_size+=n;
                    argLen = n;
                    i = i + n + 1;
                    break;
            }
        } else {
            Url = argv[i];
        }
        i++;
    }
    request_size+=40;
    char *args="";
    int port=80;
    char *host,*path;
    if(UrlToString(Url, strlen(Url),&host,&port,&path)==-1)
        exit(EXIT_FAILURE);

    char *request= calloc(sizeof(char), request_size);
    char type[5]="";
    if(parLen > 0) {
        strcat(type,"POST");
    }else{
        strcat(type,"GET");
    }
    if(argLen>0)
        args = ArgsToString(arg, argLen);

    sprintf(request,"%s %s%s HTTP/1.1\nHost: %s",type,path,args,host);

    if(parLen>0)
        sprintf(request,"%s\nContent-length: %d\n\r\n\r\n%s",request,parLen,par);

    printf("HTTP request =\n%s\nLEN = %d\n", request, strlen(request));
    sprintf(request,"HTTP request =\n%s\nLEN = %d\n", request, strlen(request));

    ////////////////////////////////////////////////////////////////

    int sockfd;
    struct  sockaddr_in serveraddr;
    sockfd = socket(AF_INET, SOCK_STREAM,0);
    bzero(&serveraddr, sizeof(serveraddr));

    serveraddr.sin_addr.s_addr= inet_addr(gethostbyname(host));
    serveraddr.sin_port=htons(port);
    connect(sockfd, (struct sockaddr *)&serveraddr,sizeof(serveraddr));



        write(sockfd, request, strlen(request));
        bzero(request, strlen(request));
        read(sockfd, request, strlen(request));
        printf("From Server : %s", request);
        if ((strncmp(request, "exit", 4)) == 0) {
            printf("Client Exit...\n");

    }

    return request;
}

char *ArgsToString(char **arg, int argLen) {
    int resLen = 0;
    int count = 0;
    char *res = NULL;
    for (int j = 0; j < argLen; ++j) {
        int i = 1;
        resLen += strlen(arg[j]);
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

    if(por!=NULL){
        *port = atoi(&por[1]);
        if(atoi(&por[1]) ==0 && por[1]=='0')
            *port = 80;
    }

    if (strstr(&url[8], "/") != NULL){
        int len=strlen(strstr(&url[8], "/"));
        *path = malloc(sizeof(char) * len + 1);
        strncpy(*path,strstr(&url[8], "/"), len);
    }else{

    *path= malloc(sizeof(char) * 2);
    strcpy(*path,"/");
    }

    while (i < length) {
        if (url[i] == '/' || url[i] == ':')
            break;
        i++;
    }
    *host = malloc(sizeof(char) * i - 7 + 1);
    strncpy(*host,&url[7],i-7);
    return 0;
}
