// the information of player is stored in data_t
typedef struct {
    char *username;
    char keywords[1000000];
    int cookie;
    bool is_ready;
    bool is_start;
    bool gameover_page;
    bool endgame_page;
    int pair_cookie;
} data_t;

typedef struct node node_t;

struct node {
    data_t data;
    node_t *next;
};

typedef struct {
    node_t *head;
    node_t *foot;
} list_t;
/* same as #include "listops.c" (line 29-121)*/
// make an empty list and return it
list_t *make_empty_list(void);

// free a list
void free_list(list_t *list);

// insert a value at the foot of the list
list_t *insert_at_foot(list_t *list, data_t value);
/* same as #include "listops.c" */
