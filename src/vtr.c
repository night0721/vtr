#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MONSTER_COUNT 2

#define GOLD "\U000F124F"
#define BRICK "\U000F1288"
#define DIRT "\U000F1A48"
#define WOOD "\U0000F06C"
#define STONE "\U0000EB48"
#define WATER "\U000F058C"
#define TREE "\U0000E21C"
#define DIAMOND "\U000F01C8"
#define HEART "\U0000F004"
#define RUBY "\U0000E23E"
#define PICKAXE "\U000F08B7"
#define AXE "\U000F08C8"
#define SWORD "\U000F04E5"
#define SHOVEL "\U000F0710"
#define CHESTPLATE "\U0000EB0E"
#define SHOES "\U000F15C7"
#define SKULL "\U000F068C"
#define INVENTORY "\U0000F187"
#define PLAYER "\U0000EA67"
#define MONSTER "\U0000F23C"

typedef struct {
	char name[256];
	char icon[4];
	int count;
} item_t;

typedef struct {
	int max_y;
	int max_x;
	int monster_x[MONSTER_COUNT];
	int monster_y[MONSTER_COUNT];
	char **grid;
} world_t;

typedef struct {
	int y;
	int x;
	int health;
	int kills;
	item_t inventory[45];
	int slot;
} player_t;

world_t world;
player_t player;

char *statusline;
pthread_mutex_t world_lock;

void *clear_status_thread(void *arg);
void set_status(char *new_status);

void initialize_world()
{
	for (int i = 0; i < world.max_y; i++) {
		for (int j = 0; j < world.max_x; j++) {
			int rand_value = rand() % 100;
			if (rand_value < 1) {
				/* 1% chance for a tree */
				world.grid[i][j] = 'T';
			} else if (rand_value < 2) {
				/* 1% chance for stone */
				world.grid[i][j] = 'S'; 
			} else if (rand_value < 3) {
				/* 1% chance for dirt */
				world.grid[i][j] = 'D';
			} else {
				/* Empty space */
				world.grid[i][j] = ' ';
			}
		}
	}

	player.health = 100;
	player.inventory[0] = (item_t) { "Dirt", DIRT, 0 };
	player.inventory[1] = (item_t) { "Wood", WOOD, 0 };
	player.inventory[2] = (item_t) { "Stone", STONE, 0 };
	player.inventory[3] = (item_t) { "Diamond", DIAMOND, 0 };
	player.inventory[4] = (item_t) { "Ruby", RUBY, 0 };
	player.inventory[5] = (item_t) { "Pickaxe", PICKAXE, 0 };
	player.inventory[6] = (item_t) { "Axe", AXE, 0 };
	player.inventory[7] = (item_t) { "Sword", SWORD, 0 };
	player.inventory[8] = (item_t) { "Shovel", SHOVEL, 0 };
	player.y = world.max_y / 2;
	player.x = world.max_x / 2;
	/* Initialise player position */
	world.grid[player.y][player.x] = 'P';

	/* Initialize monsters */
	for (int i = 0; i < MONSTER_COUNT; i++) {
		world.monster_x[i] = rand() % world.max_x;
		world.monster_y[i] = rand() % world.max_y;
		/* Monster position */
		world.grid[world.monster_y[i]][world.monster_x[i]] = 'M';
	}
}

void print_icon_boxes()
{
	const int num_boxes = 9;

	printf("\033[2K\033[%dC", world.max_x / 2 - (5 * 9) / 2);
	for (int i = 0; i < num_boxes; i++) {
		if (i + 1 == player.slot) {
			printf("\033[1m┌───┐\033[0m"); /* Bold for selected box */
		} else {
			printf("┌───┐");
		}
	}
	printf("\n");

	printf("\033[2K\033[%dC", world.max_x / 2 - (5 * 9) / 2);
	for (int i = 0; i < num_boxes; i++) {
		if (i + 1 == player.slot) {
			printf("\033[1m│ \033[38;2;255;0;0m%s\033[0m\033[1m │\033[0m", player.inventory[i].icon, player.inventory[i].count); /* Bold for selected box */
		} else {
			printf("│ %s │", player.inventory[i].icon, player.inventory[i].count);
		}
	}
	printf("\n");

	printf("\033[2K\033[%dC", world.max_x / 2 - (5 * 9) / 2);
	for (int i = 0; i < num_boxes; i++) {
		if (i + 1 == player.slot) {
			printf("\033[1m└───┘\033[0m"); /* Bold for selected box */
		} else {
			printf("└───┘");
		}
	}
}

void print_world()
{
	pthread_mutex_lock(&world_lock);
	printf("\033[H");
	for (int i = world.max_y - 1; i >= 0; i--) {
		for (int j = 0; j < world.max_x; j++) {
			switch(world.grid[i][j]) {
				case 'P':
					printf("%s", PLAYER);
					break;
				case 'T':
					printf("%s", TREE);
					break;
				case 'S':
					printf("%s", STONE);
					break;
				case 'D':
					printf("%s", DIRT);
					break;
				case 'M':
					printf("%s", MONSTER);
					break;
				default:
					printf("%c", world.grid[i][j]);
					break;
			}
		}
		printf("\n");
	}
	printf("\033[2K\033[%dC" HEART " : %d " SKULL " : %d " INVENTORY " : %d\n", world.max_x / 2 - 8, player.health, player.kills, player.inventory[player.slot - 1].count);
	printf("\033[2K\033[%dC%s\n", world.max_x / 2 - strlen(statusline) / 2, statusline);
	print_icon_boxes();
	fflush(stdout);
	pthread_mutex_unlock(&world_lock);
}

void destory_block()
{
	int y = player.y;
	int x = player.x;

	/* Check surrounding blocks for resources */
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			/* Skip the player's current position */
			if (dy == 0 && dx == 0) continue;

			int ny = y + dy;
			int nx = x + dx;

			/* Check bounds */
			if (ny >= 0 && ny < world.max_y && nx >= 0 && nx < world.max_x) {
				char tile = world.grid[ny][nx];
				switch (tile) {
					case 'D':
						if (strncmp(player.inventory[player.slot - 1].name, "Shovel", 6) != 0) {
							set_status("You must use a shovel to mine dirt!");
							return;
						} else {
							player.inventory[0].count += 1;
						}
						break;
					case 'T':
						if (strncmp(player.inventory[player.slot - 1].name, "Axe", 6) != 0) {
							set_status("You must use an axe to mine wood!");
							return;
						} else {
							player.inventory[1].count += 1;
						}
						player.inventory[1].count += 1;
						break;
					case 'S':
						if (strncmp(player.inventory[player.slot - 1].name, "Pickaxe", 6) != 0) {
							set_status("You must use a pickaxe to mine stone!");
							return;
						} else {
							player.inventory[2].count += 1;
						}
						break;
				}
				world.grid[ny][nx] = ' ';
			}
		}
	}
}

void *clear_status_thread(void *arg)
{
	/* Wait for 2 seconds */
	sleep(1);
	pthread_mutex_lock(&world_lock);
	/* Clear the status line */
	strcpy(statusline, "");
	pthread_mutex_unlock(&world_lock);
	return NULL;
}

void set_status(char *new_status)
{
	pthread_mutex_lock(&world_lock);
	/* Set the new status */
	strcpy(statusline, new_status);
	pthread_mutex_unlock(&world_lock);

	pthread_t status_thread;
	/* Start a thread to clear status asynchronously */
	pthread_create(&status_thread, NULL, clear_status_thread, NULL);
	/* Detach thread so it cleans up itself after execution */
	pthread_detach(status_thread);
}

void move_player(char direction)
{
	int new_x = player.x;
	int new_y = player.y;

	/* Determine the target position based on the direction */
	switch (direction) {
		case 'w':
			new_y = (player.y < world.max_y - 1) ? player.y + 1 : player.y;
			break;
		case 's':
			new_y = (player.y > 0) ? player.y - 1 : player.y;
			break;
		case 'a':
			new_x = (player.x > 0) ? player.x - 1 : player.x;
			break;
		case 'd':
			new_x = (player.x < world.max_x - 1) ? player.x + 1 : player.x;
			break;
		default:
			break;
	}

	/* If the block is not empty don't overlap it */
	if (world.grid[new_y][new_x] != ' ') {
		return;
	}

	/* Clear current player position */
	world.grid[player.y][player.x] = ' ';

	/* Update player position */
	player.x = new_x;
	player.y = new_y;
	world.grid[player.y][player.x] = 'P';
}

void move_monsters()
{
	for (int i = 0; i < MONSTER_COUNT; i++) {
		/* Clear current monster position */
		world.grid[world.monster_y[i]][world.monster_x[i]] = ' ';

		/* Try a maximum of 4 random movements to avoid collision */
		int new_x, new_y, attempts = 0;
		do {
			/* Random movement for monsters */
			new_x = (world.monster_x[i] + (rand() % 3 - 1) + world.max_x) % world.max_x;
			new_y = (world.monster_y[i] + (rand() % 3 - 1) + world.max_y) % world.max_y;
			attempts++;
		} while ((world.grid[new_y][new_x] != ' ') && attempts < 4);

		/* Only update if the chosen position is empty */
		if (world.grid[new_y][new_x] == ' ') {
			world.monster_x[i] = new_x;
			world.monster_y[i] = new_y;
		}

		/* Update monster position */
		world.grid[world.monster_y[i]][world.monster_x[i]] = 'M';
	}
}

void *monster_movement(void *arg)
{
	while (1) {
		move_monsters();
		print_world();
		usleep(500000);
	}
	return NULL;
}

void *player_input(void *arg)
{
	struct termios oldt, newt;

	/* Setup non-blocking input */
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	/* Disable canonical mode and echo */
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	while (1) {
		char ch = getchar();
		if (ch == 'q') {
			break;
		} else if (ch == ' ') {
			destory_block();
		} else if (isdigit(ch) > 0) {
			player.slot = ch - '0';
		} else {
			move_player(ch);
		}

		print_world();
	}

	/* Restore old terminal settings */
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	printf("\n\033[?25h");
	exit(1);
	return NULL;
}

int main()
{
	srand(time(NULL));
	pthread_t monster_thread, input_thread;

	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	int lines = w.ws_row;
	int columns = w.ws_col;

	world.max_x = columns;
	world.max_y = lines - 5;
	world.grid = malloc(world.max_y * sizeof(char *));

	char status[columns];
	memset(status, 0, columns);
	statusline = status;

	for (int i = 0; i < world.max_y; i++) {
		world.grid[i] = malloc(columns);
	}

	/* Hide cursor */
 	printf("\033[?25l");
	pthread_mutex_init(&world_lock, NULL);
	initialize_world();
	print_world();

	/* Create threads for monster movement and player input */
	pthread_create(&monster_thread, NULL, monster_movement, NULL);
	pthread_create(&input_thread, NULL, player_input, NULL);

	/* Wait for threads to finish */
	pthread_join(input_thread, NULL);
	pthread_join(monster_thread, NULL);

	return 0;
}
