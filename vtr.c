#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>

#define MONSTER_COUNT 10

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
#define PATH "\033[34mX\033[0m"

enum keys {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN,
	LEFT_CLICK,
	RIGHT_CLICK,
	SCROLL_UP,
	SCROLL_DOWN
};

typedef struct {
	int y;
	int x;
	int health;
	int attack;
} monster_t;

typedef struct {
	char name[256];
	char icon[4];
	int count;
	int attack;
} item_t;

typedef struct {
	int max_y;
	int max_x;
	monster_t monsters[MONSTER_COUNT];
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


typedef struct {
	int x, y;
	int dist;
} node_t;

/* Directions for movement (up, down, left, right) */
int directions[4][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}};

world_t world;
player_t player;

char *statusline;
int mouse_row, mouse_col;
pthread_mutex_t world_lock;

void *clear_status_thread(void *arg);
void set_status(char *new_status);

void initialize_world(void)
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
	player.slot = 1;
	player.inventory[0] = (item_t) { "Dirt", DIRT, 0, 1 };
	player.inventory[1] = (item_t) { "Wood", WOOD, 0, 1 };
	player.inventory[2] = (item_t) { "Stone", STONE, 0, 1 };
	player.inventory[3] = (item_t) { "Diamond", DIAMOND, 0, 1 };
	player.inventory[4] = (item_t) { "Ruby", RUBY, 0, 1 };
	player.inventory[5] = (item_t) { "Pickaxe", PICKAXE, 0, 1 };
	player.inventory[6] = (item_t) { "Axe", AXE, 0, 5 };
	player.inventory[7] = (item_t) { "Sword", SWORD, 0, 5 };
	player.inventory[8] = (item_t) { "Shovel", SHOVEL, 0, 0 };
	player.y = world.max_y / 2;
	player.x = world.max_x / 2;
	/* Initialise player position */
	world.grid[player.y][player.x] = 'P';

	/* Initialize monsters */
	for (int i = 0; i < MONSTER_COUNT; i++) {
		world.monsters[i].x = rand() % world.max_x;
		world.monsters[i].y = rand() % world.max_y;
		world.monsters[i].health = 10;
		world.monsters[i].attack = 1;
		world.grid[world.monsters[i].y][world.monsters[i].x] = 'M';
	}
}

void print_icon_boxes(void)
{
	const int num_boxes = 9;

	printf("\033[2K\033[%dC", world.max_x / 2 - (5 * 9) / 2);
	for (int i = 0; i < num_boxes; i++) {
		if (i + 1 == player.slot) {
			printf("┏━━━┓"); /* Bold for selected box */
		} else {
			printf("┌───┐");
		}
	}
	printf("\n");

	printf("\033[2K\033[%dC", world.max_x / 2 - (5 * 9) / 2);
	for (int i = 0; i < num_boxes; i++) {
		if (i + 1 == player.slot) {
			printf("┃ %s ┃", player.inventory[i].icon); /* Bold for selected box */
		} else {
			printf("│ %s │", player.inventory[i].icon);
		}
	}
	printf("\n");

	printf("\033[2K\033[%dC", world.max_x / 2 - (5 * 9) / 2);
	for (int i = 0; i < num_boxes; i++) {
		if (i + 1 == player.slot) {
			printf("┗━━━┛"); /* Bold for selected box */
		} else {
			printf("└───┘");
		}
	}
}

void print_world(void)
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
				case 'X':
					printf("%s", PATH);
					break;
				default:
					printf("\033[38;2;0;0;255m=\033[0m");
					break;
			}
		}
		printf("\n");
	}
	/* Center text depending on length */
	printf("\033[2K\033[%dC" HEART " : %d " SKULL " : %d " INVENTORY " : %d\n", world.max_x / 2 - 13, player.health, player.kills, player.inventory[player.slot - 1].count);
	printf("\033[2K\033[%luC%s\n", world.max_x / 2 - strlen(statusline) / 2, statusline);
	print_icon_boxes();
	fflush(stdout);
	pthread_mutex_unlock(&world_lock);
}

void dijkstra(int start_x, int start_y, int end_x, int end_y)
{
	int dist[world.max_y][world.max_x];
	bool visited[world.max_y][world.max_x];

	/* Initialize distances */
	for (int i = 0; i < world.max_y; i++) {
		for (int j = 0; j < world.max_x; j++) {
			dist[i][j] = INT_MAX;
			visited[i][j] = false;

		}
	}
	/* Starting distance is zero */
	dist[start_y][start_x] = 0;

	/* Priority queue for Dijkstra */
	node_t queue[world.max_x * world.max_y];
	int front = 0, rear = 0;
	
	queue[rear++] = (node_t){start_x, start_y, 0};

	while (front < rear) {
		node_t current = queue[front++];
		int x = current.x, y = current.y;
		
		if (visited[y][x]) continue;
		visited[y][x] = true;

		if (x == end_x && y == end_y) break;

		/* Explore neighbors */
		for (int i = 0; i < 4; i++) {
			int nx = x + directions[i][0];
			int ny = y + directions[i][1];

			if (nx >= 0 && ny >= 0 && nx < world.max_x && ny < world.max_y && world.grid[ny][nx] != 'T' && world.grid[ny][nx] != 'S' && world.grid[ny][nx] != 'D' && world.grid[ny][nx] != 'P' && world.grid[ny][nx] != 'M') {
				int alt = dist[y][x] + 1;
				if (alt < dist[ny][nx]) {
					dist[ny][nx] = alt;
					queue[rear++] = (node_t){nx, ny, alt};
				}
			}
		}
	}

	/* Backtrace from end to start to mark path */
	int x = end_x, y = end_y;
	while (!(x == start_x && y == start_y)) {
		/* Mark path on grid */
		world.grid[y][x] = 'X';
		
		int min_dist = dist[y][x];
		for (int i = 0; i < 4; i++) {
			int nx = x + directions[i][0];
			int ny = y + directions[i][1];
			if (nx >= 0 && ny >= 0 && nx < world.max_x && ny < world.max_y && dist[ny][nx] < min_dist) {
				min_dist = dist[ny][nx];
				x = nx;
				y = ny;
			}
		}
	}
}

int find_monster(int y)
{
	for (int i = 0; i < MONSTER_COUNT; i++) {
		if (world.monsters[i].y == y) {
			return i;
		}
	}
	return -1;
}

void destroy_block(void)
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
							continue;
						} else {
							player.inventory[0].count += 1;
						}
						break;
					case 'T':
						if (strncmp(player.inventory[player.slot - 1].name, "Axe", 3) != 0) {
							set_status("You must use an axe to mine wood!");
							continue;
						} else {
							player.inventory[1].count += 1;
						}
						player.inventory[1].count += 1;
						break;
					case 'S':
						if (strncmp(player.inventory[player.slot - 1].name, "Pickaxe", 7) != 0) {
							set_status("You must use a pickaxe to mine stone!");
							continue;
						} else {
							player.inventory[2].count += 1;
						}
						break;
					case 'M':;
						int index = find_monster(ny);
						if (index != -1) {
							world.monsters[index].health -= player.inventory[player.slot - 1].attack;
							if (world.monsters[index].health <= 0) {
								break;
							}
						}
						continue;
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

void move_monsters(void)
{
	for (int i = 0; i < MONSTER_COUNT; i++) {
		if (world.monsters[i].health <= 0) continue;
		/* Clear current monster position */
		world.grid[world.monsters[i].y][world.monsters[i].x] = ' ';

		/* Try a maximum of 4 random movements to avoid collision */
		int new_x, new_y, attempts = 0;
		do {
			/* Random movement for monsters */
			new_x = (world.monsters[i].x + (rand() % 3 - 1) + world.max_x) % world.max_x;
			new_y = (world.monsters[i].y + (rand() % 3 - 1) + world.max_y) % world.max_y;
			attempts++;
		} while ((world.grid[new_y][new_x] != ' ') && attempts < 4);

		/* Only update if the chosen position is empty */
		if (world.grid[new_y][new_x] == ' ') {
			world.monsters[i].x = new_x;
			world.monsters[i].y = new_y;
		}

		/* Update monster position */
		world.grid[world.monsters[i].y][world.monsters[i].x] = 'M';
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

int read_key(void)
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1) {
			perror("read");
			return -1;
		}
	}

	if (c == '\033') {
		char seq[14];

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\033';
		if (seq[0] == '[') {
			if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\033';

			/* Mouse sequence */
			if (seq[1] == '<') {
				static int scroll_counter = 0;

				int i = 2;
				while (i < 14 && read(STDIN_FILENO, &seq[i], 1) == 1) {
					if (seq[i] == 'M') {
						i++;
						break;
					} else if (seq[i] == 'm'){
						return '\033';
					}
					i++;
				}
				seq[i] = '\0';  // Ensure the sequence is null-terminated

				int button = -1;
				sscanf(seq, "[<%d;%d;%d", &button, &mouse_col, &mouse_row);
				switch (button) {
					case 64:
						scroll_counter++;
						if (scroll_counter == 3) {
							scroll_counter = 0;
							return SCROLL_DOWN;
						}
						break;
					case 65:
						scroll_counter++;
						if (scroll_counter == 3) {
							scroll_counter = 0;
							return SCROLL_UP;
						}
						break;
					case 0:
						return LEFT_CLICK;
						break;
					case 2:
						return RIGHT_CLICK;
						break;
					default:
						scroll_counter = 0;
						break;
				}
			}
			else if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\033';
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			} else {
				switch (seq[1]) {
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'F': return END_KEY;
					case 'H': return HOME_KEY;
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
				case 'F': return END_KEY;
				case 'H': return HOME_KEY;
			}
		}
		return '\033';
	} else {
		return c;
	}
}

void *player_input(void *arg)
{
	/* Hide cursor, enable mouse reporting */
	printf("\033[?25l\033[?1000;1006h");

	struct termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	/* Disable canonical mode and echo */
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	int loop = 1;
	while (loop) {
		int c = read_key();
		switch (c) {
			case 'q':
				loop = 0;
				break;

			case ' ':
				destroy_block();
				break;

			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				player.slot = c - '0';
				break;

			case SCROLL_UP:
				player.slot = (player.slot % 9) + 1;
				break;

			case SCROLL_DOWN:
				player.slot = (player.slot - 2 + 9) % 9 + 1;
				break;

			case LEFT_CLICK:
				destroy_block();
				break;

			case RIGHT_CLICK:
				break;

			case 'x':
				dijkstra(0, 0, 168, 43);
				break;
			
			case '\033':
				break;

			default:
				move_player(c);
				break;
		}
		print_world();
	}

	/* Show cursor and disable mouse reporting */
	printf("\n\033[?25h\033[?1000;1006l");

	/* Restore old terminal settings */
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	exit(1);
	return NULL;
}

int main(void)
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
