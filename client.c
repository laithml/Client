#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int UrlToString(char *, int, char **, int *, char **);

char *ArgsToString(char **, int);

int main(int argc, char *argv[]) {
    int i = 1;
    char *par;
    char **arg;
    int parLen;
    int argLen;
    char *Url;
    while (i < argc) {
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
                    argLen = n;
                    i = i + n + 1;
                    break;
            }
        } else {
            Url = argv[i];
        }
        i++;
    }

    char *res = ArgsToString(arg, argLen);
    int port;
    char *host,*path;
    UrlToString(Url, strlen(Url),&host,&port,&path);
    printf("arg: %s\n",res);
    printf("par: %s\n", par);
    printf("Host: %s\n", host);
    printf("Path: %s\n", path);
    printf("Port: %d\n", port);

    return 0;
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
    *port = atoi(&por[1]);
    if(atoi(&por[1]) ==0 && por[1]=='0')
        *port = 80;
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
