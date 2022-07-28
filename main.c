#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "SDL.h"
#include "SDL_timer.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#define W_HEIGHT 600
#define W_WIDTH (W_HEIGHT + (W_HEIGHT * 0.5))
#define TITLE_SIZE W_HEIGHT / 7.5
#define INSTRUCTION_SIZE TITLE_SIZE / 2
#define FONT "res/yoster.ttf"

#define TITLE "SQUARE JUMP"
#define STR_INSTRUCTION_1 "S TO START"
#define STR_INSTRUCTION_2 "SPACE TO JUMP FORWARD"
#define STR_INSTRUCTION_3 "RIGHT ARROW KEY TO MOVE FORWARD"

#define INITIAL_FLOOR_HEIGHT W_HEIGHT * 0.3
#define INITIAL_FLOOR_Y W_HEIGHT - INITIAL_FLOOR_HEIGHT
#define PLAYER_WIDTH W_WIDTH / 15
#define PLAYER_HEIGHT W_HEIGHT / 15
#define PLAYER_SPEED 2.5

#define JUMP_DILATION 0.01
#define HALF_JUMP_WIDTH PLAYER_HEIGHT * 3
#define JUMP_SPEED 2.5

#define NUM_OBSTACLES 50
#define OBSTACLE_WIDTH W_WIDTH / 10
#define XTEND_OBSTACLE_WIDTH W_WIDTH / 2
#define OBSTACLE_HEIGHT W_HEIGHT / 8
#define XTEND_OBSTACLE_HEIGHT W_HEIGHT / 3
#define HOLE_WIDTH HALF_JUMP_WIDTH * 1.5
#define XTEND_HOLE_WIDTH HALF_JUMP_WIDTH * 3.5
#define HOLE_HEIGHT INITIAL_FLOOR_HEIGHT
#define HOLE_Y INITIAL_FLOOR_Y
#define OBSTACLE_SPACING W_WIDTH / 5
#define XTEND_OBSTACLE_SPACING W_WIDTH / 2

#define GAME_OVER "GAME OVER"
#define GAME_OVER_DISPLAY_TIME 5000

#define TRUE 1
#define FALSE 0

#define DELAY_FACTOR 10

#define FLOOR_Y INITIAL_FLOOR_Y

#define LEVEL "LEVEL"
#define LEVEL_DIGITS 2
#define LEVEL_SIZE 24
#define ASCII_A 65


double floor_y = INITIAL_FLOOR_Y;
int is_alive = TRUE;
int player_speed = PLAYER_SPEED;
int jump_speed = JUMP_SPEED;
int hole_collision = FALSE;
double fall_width_x = 0;
int level = ASCII_A;


void render_pre_play(SDL_Renderer* renderer, SDL_Rect* bg_rect, TTF_Font* title_font, TTF_Font* message_font, SDL_Colour font_colour,
	SDL_Colour bg_colour);
SDL_Rect* get_rect(int start_coordinate_x, int start_coordinate_y, int width, int height);
void render_in_play(SDL_Renderer* renderer, SDL_Rect* bg_rect, SDL_Rect* player_rect, SDL_Rect* floor_rect, SDL_Colour bg_colour, SDL_Colour player_colour,
	SDL_Colour floor_colour, SDL_Rect** obstacles, TTF_Font* title_font, SDL_Colour font_colour);
void jump(SDL_Renderer* renderer, SDL_Rect* bg_rect, SDL_Rect* player_rect, SDL_Rect* floor_rect, SDL_Colour bg_colour, SDL_Colour player_colour,
	SDL_Colour floor_colour, SDL_Rect** obstacles, SDL_Event event, SDL_Window* window, TTF_Font* title_font, SDL_Colour font_colour);
SDL_Rect** get_obstacles(void);
void shift_obstacles(SDL_Rect* player_rect, SDL_Rect** obstacles, int shift);
void screen_scroll(SDL_Rect* player_rect, SDL_Rect* floor_rect, SDL_Rect** obstacles, int is_jump, int is_auto, SDL_Renderer* renderer,
	SDL_Colour player_colour);
void colliding_obstacle(SDL_Rect** obstacles, SDL_Renderer* renderer, SDL_Rect* bg_rect, SDL_Rect* player_rect, SDL_Rect* floor_rect,
	SDL_Colour bg_colour, SDL_Colour player_colour, SDL_Colour floor_colour, SDL_Event event, TTF_Font* title_font, SDL_Colour font_colour);
void loss(SDL_Renderer* renderer, SDL_Colour bg_colour, SDL_Colour font_colour, TTF_Font* title_font);
double fall(SDL_Renderer* renderer, SDL_Rect* bg_rect, SDL_Rect* player_rect, SDL_Rect* floor_rect, SDL_Colour bg_colour, SDL_Colour player_colour,
	SDL_Colour floor_colour, SDL_Rect** obstacles, SDL_Event event, SDL_Window* window, TTF_Font* title_font, SDL_Colour font_colour);
void is_within_bounds(SDL_Rect* player_rect);

int
main(int argc, char* argv[]) {
	
	assert(SDL_Init(SDL_INIT_TIMER || SDL_INIT_VIDEO || SDL_INIT_EVENTS) == 0);
	assert(TTF_Init() == 0);

	SDL_Window* window;
	SDL_Renderer* renderer;

	SDL_Rect* bg_rect = get_rect(0, 0, W_WIDTH, W_HEIGHT);
	SDL_Rect* player_rect = get_rect((W_WIDTH - PLAYER_WIDTH) / 2, floor_y - PLAYER_HEIGHT,
		PLAYER_WIDTH, PLAYER_HEIGHT);
	SDL_Rect* floor_rect = get_rect(0, floor_y, W_WIDTH, INITIAL_FLOOR_HEIGHT);

	TTF_Font* title_font = TTF_OpenFont(FONT, TITLE_SIZE);
	TTF_Font* level_font = TTF_OpenFont(FONT, LEVEL_SIZE);
	TTF_Font* message_font = TTF_OpenFont(FONT, INSTRUCTION_SIZE);

	SDL_Colour bg_colour = {0, 0, 0};
	SDL_Colour font_colour = {0, 255, 0};
	SDL_Colour player_colour = {0, 255, 0};
	SDL_Colour floor_colour = {0, 0, 255};

	SDL_Event event;

	int s_was_pressed = FALSE;
	int was_pre_play_rendered = FALSE;

	SDL_Rect** obstacles = get_obstacles();

	assert((window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W_WIDTH, W_HEIGHT, 0)) != NULL);
	assert((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED || SDL_RENDERER_PRESENTVSYNC)) != NULL);

	
	// Renders pre-game screen while `S` has not been pressed.
	while (!s_was_pressed) {
		if (!was_pre_play_rendered) {
			render_pre_play(renderer, bg_rect, title_font, message_font, font_colour, bg_colour);
			was_pre_play_rendered = TRUE;
		}
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_s) {
					s_was_pressed = TRUE;
				}
			}
			else if (event.key.keysym.sym == SDLK_ESCAPE) {
				SDL_DestroyWindow(window);
				exit(EXIT_SUCCESS);
			}
		}
	}

	SDL_RenderClear(renderer);

	// Logic for in-game play.
	while (is_alive) {
		render_in_play(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, level_font, font_colour);
		// Logic for player movement according to key that was pressed
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_SPACE) {
					jump(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, event, window, level_font, font_colour);
				}
				else if (event.key.keysym.sym == SDLK_RIGHT) {
					player_rect->x += player_speed;

					colliding_obstacle(obstacles, renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, event,
						level_font, font_colour);

					fall_width_x = fall(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, event, window,
						level_font, font_colour);
					screen_scroll(player_rect, floor_rect, obstacles, FALSE, FALSE, renderer, player_colour);
					render_in_play(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, level_font, font_colour);
				}
				else if (event.key.keysym.sym == SDLK_ESCAPE) {
					SDL_DestroyWindow(window);
					exit(EXIT_SUCCESS);
				}
				is_within_bounds(player_rect);
			}
		}
		is_within_bounds(player_rect);
		colliding_obstacle(obstacles, renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, event, 
			level_font, font_colour);
		fall_width_x = fall(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, event, window,
			level_font, font_colour);
		screen_scroll(player_rect, floor_rect, obstacles, FALSE, TRUE, renderer, player_colour);
	}

	while (!is_alive) {
		loss(renderer, bg_colour, font_colour, title_font);

		while (SDL_PollEvent(&event) != 0) {
			if (event.key.keysym.sym == SDLK_ESCAPE) {
				SDL_DestroyWindow(window);
				exit(EXIT_SUCCESS);
			}
		}
	}

	return 0;
}


/* Bounds checking. */
void
is_within_bounds(SDL_Rect* player_rect) {
	if (player_rect->x < 0 || player_rect->y < 0) {
		is_alive = FALSE;
	}
}


/* Renders loss message. */
void
loss(SDL_Renderer* renderer, SDL_Colour bg_colour, SDL_Colour font_colour, TTF_Font* title_font) {
	
	int str_width, str_height;
	SDL_Surface* s_game_over = TTF_RenderText_Solid(title_font, GAME_OVER, font_colour);
	SDL_Texture* t_game_over = SDL_CreateTextureFromSurface(renderer, s_game_over);

	assert(TTF_SizeText(title_font, TITLE, &str_width, &str_height) == 0);
	SDL_Rect* title_rect = get_rect((W_WIDTH - str_width) / 2, (W_HEIGHT - str_height) / 2,
		str_width, str_height);
	
	SDL_SetRenderDrawColor(renderer, bg_colour.r, bg_colour.g, bg_colour.b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, t_game_over, NULL, title_rect);

	SDL_RenderPresent(renderer);
}


/* Logic for screen scholling. */
void
screen_scroll(SDL_Rect* player_rect, SDL_Rect* floor_rect, SDL_Rect** obstacles, int is_jump, int is_auto, SDL_Renderer* renderer,
	SDL_Colour player_colour) {
	
	double mid = W_WIDTH / 2;

	// Screen scrolling triggered by user movement
	if (!is_auto) {
		if (is_jump && player_rect->x > mid) {
			player_rect->x -= jump_speed;
			shift_obstacles(player_rect, obstacles, jump_speed);
		}
		else if (is_jump && player_rect->x < mid) {
			shift_obstacles(player_rect, obstacles, jump_speed);
		}
		else if (player_rect->x > mid) {
			player_rect->x -= player_speed;
			shift_obstacles(player_rect, obstacles, player_speed);
		}
		else {
			shift_obstacles(player_rect, obstacles, player_speed);
		}
	}

	// Automatic screen scrolling
	else {
		player_rect->x -= player_speed / 2;
		shift_obstacles(player_rect, obstacles, player_speed / 2);

		SDL_Delay(DELAY_FACTOR);
	}
}


/* Detects collision between player and obstacles. Adjusts `floor_y` as necessary. */
void
colliding_obstacle(SDL_Rect** obstacles, SDL_Renderer* renderer, SDL_Rect* bg_rect, SDL_Rect* player_rect, SDL_Rect* floor_rect,
	SDL_Colour bg_colour, SDL_Colour player_colour, SDL_Colour floor_colour, SDL_Event event, TTF_Font* level_font, SDL_Colour font_colour) {

	int i, is_colliding_obstacle = FALSE, x = 0, y, player_start_y = player_rect->y, has_jump = FALSE, obstacles_passed = 0;
	if (is_alive) {
		for (i = 0; i < NUM_OBSTACLES; i++) {
			// Case 1: Player is on top of obstacle
			if ((player_rect->y <= (obstacles[i]->y - PLAYER_HEIGHT)) && (player_rect->x > (obstacles[i]->x - PLAYER_WIDTH)) &&
				(player_rect->x < (obstacles[i]->x + obstacles[i]->w)) && (obstacles[i]->y != HOLE_Y)) {
				hole_collision = FALSE;
				floor_y = obstacles[i]->y;
				break;
			}
			// Case 2: Player collides with obstacle
			else if ((SDL_HasIntersection(player_rect, obstacles[i]) == SDL_TRUE) && (obstacles[i]->y != HOLE_Y)) {

				while (player_rect->y < (floor_y - PLAYER_HEIGHT)) {
					player_rect->y += player_speed;
					render_in_play(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, level_font, font_colour);
					SDL_Delay(DELAY_FACTOR);
				}
				player_rect->y = floor_y - PLAYER_HEIGHT;
				is_alive = FALSE;
				SDL_Delay(GAME_OVER_DISPLAY_TIME / 10);
				break;
			}
			// Case 3: Player collides with hole
			else if ((player_rect->y >= (HOLE_Y - PLAYER_HEIGHT)) && (player_rect->x > obstacles[i]->x) &&
				((player_rect->x + PLAYER_WIDTH) < (obstacles[i]->x + obstacles[i]->w)) && (obstacles[i]->y == HOLE_Y)) {
				hole_collision = TRUE;
				floor_y = W_HEIGHT + PLAYER_HEIGHT;
				break;
			}
			// Case 4: Player collides with floor
			else if ((player_rect->y >= (obstacles[i]->y - PLAYER_HEIGHT)) && (SDL_HasIntersection(player_rect, obstacles[i]) == SDL_TRUE)
				&& ((player_rect->x + PLAYER_WIDTH) >= (obstacles[i]->x + obstacles[i]->w)) && (obstacles[i]->y == HOLE_Y)) {
				floor_y = W_HEIGHT + PLAYER_HEIGHT;
				while (player_rect->y < (floor_y - PLAYER_HEIGHT)) {
					player_rect->y += jump_speed;
					render_in_play(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, level_font, font_colour);
					SDL_Delay(DELAY_FACTOR);
				}

				player_rect->y = floor_y - PLAYER_HEIGHT;
				is_alive = FALSE;
				SDL_Delay(GAME_OVER_DISPLAY_TIME / 10);
				break;
			}
			// Cae 5: No collision
			else {
				hole_collision = FALSE;
				floor_y = FLOOR_Y;
			}

			if (obstacles[i]->x < player_rect->x) {
				obstacles_passed++;
			}
		}

		player_speed = PLAYER_SPEED + (obstacles_passed / 5);
		jump_speed = JUMP_SPEED + (obstacles_passed / 5);
		level = ASCII_A + obstacles_passed / 5;
	}
}



/* Logic for falling from a jump or obstacle. */
double
fall(SDL_Renderer* renderer, SDL_Rect* bg_rect, SDL_Rect* player_rect, SDL_Rect* floor_rect, SDL_Colour bg_colour, SDL_Colour player_colour,
	SDL_Colour floor_colour, SDL_Rect** obstacles, SDL_Event event, SDL_Window* window, TTF_Font* level_font, SDL_Colour font_colour) {

	double x = (!hole_collision) ? 0 : fall_width_x;
	double y, player_start_y = player_rect->y;

	while (is_alive && (player_rect->y < (floor_y - PLAYER_HEIGHT))) {

		x += jump_speed;
		y = (!hole_collision) ? -JUMP_DILATION * pow(x, 2) : -JUMP_DILATION * pow(x, 2) + JUMP_DILATION * pow(fall_width_x, 2);

		colliding_obstacle(obstacles, renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, event, level_font, font_colour);

		player_rect->x += jump_speed;
		player_rect->y = player_start_y - y;

		screen_scroll(player_rect, floor_rect, obstacles, TRUE, FALSE, renderer, player_colour);

		render_in_play(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, level_font, font_colour);

		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_SPACE) {
					jump(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, event, window, level_font, font_colour);
					return x;
				}
				else if (event.key.keysym.sym == SDLK_ESCAPE) {
					SDL_DestroyWindow(window);
					exit(EXIT_SUCCESS);
				}
			}
		}

		SDL_Delay(DELAY_FACTOR);
	}

	if (hole_collision && (player_rect->y >= (floor_y - PLAYER_HEIGHT))) {
		is_alive = FALSE;
		return x;
	}
	else {
		player_rect->y = floor_y - PLAYER_HEIGHT;
		return x;
	}
}


/* Logic for jumping. */
void
jump(SDL_Renderer* renderer, SDL_Rect* bg_rect, SDL_Rect* player_rect, SDL_Rect* floor_rect, SDL_Colour bg_colour, SDL_Colour player_colour,
	SDL_Colour floor_colour, SDL_Rect** obstacles, SDL_Event event, SDL_Window* window, TTF_Font* level_font, SDL_Colour font_colour) {
	
	double x = -HALF_JUMP_WIDTH, y, player_start_y = player_rect->y;

	do {
		x += jump_speed;
		y = -JUMP_DILATION * pow(x, 2);

		colliding_obstacle(obstacles, renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, event, level_font, font_colour);
		is_within_bounds(player_rect);

		player_rect->x += jump_speed;
		player_rect->y = player_start_y - y - (JUMP_DILATION * pow(HALF_JUMP_WIDTH, 2));
		
		screen_scroll(player_rect, floor_rect, obstacles, TRUE, FALSE, renderer, player_colour);

		render_in_play(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, level_font, font_colour);

		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_SPACE) {
					jump(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, event, window, level_font, font_colour);
					return;
				}
				else if (event.key.keysym.sym == SDLK_ESCAPE) {
					SDL_DestroyWindow(window);
					exit(EXIT_SUCCESS);
				}
			}
		}
		
		SDL_Delay(DELAY_FACTOR);
	} while (is_alive && (x < 0));
	
	if (is_alive) {
		colliding_obstacle(obstacles, renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, event, level_font, font_colour);
		fall_width_x = fall(renderer, bg_rect, player_rect, floor_rect, bg_colour, player_colour, floor_colour, obstacles, event, window, level_font, font_colour);
	}
}


/* Shifts all obstacles `n` pixels to the left. */
void
shift_obstacles(SDL_Rect* player_rect, SDL_Rect** obstacles, int n) {
	int i;
	for (i = 0; i < NUM_OBSTACLES; i++) {
		obstacles[i]->x -= n;
	}
}


/* Returns an array of SDL_Rect* corresponding to holes or obstacles of arbitrary size and position. */
SDL_Rect**
get_obstacles(void) {
	int i, rand_int, prev_obstacle_x = (W_WIDTH / 2) + OBSTACLE_SPACING, obstacle_x, obstacle_width, obstacle_height;
	SDL_Rect** obstacles = (SDL_Rect**)malloc(NUM_OBSTACLES * sizeof(SDL_Rect*));
	SDL_Rect* obstacle = NULL;

	srand(time(NULL));

	for (i = 0; i < NUM_OBSTACLES; i++) {
		rand_int = (rand() % 3);

		if (rand_int == 0) {
			obstacle_width = HOLE_WIDTH + (rand() % (int)XTEND_HOLE_WIDTH);
			obstacle_x = prev_obstacle_x + (rand() % (int)XTEND_OBSTACLE_SPACING);
			obstacle = get_rect(obstacle_x, HOLE_Y, obstacle_width, HOLE_HEIGHT);
			prev_obstacle_x = obstacle_x + obstacle_width + OBSTACLE_SPACING;
		}
		else if (rand_int == 1) {
			obstacle_width = OBSTACLE_WIDTH + (rand() % (int)XTEND_OBSTACLE_WIDTH);
			obstacle_height = OBSTACLE_HEIGHT + (rand() % (int)XTEND_OBSTACLE_HEIGHT);
			obstacle_x = prev_obstacle_x + (rand() % (int)XTEND_OBSTACLE_SPACING);
			obstacle = get_rect(obstacle_x, FLOOR_Y - obstacle_height,
				obstacle_width, obstacle_height);
			prev_obstacle_x = obstacle_x + obstacle_width + OBSTACLE_SPACING;
		}
		else if (rand_int == 2) {
			obstacle_width = OBSTACLE_WIDTH + (rand() % (int)XTEND_OBSTACLE_WIDTH);
			obstacle_height = OBSTACLE_HEIGHT + (rand() % (int)XTEND_OBSTACLE_HEIGHT);
			obstacle_height = (obstacle_height < (W_HEIGHT / 2)) ? (W_HEIGHT / 2) : obstacle_height;
			obstacle_x = prev_obstacle_x + (rand() % (int)XTEND_OBSTACLE_SPACING);
			obstacle = get_rect(obstacle_x, 0,
				obstacle_width, obstacle_height);
			prev_obstacle_x = obstacle_x + obstacle_width + OBSTACLE_SPACING;
		}
	
		obstacles[i] = obstacle;
	}

	return obstacles;
}


/* Renders in-game play. */
void
render_in_play(SDL_Renderer* renderer, SDL_Rect* bg_rect, SDL_Rect* player_rect, SDL_Rect* floor_rect, SDL_Colour bg_colour, SDL_Colour player_colour, 
	SDL_Colour floor_colour, SDL_Rect** obstacles, TTF_Font* level_font, SDL_Colour font_colour) {

	int i, is_hole, level_width, level_height, int_width, int_height;
	char* int_level = (char*)malloc(LEVEL_DIGITS * sizeof(char));
	SDL_Rect* obstacle;
	SDL_Colour* colour;

	int_level[0] = (char)level;
	int_level[1] = '\0';

	SDL_Surface* s_level = TTF_RenderText_Solid(level_font, LEVEL, font_colour);
	SDL_Texture* t_level = SDL_CreateTextureFromSurface(renderer, s_level);
	assert(TTF_SizeText(level_font, TITLE, &level_width, &level_height) == 0);
	SDL_Rect* level_rect = get_rect(0, (W_HEIGHT - level_height), level_width, level_height);

	SDL_Surface* s_level_int = TTF_RenderText_Solid(level_font, int_level, font_colour);
	SDL_Texture* t_level_int = SDL_CreateTextureFromSurface(renderer, s_level_int);
	assert(TTF_SizeText(level_font, int_level, &int_width, &int_height) == 0);
	SDL_Rect* level_int_rect = get_rect(level_width + (2 * int_width), (W_HEIGHT - level_height), int_width, int_height);
	
	SDL_SetRenderDrawColor(renderer, bg_colour.r, bg_colour.g, bg_colour.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(renderer, bg_rect);

	SDL_SetRenderDrawColor(renderer, floor_colour.r, floor_colour.g, floor_colour.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(renderer, floor_rect);

	for (i = 0; i < NUM_OBSTACLES; i++) {
		obstacle = obstacles[i];
		is_hole = (obstacles[i]->y == HOLE_Y) ? TRUE : FALSE;
		colour = (is_hole) ? &bg_colour : &floor_colour;

		SDL_SetRenderDrawColor(renderer, colour->r, colour->g, colour->b, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(renderer, obstacle);
	}

	SDL_SetRenderDrawColor(renderer, player_colour.r, player_colour.g, player_colour.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(renderer, player_rect);

	SDL_SetRenderDrawColor(renderer, font_colour.r, font_colour.g, font_colour.b, SDL_ALPHA_OPAQUE);
	SDL_RenderCopy(renderer, t_level, NULL, level_rect);
	SDL_RenderCopy(renderer, t_level_int, NULL, level_int_rect);
	
	SDL_RenderPresent(renderer);
}


/* Renders pre-game screen. */
void
render_pre_play(SDL_Renderer* renderer, SDL_Rect* bg_rect, TTF_Font* title_font, TTF_Font* message_font, SDL_Colour font_colour,
	SDL_Colour bg_colour) {

	int title_width, title_height, message_width, message_height;
		
	SDL_Surface* s_title = TTF_RenderText_Solid(title_font, TITLE, font_colour);
	SDL_Texture* t_title = SDL_CreateTextureFromSurface(renderer, s_title);
	assert(TTF_SizeText(title_font, TITLE, &title_width, &title_height) == 0);
	SDL_Rect* title_rect = get_rect((W_WIDTH - title_width) / 2, (W_HEIGHT - title_height) / 2, title_width, title_height);

	SDL_Surface* s_instruction_1 = TTF_RenderText_Solid(message_font, STR_INSTRUCTION_1, font_colour);
	SDL_Texture* t_instruction_1 = SDL_CreateTextureFromSurface(renderer, s_instruction_1);
	assert(TTF_SizeText(message_font, STR_INSTRUCTION_1, &message_width, &message_height) == 0);
	SDL_Rect* instruction_1_rect = get_rect((W_WIDTH - message_width) / 2, (W_HEIGHT - title_height) / 2 + title_height,
		message_width, message_height);

	SDL_Surface* s_instruction_2 = TTF_RenderText_Solid(message_font, STR_INSTRUCTION_2, font_colour);
	SDL_Texture* t_instruction_2 = SDL_CreateTextureFromSurface(renderer, s_instruction_2);
	assert(TTF_SizeText(message_font, STR_INSTRUCTION_2, &message_width, &message_height) == 0);
	SDL_Rect* instruction_2_rect = get_rect((W_WIDTH - message_width) / 2, instruction_1_rect->y + message_height,
		message_width, message_height);

	SDL_Surface* s_instruction_3 = TTF_RenderText_Solid(message_font, STR_INSTRUCTION_3, font_colour);
	SDL_Texture* t_instruction_3 = SDL_CreateTextureFromSurface(renderer, s_instruction_3);
	assert(TTF_SizeText(message_font, STR_INSTRUCTION_3, &message_width, &message_height) == 0);
	SDL_Rect* instruction_3_rect = get_rect((W_WIDTH - message_width) / 2, instruction_2_rect->y + message_height,
		message_width, message_height);

	SDL_SetRenderDrawColor(renderer, bg_colour.r, bg_colour.g, bg_colour.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(renderer, bg_rect);

	SDL_SetRenderDrawColor(renderer, font_colour.r, font_colour.g,font_colour.b, SDL_ALPHA_OPAQUE);
	SDL_RenderCopy(renderer, t_title, NULL, title_rect);
	SDL_RenderCopy(renderer, t_instruction_1, NULL, instruction_1_rect);
	SDL_RenderCopy(renderer, t_instruction_2, NULL, instruction_2_rect);
	SDL_RenderCopy(renderer, t_instruction_3, NULL, instruction_3_rect);

	SDL_RenderPresent(renderer);
}


/* Returns a SDL_Rect* with: top left vertex (start_coordinate_x, start_coordinate_y) and size `width` X `height`. */
SDL_Rect*
get_rect(int start_coordinate_x, int start_coordinate_y, int width, int height) {
	SDL_Rect* rectangle = (SDL_Rect*)malloc(sizeof(SDL_Rect));
	assert(rectangle != NULL);
	rectangle->x = start_coordinate_x;
	rectangle->y = start_coordinate_y;
	rectangle->w = width;
	rectangle->h = height;
	return rectangle;
}