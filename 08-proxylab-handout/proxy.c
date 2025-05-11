#include <csapp.h>
#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char* user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void read_requesthdrs(rio_t* rp);
int  parse_uri(char* uri, char* filename, char* cgiargs);
void serve_static(int fd, char* filename, int filesize);
void get_filetype(char* filename, char* filetype);
void serve_dynamic(int fd, char* filename, char* cgiargs);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);

int main(int argc, char** argv)
{
    int                     listenfd, connfd;
    char                    hostname[MAXLINE], port[MAXLINE];
    socklen_t               clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd    = Accept(listenfd, (SA*)&clientaddr, &clientlen);   // line:netp:tiny:accept
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);    // line:netp:tiny:doit
        Close(connfd);   // line:netp:tiny:close
    }
}

typedef struct
{
    char host[MAXLINE];
    char port[MAXLINE];
    char path[MAXLINE];
} url_t;

void build_header(char* data, url_t* url, rio_t* rio)
{
    char host[MAXLINE];
    char line[MAXLINE];
    char other[MAXLINE];

    // 拼接一个HTTP请求 = 请求行 + 请求报头
    sprintf(host, "HOST: %s\r\n", url->host);
    while (Rio_readlineb(rio, line, MAXLINE) > 0) {
        if (strcmp(line, "\r\n")) break;
        if (strncmp(line, "HOST", 4) == 0) strcpy(host, line);   // replace host

        if (strncmp(line, "User-Agent", 10) && strncmp(line, "Connection", 10) &&
            strncmp(line, "Proxy-Connection", 16))
            strcat(other, line);
    }

    sprintf(data,
            "%s %s HTTP/1.0\r\n%s%sConnectionn: close\r\nProxy-Connection: close\r\n%s\r\n",
            method,
            url->path,
            host,
            other);
}
/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd)
{
    rio_t rio;
    url_t url;
    char  data[MAXLINE];
    char  buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    /* Read request line */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) return;
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET")) {
        printf("Proxy does not implement the method");
        return;
    }
    /* parse client request and generate a new request */
    parse_uri(uri, &url);

    build_header(data, &url, &rio);

    /* connect to server */
    int clientfd;
    if ((clientfd = Open_clientfd(url.host, url.port)) < 0) {
        printf("Connection to server failed");
    }

    // send to server
    Rio_readinitb(&rio, clientfd);
    Rio_writen(clientfd, data, sizeof(data));

    // response to client
    int len;
    while ((len = Rio_readlineb(&rio, buf, MAXLINE)) > 0) Rio_writen(fd, buf, len);

    Close(clientfd);
}
/* $end doit */

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t* rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while (strcmp(buf, "\r\n")) {   // line:netp:readhdrs:checkterm
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */


/* $begin parse_uri */
int parse_uri(char* str, url_t* url)
{
    // get the first place of '//'
    char* ptr = strstr(str, "//");

    if (ptr == NULL) {
        printf("No host found\r\n");
        return;
    }
    str = ptr + 2;   // 指向"//"后面的字符

    ptr = strchr(str, '/');   // 指向：第一次出现的位置
    if (ptr == NULL) {
        printf("No host found again\r\n");
        return;
    }
    strcpy(url->path, ptr);
    *ptr = '\0';   // 截断str

    ptr = strchr(str, ':');
    if (ptr == NULL) {
        printf("use default port 80\r\n");
        strcpy(url->port, "80");
    }
    else {
        strcpy(url->port, ptr + 1);
        *ptr = '\0';   // 截断port
    }

    strcpy(url->host, str);
}
/* $end parse_uri */

/*
 * serve_static - copy a file back to the client
 */
/* $begin serve_static */
void serve_static(int fd, char* filename, int filesize)
{
    int   srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);      // line:netp:servestatic:getfiletype
    sprintf(buf, "HTTP/1.0 200 OK\r\n");   // line:netp:servestatic:beginserve
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n", filesize);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(fd, buf, strlen(buf));   // line:netp:servestatic:endserve

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);                           // line:netp:servestatic:open
    srcp  = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);   // line:netp:servestatic:mmap
    Close(srcfd);                                                  // line:netp:servestatic:close
    Rio_writen(fd, srcp, filesize);                                // line:netp:servestatic:write
    Munmap(srcp, filesize);                                        // line:netp:servestatic:munmap
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char* filename, char* filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char* filename, char* cgiargs)
{
    char buf[MAXLINE], *emptylist[] = {NULL};

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) { /* Child */   // line:netp:servedynamic:fork
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);                        // line:netp:servedynamic:setenv
        Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */   // line:netp:servedynamic:dup2
        Execve(filename, emptylist, environ);
        /* Run CGI program */   // line:netp:servedynamic:execve
    }
    Wait(NULL); /* Parent waits for and reaps child */   // line:netp:servedynamic:wait
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf,
            "<body bgcolor="
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
/* $end clienterror */
