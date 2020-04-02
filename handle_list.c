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

// get keywords of the  player with the cookie in the list
char
*get_keywords(list_t *list, int cookie) {
    node_t *current;
    assert(list!=NULL);
    current = list->head;
    while (current) {
        if (cookie == current->data.cookie) {
            return current->data.keywords;
        }
        current = current->next;
    }
    return NULL;
}

// return true if the player with the cookie is start
bool
is_start(list_t *list, int cookie) {
    node_t *current;
    assert(list!=NULL);
    current = list->head;
    while (current) {
        if (cookie == current->data.cookie) {
            if (current->data.is_start){
                return true;
            }
        }
        current = current->next;
    }
    return false;
}

// return true if the player with the cookie is paired with another player
bool
is_pair(list_t *list, int cookie) {
    node_t *current;
    assert(list!=NULL);
    current = list->head;
    while (current) {
        if (cookie == current->data.cookie) {
            if (current->data.pair_cookie != 0){
                return true;
            }
        }
        current = current->next;
    }
    return false;
}

// set_things of player with the cookie to be true or false
void
set_ready(list_t *list, int cookie, bool set_bool) {
    node_t *current;
    assert(list!=NULL);
    current = list->head;
    while (current) {
        if (cookie == current->data.cookie) {
            current->data.is_ready = set_bool;
        }
        current = current->next;
    }
}

void
set_start(list_t *list, int cookie_1, int cookie_2, bool set_bool) {
    node_t *current;
    assert(list!=NULL);
    current = list->head;
    while (current) {
        if (cookie_1 == current->data.cookie) {
            current->data.is_start = set_bool;
            current->data.pair_cookie = cookie_2;
        }
        if (cookie_2 == current->data.cookie) {
            current->data.is_start = set_bool;
            current->data.pair_cookie = cookie_1;
        }
        current = current->next;
    }
}

void
set_gameover_page(list_t *list, int cookie, bool set_bool) {
    node_t *current;
    assert(list!=NULL);
    current = list->head;
    while (current) {
        if (cookie == current->data.cookie) {
            current->data.gameover_page = set_bool;
        }
        current = current->next;
    }
}

void
set_endgame_page(list_t *list, int cookie, bool set_bool) {
    node_t *current;
    assert(list!=NULL);
    current = list->head;
    while (current) {
        if (cookie == current->data.cookie) {
            current->data.endgame_page = set_bool;
        }
        current = current->next;
    }
}

// pair 2 players who is ready and set them to be start
void
pair_users(list_t *list) {
    int count = 0;
    int cookie_1 = 0;
    int cookie_2 = 0;
    node_t *current;
    assert(list!=NULL);
    current = list->head;
    while (current) {
        if (current->data.is_ready) {
            count += 1;
            if (count == 1) {
                cookie_1 = current->data.cookie;
            } else if (count == 2) {
                cookie_2 = current->data.cookie;
            }
        }
        current = current->next;
    }
    if (count == 2) {
        set_start(list, cookie_1, cookie_2, true);
    }
}

// set variables of players in link list in case gameover or endgame
void
gameover_setting(list_t *list, int cookie, node_t *current){
    set_gameover_page(list, cookie, true);
    set_gameover_page(list, current->data.pair_cookie, true);
    set_endgame_page(list, cookie, false);
    set_endgame_page(list, current->data.pair_cookie, false);
    set_ready(list, cookie, false);
    set_ready(list, current->data.pair_cookie, false);
    bzero(get_keywords(list, cookie), 1000000);
    bzero(get_keywords(list, current->data.pair_cookie), 1000000);
    set_start(list, current->data.pair_cookie, 0, false);
    set_start(list, cookie, 0, false);
}

void
endgame_setting(list_t *list, int cookie, node_t *current) {
    set_endgame_page(list, cookie, true);
    set_endgame_page(list, current->data.pair_cookie, true);
    set_ready(list, cookie, false);
    set_ready(list, current->data.pair_cookie, false);
    bzero(get_keywords(list, cookie), 1000000);
    bzero(get_keywords(list, current->data.pair_cookie), 1000000);
    set_start(list, cookie, current->data.pair_cookie, false);
    set_start(list, current->data.pair_cookie, cookie, false);
}

bool
endgame_keyword(list_t *list, int cookie, char *keyword) {
    node_t *current;
    assert(list!=NULL);
    current = list->head;
    char keyword_cmp[1000000];
    strcpy(keyword_cmp, keyword);
    strcat(keyword_cmp, " ");
    while (current) {
        if (cookie == current->data.cookie) {
            if (strstr(get_keywords(list, current->data.pair_cookie), keyword_cmp)){
                endgame_setting(list, cookie, current);
                return true;
            }
        }
        current = current->next;
    }
    return false;
}

bool
gameover_quit(list_t *list, int cookie) {
    node_t *current;
    assert(list!=NULL);
    current = list->head;
    while (current) {
        if (cookie == current->data.cookie) {
            gameover_setting(list, cookie, current);
        }
        current = current->next;
    }
    return false;
}