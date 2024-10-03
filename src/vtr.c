#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MONSTER_COUNT 2

typedef struct {
	int wood;
	int stone;
	int dirt;
} inventory_t;

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
	inventory_t inv;
} player_t;

world_t world;
player_t player;

char *statusline;
pthread_mutex_t world_lock;

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

void print_world()
{
	pthread_mutex_lock(&world_lock);
	printf("\033[H");
	for (int i = world.max_y - 1; i >= 0; i--) {
		for (int j = 0; j < world.max_x; j++) {
			printf("%c", world.grid[i][j]);
		}
		printf("\n");
	}
	printf("Health: %d Kills: %d Inventory: %d Dirt %d Wood %d Stone \n", player.health, player.kills, player.inv.dirt, player.inv.wood, player.inv.stone);
	printf("%s", statusline);
	fflush(stdout);
	pthread_mutex_unlock(&world_lock);
}

void gather_resources()
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
				if (tile == 'T') {
					player.inv.wood += 1;
					world.grid[ny][nx] = ' ';
				} else if (tile == 'S') {
					player.inv.stone += 1;
					world.grid[ny][nx] = ' ';
				} else if (tile == 'D') {
					player.inv.dirt += 1;
					world.grid[ny][nx] = ' ';
				}
			}
		}
	}
}

void *clear_status_thread(void *arg)
{
	/* Wait for 2 seconds */
	sleep(2);
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

	switch (world.grid[new_y][new_x]) {
		case 'M':
			set_status("There's a monster in the way!");
			return;
		case 'T':
			set_status("There's a tree in the way!");
			return;
		case 'S':
			set_status("There's a stone in the way!");
			return;
		case 'D':
			set_status("There's a dirt in the way!");
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

		/* Random movement for monsters */
		world.monster_x[i] = (world.monster_x[i] + (rand() % 3 - 1) + world.max_x) % world.max_x;
		world.monster_y[i] = (world.monster_y[i] + (rand() % 3 - 1) + world.max_y) % world.max_y;

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
		} else if (ch == 'g') {
			gather_resources();
		} else {
			move_player(ch);
		}

		print_world();
	}

	/* Restore old terminal settings */
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
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
	world.max_y = lines - 2;
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
