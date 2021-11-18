#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <ctype.h>
#include <signal.h>

#define pic_MAXLEN 10000005
#define MAXLIS 1024
#define MAXLINE 4096
char buff[pic_MAXLEN + 5];

int getLength(char *package)
{
    char buff[MAXLINE];
    strcpy(buff, package);
    int len;
    char *ptr;
    ptr = strstr(buff, "Content-Length: ");
    ptr += strlen("Content-Length: ");
    char *ptr_end;
    ptr_end = ptr;
    while (isdigit(*ptr_end))
    {
        ptr_end++;
    }
    ptr_end = '\0';
    len = atoi(ptr);
    return len;
}

char *findBoundary(char *package)
{
    char buff[MAXLINE];
    strcpy(buff, package);
    char *boundary;
    char *end;
    boundary = strstr(buff, "boundary=");
    boundary += 9;
    end = boundary;
    end = strstr(boundary, "\r\n");
    *(end) = '\0';
    return boundary;
}

char *findImg(char *package, char *boundary)
{

    char buff[100000];
    strcpy(buff, package);

    char boundary_start[MAXLINE] = "--";
    strcat(boundary_start, boundary);
    char boundary_end[MAXLINE] = "--";
    strcat(boundary_end, boundary);
    strcat(boundary_end, "--");
    int lens = strlen(boundary_start);
    //printf("len = %d\n", lens);

    char *img;
    img = package;
    printf("%s\n", buff);
    img = strstr(buff, boundary_start);
    img = strstr(img, "Content-Type: image/png\r\n\r\n");
    img += strlen("Content-Type: image/png\r\n\r\n");
    //printf("img is \n %s",img);

    return img;
}

char *get_picture(char *package_head)
{
    char *c = package_head;
    c = strstr(c, "\r\n\r\n");
    c = c + 5;
    c = strstr(c, "\r\n\r\n");
    c = c + 4;
    return c;
}

int main(int argc, char **argv)
{
    /*create a socket*/
    int clientSockfd = 0;
    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("Fail to create a socket.\n");
        exit(1);
    }

    int reuseAddrOption = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseAddrOption, sizeof(reuseAddrOption)) < 0)
    {
        printf("Fail to set a socket: %s\n", strerror(errno));
        exit(1);
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseAddrOption, sizeof(reuseAddrOption));

    /*set your socket*/
    int SERV_PORT = 8787;
    struct sockaddr_in myaddr;
    struct sockaddr_in client;

    bzero((char *)&myaddr, sizeof(myaddr));
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(SERV_PORT);

    /*bind addr to socket*/
    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
        printf("Fail to bind a socket: %s\n", strerror(errno));
        exit(1);
    }

    /*listen to socket*/
    if (listen(sockfd, MAXLIS) < 0)
    {
        printf("Fail to listen s socket\n");
        exit(1);
    }
    printf("Listen Successful\n");

    /*wait until the connect request*/

    /* prevent zombie*/
    signal(SIGCHLD, SIG_IGN);

    /*accept client connect request*/
    while (1)
    {
        clientSockfd = accept(sockfd, NULL, NULL);
        if (clientSockfd < 0)
        {
            printf("Fail to accept a socket.\n");
        }
        printf("Accept package!\n");
        pid_t pid = fork();

        if (pid == -1)
        {
            printf("fork error\n");
            exit(1);
        }

        if (!pid)
        {
            /*child_process*/

            /*after accept put package into buff*/
            if (read(clientSockfd, buff, pic_MAXLEN) == -1)
            {
                printf("Fail to read a package.\n");
                exit(1);
            }

            printf("Sucessful Read! Package Message is\n");
            printf("%s", buff);

            if (strncmp(buff, "GET ", 4) == 0 && strstr(buff, "Accept: text/html"))
            {
                write(clientSockfd, "HTTP/1.1 200 OK\r\n", (sizeof("HTTP/1.1 200 OK\r\n") / sizeof(char)) - 1);
                write(clientSockfd, "Context-Type: text/html; charset=utf-8\r\n", (sizeof("Context-Type: text/html; charset=utf-8\r\n") / sizeof(char)) - 1);
                write(clientSockfd, "\r\n", (sizeof("\r\n") / sizeof(char)) - 1);
                FILE *html;
                char tmp[MAXLINE];
                if ((html = fopen("./index.html", "r")) == NULL)
                {
                    printf("ERROR opening: %s\n", strerror(errno));
                    exit(1);
                }

                size_t bytes_read;
                while ((bytes_read = fread(tmp, sizeof(char), MAXLINE, html)) > 0)
                {
                    if (write(clientSockfd, tmp, bytes_read) < 0)
                    {
                        printf("ERROR writing: %s\n", strerror(errno));
                    }
                }
            }
            else if (strncmp(buff, "GET ", 4) == 0 && strstr(buff, "GET /img1.jpeg"))
            {
                write(clientSockfd, "HTTP/1.1 200 OK\r\n", (sizeof("HTTP/1.1 200 OK\r\n") / sizeof(char)) - 1);
                write(clientSockfd, "Context-Type: image/jpeg\r\n", (sizeof("Context-Type: image/jpeg\r\n") / sizeof(char)) - 1);
                write(clientSockfd, "\r\n", (sizeof("\r\n") / sizeof(char)) - 1);
                FILE *img;
                u_char tmp[MAXLINE];
                if ((img = fopen("./img1.jpeg", "r")) == NULL)
                {
                    printf("ERROR opening: %s\n", strerror(errno));
                    exit(1);
                }

                size_t bytes_read;
                while ((bytes_read = fread(tmp, sizeof(char), MAXLINE, img)) > 0)
                {
                    if (write(clientSockfd, tmp, bytes_read) < 0)
                    {
                        printf("ERROR writing: %s\n", strerror(errno));
                    }
                }
            }
            else if (strncmp(buff, "POST ", 5) == 0 && strstr(buff, "multipart/form-data"))
            {
                char *boundary = findBoundary(buff);
                //printf("boundary is %s\n",boundary);
                char *img = get_picture(buff);
                //printf("img is \n %s",img);
                int len = getLength(buff);
                //printf("len is %d\n", len);
                FILE *fp = fopen("img1.jpeg", "w");
                if (fp == NULL)
                {
                    printf("fopen error\n");
                    exit(1);
                }

                fwrite(img, sizeof(char), len, fp);
                fclose(fp);

                write(clientSockfd, "HTTP/1.1 200 OK\r\n", (sizeof("HTTP/1.1 200 OK\r\n") / sizeof(char)) - 1);
                write(clientSockfd, "Context-Type: text/html; charset=utf-8\r\n", (sizeof("Context-Type: text/html; charset=utf-8\r\n") / sizeof(char)) - 1);
                write(clientSockfd, "\r\n", (sizeof("\r\n") / sizeof(char)) - 1);
                FILE *html;
                char tmp[MAXLINE];
                if ((html = fopen("./index.html", "r")) == NULL)
                {
                    printf("ERROR opening: %s\n", strerror(errno));
                    exit(1);
                }

                size_t bytes_read;
                while ((bytes_read = fread(tmp, sizeof(char), MAXLINE, html)) > 0)
                {
                    if (write(clientSockfd, tmp, bytes_read) < 0)
                    {
                        printf("ERROR writing: %s\n", strerror(errno));
                    }
                }
            }

            shutdown(clientSockfd, SHUT_RDWR);
            close(clientSockfd);
            exit(0);
        }
    }

    return 0;
}
