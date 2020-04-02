// get and post page
bool get_page(int sockfd, char *GET_file_name, int *round);

bool post_page(int sockfd, char *POST_file_name, list_t *list, char *print_word, bool username_bool,
        bool cookie_boolean, int *round, int *next_cookie);