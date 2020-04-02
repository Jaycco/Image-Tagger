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

// constants
static char const * const HTTP_200_FORMAT = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";
static char const * const HTTP_200_FORMAT_COOKIE = "HTTP/1.1 200 OK\r\n\
Set-Cookie: %d\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";

// handle get page
bool get_page(int sockfd, char *GET_file_name, int *round)
{
    char buff[2049];
    bzero(buff, 2049);
    // get the size of the file
    struct stat st;
    stat(GET_file_name, &st);
    int n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
    // send the header first
    if (write(sockfd, buff, n) < 0) {
        perror("write");
        return false;
    }
    // read the content of the HTML file
    int filefd = open(GET_file_name, O_RDONLY);
    n = read(filefd, buff, 2048);
    if (n < 0) {
        perror("read");
        close(filefd);
        return false;
    }
    close(filefd);
    // change the image if round is not 1
    if (*round != 1) {
        if (strstr(buff, "image-")){
            char *image = strstr(buff, ".jpg") - 1;
            char num_c = '4';
            image[0] = num_c;
        }
    }
    // write
    long size = st.st_size;
    if (write(sockfd, buff, size) < 0) {
        perror("write");
        return false;
    }
    return true;
}

// handle post page
bool post_page(int sockfd, char *POST_file_name, list_t *list, char *print_word, bool username_bool,
        bool cookie_boolean, int *round, int *next_cookie)
{
    char buff[2049];
    bzero(buff, 2049);
    int n = 0;
    int print_word_length = strlen(print_word);
    // the length needs to include the ", " before the print_word
    long added_length = print_word_length + 7;

    // get the size of the file
    struct stat st;
    stat(POST_file_name, &st);
    // increase file size to accommodate the print_word
    long size = st.st_size + added_length;

    // set cookie if use do not have cookie
    if (cookie_boolean){
        n = sprintf(buff, HTTP_200_FORMAT, size);
    } else {
        int cookie;
        cookie = *next_cookie;
        *next_cookie += 1;
        n = sprintf(buff, HTTP_200_FORMAT_COOKIE, cookie, size);
        data_t data;
        data.cookie = cookie;
        char username_cpy[strlen(print_word)];
        strcpy(username_cpy, print_word);
        data.username = (char *) malloc((strlen(username_cpy) + 1) * sizeof(char));
        strcpy(data.username, username_cpy);
        insert_at_foot(list, data);
    }

    // send the header first
    if (write(sockfd, buff, n) < 0)
    {
        perror("write");
        return false;
    }
    // read the content of the HTML file
    int filefd = open(POST_file_name, O_RDONLY);
    n = read(filefd, buff, 2048);
    if (n < 0)
    {
        perror("read");
        close(filefd);
        return false;
    }
    close(filefd);
    // move the trailing part backward
    int p1, p2;
    char from_str[1000000];
    char to_str[1000000];
    int add_to_p = 0;
    // get the position to add to (add_to_p) according to username or keyword.
    if (username_bool) {
        strcpy(from_str, strstr(buff, "<form method=\"GET\">"));
        strcpy(to_str, strstr(buff, "</html>"));
        add_to_p = (strlen(from_str) + 1) - (strlen(to_str) - strlen("</html>"));
    } else {
        strcpy(from_str, strstr(buff, "</body>"));
        strcpy(to_str, strstr(buff, "</html>"));
        add_to_p = (strlen(from_str) + 1) - (strlen(to_str) - (strlen("</html>") + 2));
    }
    for (p1 = size - 1, p2 = p1 - added_length; p1 >= size - add_to_p; --p1, --p2){
        buff[p1] = buff[p2];
    }
    ++p2;
    // put the content
    buff[p2++] = '<';
    buff[p2++] = 'p';
    buff[p2++] = '>';
    // copy the print_word and put remaining content
    strncpy(buff + p2, print_word, print_word_length);
    p2 = p2 + print_word_length;
    buff[p2++] = '<';
    buff[p2++] = '/';
    buff[p2++] = 'p';
    buff[p2++] = '>';

    // change image if round is not 1
    if (*round != 1) {
        if (strstr(buff, "image-")){
            char *image = strstr(buff, ".jpg") - 1;
            char num_c = '4';
            image[0] = num_c;
        }
    }

    // write
    if (write(sockfd, buff, size) < 0)
    {
        perror("write");
        return false;
    }
    return true;
}