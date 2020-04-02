// get keywords of the  player with the cookie in the list
char *get_keywords(list_t *list, int cookie);

// return true if the player with the cookie is start
bool is_start(list_t *list, int cookie);

// return true if the player with the cookie is paired with another player
bool is_pair(list_t *list, int cookie);

// set_things of player with the cookie to be true or false
void set_ready(list_t *list, int cookie, bool set_bool);

void set_start(list_t *list, int cookie_1, int cookie_2, bool set_bool);

void set_gameover_page(list_t *list, int cookie, bool set_bool);

void set_endgame_page(list_t *list, int cookie, bool set_bool);

// pair 2 players who is ready and set them to be start
void pair_users(list_t *list);

// set variables of players in link list in case gameover or endgame
void gameover_setting(list_t *list, int cookie, node_t *current);

void endgame_setting(list_t *list, int cookie, node_t *current);

bool endgame_keyword(list_t *list, int cookie, char *keyword);

bool gameover_quit(list_t *list, int cookie);