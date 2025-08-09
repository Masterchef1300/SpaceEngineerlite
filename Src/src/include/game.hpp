// include/game.hpp
#pragma once
#include <windows.h>
#include <vector>

enum BlockType { BLOCK_EMPTY=0, BLOCK_ARMOR, BLOCK_THRUSTER, BLOCK_CORE, BLOCK_MINER };

struct Vec2 { double x,y; Vec2():x(0),y(0){} Vec2(double X,double Y):x(X),y(Y){} double len() const; Vec2 normalized() const; Vec2 operator*(double s) const; Vec2 operator+(const Vec2& o) const; Vec2 operator-(const Vec2& o) const; };

struct Block { BlockType type; int hp; Block():type(BLOCK_EMPTY),hp(0){} };

struct Ship {
    int core_r, core_c;
    double pos_x, pos_y;
    double angle;
    Vec2 vel;
    double angVel;
    std::vector<std::pair<int,int>> blocks;
    Ship();
};

struct Drone { double x,y; double angle; Vec2 vel; int hp; double cooldown; };

void game_init();
void game_shutdown();
bool game_is_running();
void game_request_quit();
bool game_is_paused();
bool game_is_over();
void game_update();

// accessors for renderer
int game_get_window_width();
int game_get_window_height();
int game_get_hud_width();

int game_get_resources();
int game_get_score();

void game_get_player_pos(double &x, double &y);
double game_get_player_angle();
const std::vector<Drone>& game_get_drones();
const Block* game_get_world_ptr(); // pointer to world grid [rows * cols]
int game_world_rows();
int game_world_cols();
