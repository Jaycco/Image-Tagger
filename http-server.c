/*
** change from http-server.c
*/

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "linklist.h"
#include "handle_list.h"
#include "get_post_page.h"

// constants
static char const * const HTTP_400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_400_LENGTH = 47;
static char const * const HTTP_404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_404_LENGTH = 45;

// represents the types of method
typedef enum
{
    GET,
    POST,
    UNKNOWN
} METHOD;

static bool handle_http_request(int sockfd, list_t *list, int *round, int *next_cookie)
{
    // try to read the request
    char buff[2049];
    bzero(buff, 2049);
    int n = read(sockfd, buff, 2049);
    int cookie_boolean = 0;
    int cookie = 0;
    if (strstr(buff, "Cookie:") != NULL) {
        cookie_boolean = 1;
        char *cookie_string = strstr(buff, "Cookie: ") + 8;
        cookie = atoi(cookie_string);
    }
    // set gameover or endgame
    node_t *current_g;
    bool is_gameover = false;
    bool is_endgame = false;
    assert(list!=NULL);
    current_g = list->head;
    while (current_g) {
        if (cookie == current_g->data.cookie) {
            if (current_g->data.gameover_page){
                is_gameover = true;
            }
            if (current_g->data.endgame_page){
                is_endgame = true;
            }
            break;
        }
        current_g = current_g->next;
    }
    if (n <= 0)
    {
        if (n < 0)
            perror("read");
        else
            printf("socket %d close the connection\n", sockfd);
        return false;
    }

    // terminate the string
    buff[n] = 0;

    char * curr = buff;

    // parse the method
    METHOD method = UNKNOWN;
    if (strncmp(curr, "GET ", 4) == 0)
    {
        curr += 4;
        method = GET;
    }
    else if (strncmp(curr, "POST ", 5) == 0)
    {
        curr += 5;
        method = POST;
    }
    else if (write(sockfd, HTTP_400, HTTP_400_LENGTH) < 0)
    {
        perror("write");
        return false;
    }

    // sanitise the URI
    if (method == GET) {
        if (strncmp(buff+6, "start=", 6) == 0){
            curr += 13;
        }
    } else {
        if (strncmp(buff+7, "start=", 6) == 0){
            curr += 13;
        }
    }

    // sanitise the URI
    while (*curr == '.' || *curr == '/')
        ++curr;
    // assume the only valid request URI is "/" but it can be modified to accept more files
    if (*curr == ' ')
        // html pages to be handled have 4 possible cases: gameover, endgame, GET and POST
        if (is_gameover){
            char * POST_file_name = "7_gameover.html";
            set_gameover_page(list, cookie, false);
            if(!get_page(sockfd, POST_file_name, round)){
                return false;
            }
            return false;
        } else if (is_endgame){
            char * POST_file_name = "6_endgame.html";
            set_endgame_page(list, cookie, false);
            if(!get_page(sockfd, POST_file_name, round)){
                return false;
            }
        } else if (method == GET) {
            char * GET_file_name = "1_intro.html";
            // no cookie then send 1st page, contain 'start=' then send 3rd page, if not then send 2nd page.
            if (!cookie_boolean) {
                if(!get_page(sockfd, GET_file_name, round)){
                    return false;
                }
            } else if (strncmp(buff+6, "start=", 6) == 0){
                set_ready(list, cookie, true);
                pair_users(list);
                GET_file_name = "3_first_turn.html";
                if(!get_page(sockfd, GET_file_name, round)){
                    return false;
                }
            } else {
                char * POST_file_name = "2_start.html";
                // initialise user in linklist list
                char username[1000000];
                node_t *current;
                assert(list!=NULL);
                current = list->head;
                while (current) {
                    if (cookie == current->data.cookie) {
                        strcpy(username, current->data.username);
                        current->data.is_ready = false;
                        current->data.is_start = false;
                        current->data.gameover_page = false;
                        current->data.endgame_page = false;
                        current->data.pair_cookie = 0;
                    }
                    current = current->next;
                }
                bool username_bool = true;
                if (!post_page(sockfd, POST_file_name, list, username, username_bool,
                        cookie_boolean, round, next_cookie)){
                    return false;
                }
            }

        }
        else if (method == POST) {
            // locate the username, it is safe to do so in this sample code, but usually the result is expected to be
            // copied to another buffer using strcpy or strncpy to ensure that it will not be overwritten.
            char * POST_file_name = "2_start.html";
            bool username_bool = false;
            // if contain 'quit=' send 7th page, if contain 'keyword' then need more information, if not send 5th page
            if (strstr(buff, "quit=") != NULL) {
                POST_file_name = "7_gameover.html";
                if (is_pair(list, cookie)) {
                    gameover_quit(list, cookie);
                    set_gameover_page(list, cookie, false);
                }
                if(!get_page(sockfd, POST_file_name, round)){
                    return false;
                }
                return false;
            } else if (strstr(buff, "keyword=") != NULL) {
                bool post_page_bool = true;
                // send 5th page if not start, send 6th page if keyword in another players' keywords, if not then 4th p
                if (is_start(list, cookie)) {
                    // get keyword and store in keywords
                    if (strstr(buff, "=&") == NULL) {
                        char * keyword_o = strstr(buff, "keyword=") + 8;
                        char *keyword_strtok;
                        keyword_strtok = strtok(keyword_o, "&");
                        char keyword[strlen(keyword_o)];
                        strcpy(keyword, keyword_strtok);
                        // endgame_keyword return true if it is endgame now
                        if (endgame_keyword(list, cookie, keyword)) {
                            post_page_bool = false;
                            POST_file_name = "6_endgame.html";
                            if(!get_page(sockfd, POST_file_name, round)){
                                return false;
                            }
                            *round += 1;
                            set_endgame_page(list, cookie, false);
                        } else {
                            POST_file_name = "4_accepted.html";
                            strcat(get_keywords(list, cookie), keyword);
                            strcat(get_keywords(list, cookie), " ");
                        }
                    } else {
                        POST_file_name = "4_accepted.html";
                    }
                } else {
                    POST_file_name = "5_discarded.html";
                }
                if (post_page_bool){
                    if (!post_page(sockfd, POST_file_name, list, get_keywords(list, cookie), username_bool,
                            cookie_boolean, round, next_cookie)){
                        return false;
                    }
                }
            } else {
                char * username_o = strstr(buff, "user=") + 5;
                char username[strlen(username_o)];
                strcpy(username, username_o);
                bool username_bool = true;
                if (!post_page(sockfd, POST_file_name, list, username, username_bool,
                        cookie_boolean, round, next_cookie)){
                    return false;
                }
            }
        }
        else
            // never used, just for completeness
            fprintf(stderr, "no other methods supported");
        // send 404
    else if (write(sockfd, HTTP_404, HTTP_404_LENGTH) < 0)
    {
        perror("write");
        return false;
    }
    return true;
}

int main(int argc, char * argv[])
{
    list_t *list = make_empty_list();
    int r = 1;
    int *round;
    round = &r;
    int next_c = 1;
    int *next_cookie;
    next_cookie = &next_c;

    if (argc < 3)
    {
        fprintf(stderr, "usage: %s ip port\n", argv[0]);
        return 0;
    }

    // create TCP socket which only accept IPv4
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // reuse the socket if possible
    int const reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // create and initialise address we will listen on
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // if ip parameter is not specified
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // bind address to socket
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen on the socket
    listen(sockfd, 5);

    // initialise an active file descriptors set
    fd_set masterfds;
    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);
    // record the maximum socket number
    int maxfd = sockfd;

    while (1)
    {
        // monitor file descriptors
        fd_set readfds = masterfds;
        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // loop all possible descriptor
        for (int i = 0; i <= maxfd; ++i)
            // determine if the current file descriptor is active
            if (FD_ISSET(i, &readfds))
            {
                // create new socket if there is new incoming connection request
                if (i == sockfd)
                {
                    struct sockaddr_in cliaddr;
                    socklen_t clilen = sizeof(cliaddr);
                    int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
                    if (newsockfd < 0)
                        perror("accept");
                    else
                    {
                        // add the socket to the set
                        FD_SET(newsockfd, &masterfds);
                        // update the maximum tracker
                        if (newsockfd > maxfd)
                            maxfd = newsockfd;
                        // print out the IP and the socket number
                        char ip[INET_ADDRSTRLEN];
                        printf(
                                ">>>new connection from %s on socket %d\n",
                                // convert to human readable string
                                inet_ntop(cliaddr.sin_family, &cliaddr.sin_addr, ip, INET_ADDRSTRLEN),
                                newsockfd
                        );
                    }
                }
                    // a request is sent from the client
                else if (!handle_http_request(i, list, round, next_cookie))
                {
                    close(i);
                    FD_CLR(i, &masterfds);
                }
            }
    }

    return 0;
}
