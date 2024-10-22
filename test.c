#include <stdio.h>

typedef struct player_t {
    int hp;
    int attack;
    int defense;
    int range;
    coordinate *pos;
} player_t;

typedef struct coordinate {
    int x;
    int y;
    int z;
} coordinate;
/*
 * Create a player_t instance
 */
player_t *
player_init()
{
    player_t *player = malloc(sizeof(player));
    if (player == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    player->hp = 100;
    player->attack = 10;
    player->defense = 10;
    player->range = 5;
    player->coordinate = malloc(sizeof(coordinate));
    if (player->coordinate == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return player;
}

int
check_enemy_in_range(player_t *player)
{
    for (int x = 0; x < player->range; x++) {
        for (int y = 0; y < player->range; y++) {
            for (int z = 0; z < player->range; z++) {
                if ()
            }
        }
    }
}
int
main()
{
    player_t *test_char = player_init();

}
