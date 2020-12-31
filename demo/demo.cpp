#include <iostream>
#include <random>
#include <string>

#include <sgg/graphics.h>

#include "vecmath.h"

static const std::string assets_path = "assets/";

struct Game {
	int window_width = 1024;
	int window_height = 768;

	int bounces = 0;

	float canvas_width = 100.0f;
	float canvas_height = 100.0f;

	float ball_center_x = 50.0f;
	float ball_center_y = 50.0f;
	float ball_direction_x = 1.0f;
	float ball_direction_y = 0.0f;
	float ball_radius = 4.0f;

	float ball_speed = 0.5f;

	float spinner_angle = 0.0f;

	void init() {
		graphics::Brush br;
		br.fill_color[0] = 0.1f;
		br.fill_color[1] = 0.1f;
		br.fill_color[2] = 0.1f;
		graphics::setWindowBackground(br);
		graphics::setFont(assets_path + "orange juice 2.0.ttf");

		std::random_device seed;
		std::default_random_engine generator(seed());
		std::uniform_real_distribution<float> distribution(-1.0, 1.0);

		ball_direction_x = distribution(generator);
		ball_direction_y = distribution(generator);

		// Normalize
		float length = std::sqrt(ball_direction_x * ball_direction_x + ball_direction_y * ball_direction_y);
		ball_direction_x /= length;
		ball_direction_y /= length;
	}
};

void update(float ms)
{
	Game* game = (Game*)graphics::getUserData();

	game->spinner_angle++;

	float ball_future_center_x = game->ball_center_x + game->ball_direction_x * game->ball_speed;
	float ball_future_center_y = game->ball_center_y + game->ball_direction_y * game->ball_speed;
    bool play_collision_sound = false;

	if ((ball_future_center_x + game->ball_radius) >= game->canvas_width) {
		math::vec2 dir { game->ball_direction_x, game->ball_direction_y };
		math::vec2 normal = { -1.0f, 0.0f };
		math::vec2 reflection = math::reflect(dir, normal);
		game->ball_direction_x = reflection.x;
		game->ball_direction_y = reflection.y;
		play_collision_sound = true;
	}
	else if ((ball_future_center_x - game->ball_radius) <= 0.0f) {
		math::vec2 dir{ game->ball_direction_x, game->ball_direction_y };
		math::vec2 normal = { 1.0f, 0.0f };
		math::vec2 reflection = math::reflect(dir, normal);
		game->ball_direction_x = reflection.x;
		game->ball_direction_y = reflection.y;
		play_collision_sound = true;
	}

	if ((ball_future_center_y + game->ball_radius) >= game->canvas_height) {
		math::vec2 dir{ game->ball_direction_x, game->ball_direction_y };
		math::vec2 normal = { 0.0f, -1.0f };
		math::vec2 reflection = math::reflect(dir, normal);
		game->ball_direction_x = reflection.x;
		game->ball_direction_y = reflection.y;
		play_collision_sound = true;
	}
	else if ((ball_future_center_y - game->ball_radius) <= 0.0f) {
		math::vec2 dir{ game->ball_direction_x, game->ball_direction_y };
		math::vec2 normal = { 0.0f, 1.0f };
		math::vec2 reflection = math::reflect(dir, normal);
		game->ball_direction_x = reflection.x;
		game->ball_direction_y = reflection.y;
		play_collision_sound = true;
	}

	if (play_collision_sound) {
		std::string wav = assets_path + "hit1.wav";
		graphics::playSound(wav, 1.0f);
		game->bounces++;
	}

	game->ball_center_x += game->ball_direction_x * game->ball_speed;
	game->ball_center_y += game->ball_direction_y * game->ball_speed;
}

void drawText() {
	Game* game = (Game*)graphics::getUserData();

	graphics::Brush br;
	br.fill_secondary_color[0] = 1.0f;
	br.fill_secondary_color[1] = 1.0f;
	br.fill_secondary_color[2] = 1.0f;
	graphics::drawText(1.0f, 10.0f, 5, "Bounces: "+std::to_string(game->bounces), br );
}

void drawBall() {
	Game* game = (Game*)graphics::getUserData();

	graphics::Brush br;
	br.fill_color[0] = 1.0f;
	br.fill_color[1] = 1.0f;
	br.fill_color[2] = 1.0f;
	br.fill_opacity = 0.5f;
	br.outline_opacity = 0.0f;
	br.texture = assets_path + "iron.png";

	graphics::drawDisk(game->ball_center_x, game->ball_center_y, game->ball_radius, br);
}

void drawSpinner() {
	Game* game = (Game*)graphics::getUserData();

	graphics::Brush br;
	br.fill_color[0] = 1.0f;
	br.fill_color[1] = 0.0f;
	br.fill_color[2] = 0.0f;
	br.fill_opacity = 1.0f;

	graphics::setOrientation(game->spinner_angle);
	graphics::drawSector(90, 10, 2, 5, 0, 90, br);
	graphics::setOrientation(0);
}

void draw()
{
	drawBall();
	drawSpinner();
	drawText();
}

int main()
{
	Game game;

	graphics::createWindow(game.window_width, game.window_height, "C++ demo");
	//graphics::setFullScreen(true);
	
	game.init();

	graphics::setUserData(&game);
	graphics::setDrawFunction(draw);
	graphics::setUpdateFunction(update);
	
	graphics::setCanvasSize(game.canvas_width, game.canvas_height);
	graphics::setCanvasScaleMode(graphics::CANVAS_SCALE_FIT);

	graphics::startMessageLoop();
	graphics::destroyWindow();

	return 0;
}
