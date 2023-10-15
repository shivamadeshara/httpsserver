#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <regex.h>
#include <sys/stat.h>
#include "asgn2_helper_funcs.h"

#define BUFFERSIZE 4096
//#define REQUEST_LINE_REGEX "^([a-zA-z]+)()/(/?[a-zA-z0-9_.-])*()(HTTP/[0-9].[0-9])"
#define METHOD_REGEX  "^([a-zA-z]+)( )"
#define URI_REGEX     "/(/?[a-zA-z0-9_.-]+) "
#define VERSION_REGEX "(HTTP/[0-9]+.[0-9]+)"

typedef struct RequestObj *Request;
typedef struct RequestObj {
    char *version;
    char *command;
    char *target_path;
    int content_length;
} RequestObj;

Request newRequest(void) {
    Request r = malloc(sizeof(RequestObj));
    r->target_path = NULL;
    r->command = NULL;
    r->version = NULL;
    r->content_length = 0;
    return (r);
}
void delete_request(Request dr) {
    //fix this later
    free(dr->command);
    free(dr->target_path);
    free(dr->version);
    free(dr);
}
//
int header_parse(char *buf, Request req) {
    //returns content_lenght if found;
    int found = 1;
    regex_t key_value_pair;
    regmatch_t pair[3];
    char *pattern = "([a-zA-z0-9_.-]+:) ([^\r\n]+)\r\n";
    regcomp(&key_value_pair, pattern, REG_EXTENDED);

    while ((found = regexec(&key_value_pair, buf, 3, pair, 0)) == 0) {
        if (found == REG_NOMATCH) {
            //printf("No Match");
            break;
        }
        char *key = malloc(pair[1].rm_eo - pair[1].rm_so + 1);
        char *value = malloc(pair[2].rm_eo - pair[2].rm_so + 1);
        memcpy(key, buf + pair[1].rm_so, pair[1].rm_eo - pair[1].rm_so);
        key[pair[1].rm_eo - pair[1].rm_so] = '\0';
        //buf+= pair[1].rm_eo - pair[1].rm_so;
        memcpy(value, buf + pair[2].rm_so, pair[2].rm_eo - pair[2].rm_so);
        value[pair[2].rm_eo - pair[2].rm_so] = '\0';
        buf += pair[2].rm_eo;
        if (strcmp(key, "Content-Length:") == 0) {
            int x = atoi(value);
            req->content_length = x;
        }
        //printf("Key:%s\n", key);
        //printf("Value: %s\n", value);
    }

    regfree(&key_value_pair);
    return 200;
}
int request_parse(char *buf, Request request) {
    regex_t method_regex, uri_regex, version_regex;
    regmatch_t method[2];
    regmatch_t location[2];
    regmatch_t ver[2];
    regcomp(&method_regex, METHOD_REGEX, REG_EXTENDED);
    regcomp(&uri_regex, URI_REGEX, REG_EXTENDED);
    regcomp(&version_regex, VERSION_REGEX, REG_EXTENDED);

    //command parsing
    int found = 1;
    if ((found = regexec(&method_regex, buf, 2, method, 0)) == REG_NOMATCH) {
        //printf("No match found");
        return 400;
    }
    request->command = malloc(method[1].rm_eo - method[1].rm_so + 1);
    memcpy(request->command, buf + method[1].rm_so, method[1].rm_eo - method[1].rm_so);
    request->command[method[1].rm_eo - method[1].rm_so] = '\0';
    regfree(&method_regex);

    buf += (method[1].rm_eo - method[1].rm_so);
    //printf("%s", buf);
    //URI parsing
    if ((found = regexec(&uri_regex, buf, 2, location, 0)) == REG_NOMATCH) {
        //printf("No URI Match found");
        return 400;
    }
    request->target_path = malloc(location[1].rm_eo - location[1].rm_so + 1);
    memcpy(request->target_path, buf + location[1].rm_so, location[1].rm_eo - location[1].rm_so);
    request->target_path[location[1].rm_eo - location[1].rm_so] = '\0';
    regfree(&uri_regex);
    buf += location[1].rm_eo - location[1].rm_so;

    //version parsing
    if ((found = regexec(&version_regex, buf, 2, ver, 0)) == REG_NOMATCH) {
        //printf("No Version Match found");
        return 400;
    }
    request->version = malloc(ver[1].rm_eo - ver[1].rm_so + 1);
    memcpy(request->version, buf + ver[1].rm_so, ver[1].rm_eo - ver[1].rm_so);
    request->version[ver[1].rm_eo - ver[1].rm_so] = '\0';
    regfree(&version_regex);
    buf += ver[1].rm_eo + sizeof("\n");
    if (strcmp(request->version, "HTTP/1.1") != 0) {
        char *vers = request->version;
        vers += 5;
        if (strlen(vers) + 1 != 4) {
            return 400;
        } else {
            return 505;
        }
    } else {
        return 200;
    }
}

char *status_message(int code) {
    //writes the corresponding error message based on the code given
    switch (code) {
    case 200: return "OK";
    case 201: return "Created";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 505: return "Version Not Supported";
    }
    return "OK";
}
void get(Request req, int out) {
    int status = 200;
    int dir = open(req->target_path, __O_DIRECTORY);
    if (dir > 0) {
        status = 403;
    }
    int fd = open(req->target_path, O_RDONLY);
    if (fd < 0) {
        status = 404;
    }

    struct stat filesize;
    fstat(fd, &filesize);
    int size = filesize.st_size;

    if (status == 200) {
        //write the response format
        char buf[2048];
        sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Length %d\r\n\r\n", size);
        write_all(out, buf, strlen(buf));
        //output the file contentst
        pass_bytes(fd, out, size);
    } else if (status == 403) {
        char buf[2048];
        sprintf(buf, "HTTP/1.1 403 Forbidden\r\nContent-Length 10\r\n\r\nForbidden\n");
        write_all(out, buf, strlen(buf));
    } else {
        char buf[2048];
        sprintf(buf, "HTTP/1.1 404 Not Found\r\nContent-Length 10\r\n\r\nNot Found\n");
        write_all(out, buf, strlen(buf));
    }
    close(fd);
}

void put(Request req, int in, char *remainder) {
    char *location = req->target_path;
    int fd = open(location, O_RDWR | O_TRUNC, 0666);
    int code = 200;
    //file exists and you are
    if (fd > 0) {
        code = 200;
    } else {
        code = 201;
        fd = open(location, O_RDWR | O_TRUNC | O_CREAT, 0666);
    }
    //writing message body to the file
    //fprintf(stderr, "Content-Length:%d\n", req->content_length);

    //int bytesread = 0;
    //int bytesleft = req->content_length - bytesread;

    int y = write_all(fd, remainder, strlen(remainder));
    int x = pass_bytes(in, fd, (req->content_length - strlen(remainder)));
    int remaining = x + y;
    fprintf(stderr, "Bytes Read:%d, ContentLength;%d \n", remaining, req->content_length);
    if (remaining < req->content_length) {
        int bytesleft = req->content_length - remaining;
        pass_bytes(in, fd, bytesleft);
    }
    //fprintf(stderr, "Bytes Passes:%d\n", x);
    close(fd);
    //response
    char buf[2048];
    //formatting and writing response to
    char *msg = status_message(code);
    sprintf(
        buf, "HTTP/1.1 %d %s\r\nContent-Length: %lu\r\n\r\n%s\n", code, msg, strlen(msg) + 1, msg);
    write_all(in, buf, strlen(buf));
}

void other_response(int code, int out) {
    char buf[2049];
    char *msg = status_message(code);
    sprintf(
        buf, "HTTP/1.1 %d %s\r\nContent-Length: %lu\r\n\r\n%s\n", code, msg, strlen(msg) + 1, msg);
    write_all(out, buf, strlen(buf));
}

//error handling
void fatal_error(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}
//response struct to make sure response is formatted correctly it should be a char* array
/*Citation */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fatal_error("Usage \n");
        exit(1);
    }
    //get port number from ru
    int port = atoi(argv[1]);
    if (port < 1 || port > 65535)
        fatal_error("Invalid Port\n");
    //create socket and initialize it
    Listener_Socket sock;
    listener_init(&sock, port);
    while (true) {
        Request req = newRequest();
        int acceptfd = listener_accept(&sock);
        if (acceptfd > 0) {
            //initialize
            char buf[2049] = { '\0' };

            read_until(acceptfd, buf, 2048, "\r\n\r\n");
            //findout where /r/n/r/n is in the code
            //locate where the \r\n\r\n is (use strstr)
            //fprintf(stderr, "Buffer%s\n", buf);
            char *remainingpos = strstr(buf, "\r\n\r\n");
            char remainder[strlen(remainingpos) + 1];
            strcpy(remainder, remainingpos + strlen("\r\n\r\n"));
            //fprintf(stderr, "Remainder%s", remainder);

            //write the remaining stuff after the \r\n\r\n

            //buf[strlen(buf)] = '\0';
            int rp = request_parse(buf, req);
            //fprintf(stderr, "%d", rp);
            header_parse(buf, req);
            /*if(hp != 200){
                other_response(hp, acceptfd);
            }*/
            if ((strcmp(req->command, "GET") != 0 && strcmp(req->command, "PUT") != 0)) {
                other_response(501, acceptfd);
            } else if (strcmp(req->command, "GET") == 0) {
                if (rp != 200) {
                    other_response(rp, acceptfd);
                } else {
                    get(req, acceptfd);
                }
            } else if (strcmp(req->command, "PUT") == 0) {
                if (rp != 200) {
                    other_response(rp, acceptfd);
                } else {
                    put(req, acceptfd, remainder);
                }
            }
        }

        close(acceptfd);
        delete_request(req);
    }
    return 0;
}
/*int main(void){
    Request req = newRequest();
    char* input_string = "PUT /foo.txt HTTP/1.1\r\nHello: 13\r\nContent-Length: 12\r\nHey: 13\r\n\r\nHello";
    //char* headers = "Content-Length: 12\r\nHey: 13\r\n\r\n";
    //char* request_line = strtok(input_string, "\r\n");
    //char* header_fields = strtok(NULL, "\r\n\r\n");
    //printf("%s", request_line);
    //printf("%s", header_fields);
    int x = request_parse(input_string, req);
    int y = header_parse(input_string, req);

    printf("Command: %s\n", req->command);
    printf("Path: %s\n", req->target_path);
    printf("Version: %s\n", req->version);
    printf("Content-Length: %d\n", req->content_length);
    printf("%d\n%d", x, y);
    
}*/
