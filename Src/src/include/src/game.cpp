// src/game.cpp
#include "game.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

static const int WINDOW_W = 1280;
static const int WINDOW_H = 720;
static const int HUD_WIDTH = 260;
static const int GRID_CELL = 24;
static const int WORLD_COLS = 80;
static const int WORLD_ROWS = 50;

using namespace std;

// simple Vec2 impl
double Vec2::len() const { return sqrt(x*x + y*y); }
Vec2 Vec2::normalized() const { double l = len(); return l>1e-9 ? Vec2(x/l,y/l) : Vec2(0,0); }
Vec2 Vec2::operator*(double s) const { return Vec2(x*s, y*s); }
Vec2 Vec2::operator+(const Vec2& o) const { return Vec2(x+o.x, y+o.y); }
Vec2 Vec2::operator-(const Vec2& o) const { return Vec2(x-o.x, y-o.y); }

Ship::Ship(){ core_r=0; core_c=0; pos_x=0; pos_y=0; angle=0; vel=Vec2(); angVel=0; }

static bool g_running = true;
static bool g_paused = false;
static bool g_gameOver = false;
static int tickCount = 0;

static Block g_world[WORLD_ROWS][WORLD_COLS];
static Ship g_ship;
static int g_resources = 0;
static int g_score = 0;
static std::vector<Drone> g_drones;

// Forward helpers
inline bool inGrid(int r,int c){ return r>=0 && r<WORLD_ROWS && c>=0 && c<WORLD_COLS; }
static Vec2 rotateLocal(int grOffsetR, int grOffsetC, double angleRad){
    double localX = grOffsetC * GRID_CELL;
    double localY = grOffsetR * GRID_CELL;
    double s = sin(angleRad), c = cos(angleRad);
    double rx = localX * c - localY * s;
    double ry = localX * s + localY * c;
    return Vec2(rx, ry);
}

void resetWorld(){
    for(int r=0;r<WORLD_ROWS;r++) for(int c=0;c<WORLD_COLS;c++){ g_world[r][c].type = BLOCK_EMPTY; g_world[r][c].hp = 0; }
    srand((unsigned)time(NULL));
    for(int i=0;i<40;i++){
        int ar = rand() % WORLD_ROWS;
        int ac = rand() % (WORLD_COLS - 20) + 10;
        int size = rand() % 6 + 3;
        for(int dr=-size; dr<=size; dr++) for(int dc=-size; dc<=size; dc++){
            int rr = ar + dr; int cc = ac + dc;
            if(inGrid(rr,cc) && rand()%100 < 60){
                g_world[rr][cc].type = BLOCK_ARMOR;
                g_world[rr][cc].hp = 40 + rand()%40;
            }
        }
    }

    int start_c = 10, start_r = WORLD_ROWS/2;
    g_ship.blocks.clear();
    g_ship.core_r = start_r; g_ship.core_c = start_c;
    g_ship.pos_x = start_c * GRID_CELL + GRID_CELL/2;
    g_ship.pos_y = start_r * GRID_CELL + GRID_CELL/2;
    g_ship.angle = 0; g_ship.vel = Vec2(); g_ship.angVel = 0;
    g_ship.blocks.push_back({0,0});
    g_ship.blocks.push_back({0,1}); g_ship.blocks.push_back({0,-1});
    g_ship.blocks.push_back({1,0}); g_ship.blocks.push_back({-1,0});
    g_ship.blocks.push_back({1,1}); // thruster
    g_ship.blocks.push_back({-1,0}); // miner

    for(auto &off : g_ship.blocks){
        int rr = g_ship.core_r + off.first;
        int cc = g_ship.core_c + off.second;
        if(inGrid(rr,cc)){ g_world[rr][cc].type = BLOCK_EMPTY; g_world[rr][cc].hp = 0; }
    }

    g_resources = 60;
    g_score = 0;
    tickCount = 0;
    g_gameOver = false;
    g_drones.clear();
    for(int i=0;i<8;i++){
        Drone d; d.x = (WORLD_COLS - 8 - rand()%10) * GRID_CELL; d.y = (5 + rand()%(WORLD_ROWS-10)) * GRID_CELL;
        d.hp = 40 + rand()%60; d.angle = 0; d.vel = Vec2(); d.cooldown = 0; g_drones.push_back(d);
    }
}

void game_init(){
    resetWorld();
    g_running = true;
}
void game_shutdown(){ g_running = false; }
bool game_is_running(){ return g_running; }
void game_request_quit(){ g_running = false; }
bool game_is_paused(){ return g_paused; }
bool game_is_over(){ return g_gameOver; }

int game_get_window_width(){ return WINDOW_W; }
int game_get_window_height(){ return WINDOW_H; }
int game_get_hud_width(){ return HUD_WIDTH; }

int game_get_resources(){ return g_resources; }
int game_get_score(){ return g_score; }

void game_get_player_pos(double &x, double &y){ x = g_ship.pos_x; y = g_ship.pos_y; }
double game_get_player_angle(){ return g_ship.angle; }
const std::vector<Drone>& game_get_drones(){ return g_drones; }
const Block* game_get_world_ptr(){ return &g_world[0][0]; }
int game_world_rows(){ return WORLD_ROWS; }
int game_world_cols(){ return WORLD_COLS; }

// modifications: place/remove world functions used by input/render
bool placeBlockAtWorld(int r,int c, BlockType type){
    if(!inGrid(r,c)) return false;
    if(g_world[r][c].type != BLOCK_EMPTY) return false;
    g_world[r][c].type = type; g_world[r][c].hp = (type==BLOCK_ARMOR?60: (type==BLOCK_THRUSTER?30:20));
    return true;
}
bool removeBlockAtWorld(int r,int c){
    if(!inGrid(r,c)) return false;
    if(g_world[r][c].type == BLOCK_EMPTY) return false;
    int gain = 0;
    switch(g_world[r][c].type){ case BLOCK_ARMOR: gain=8; break; case BLOCK_THRUSTER: gain=12; break; case BLOCK_MINER: gain=10; break; default: gain=4; break; }
    g_resources += gain; g_world[r][c].type = BLOCK_EMPTY; g_world[r][c].hp = 0; return true;
}

// Internal update helpers
void applyShipForces(){
    Vec2 totalForce(0,0); double totalTorque = 0.0;
    // Simple AI-less forces: thruster present at offset (1,1)
    // For input-derived thrust we rely on input module to set flags via global accessible functions
    Vec2 thrustInput = get_input_thrust(); // declared in input.hpp
    double userMag = thrustInput.len();
    if(userMag > 1.0) thrustInput = thrustInput.normalized();

    for(auto &off : g_ship.blocks){
        bool isThruster = (off.first==1 && off.second==1);
        if(isThruster){
            Vec2 localDir(0,-1);
            double sA = sin(g_ship.angle), cA = cos(g_ship.angle);
            Vec2 worldDir(localDir.x * cA - localDir.y * sA, localDir.x * sA + localDir.y * cA);
            Vec2 f = worldDir * (40.0 * userMag);
            totalForce += f;
            Vec2 offsetWorld = rotateLocal(off.first, off.second, g_ship.angle);
            double torque = offsetWorld.x * f.y - offsetWorld.y * f.x;
            totalTorque += torque * 0.001;
        }
    }

    Vec2 drag = g_ship.vel * -0.6;
    totalForce += drag;
    Vec2 acceleration = totalForce * (1.0 / 10.0);
    g_ship.vel = g_ship.vel + acceleration * (1.0/60.0);
    g_ship.pos_x += g_ship.vel.x * (1.0/60.0) * 60.0;
    g_ship.pos_y += g_ship.vel.y * (1.0/60.0) * 60.0;
    g_ship.angVel += totalTorque * (1.0/60.0);
    g_ship.angVel *= 0.98;
    g_ship.angle += g_ship.angVel * (1.0/60.0);

    if(fabs(g_ship.vel.x) < 1e-3) g_ship.vel.x = 0;
    if(fabs(g_ship.vel.y) < 1e-3) g_ship.vel.y = 0;
    if(fabs(g_ship.angVel) < 1e-4) g_ship.angVel = 0;

    int newCoreC = (int)(g_ship.pos_x / GRID_CELL);
    int newCoreR = (int)(g_ship.pos_y / GRID_CELL);
    g_ship.core_c = std::max(0, std::min(WORLD_COLS-1,newCoreC));
    g_ship.core_r = std::max(0, std::min(WORLD_ROWS-1,newCoreR));
}

void shipMining(){
    for(auto &off : g_ship.blocks){
        bool isMiner = (off.first==-1 && off.second==0);
        if(!isMiner) continue;
        Vec2 local = rotateLocal(off.first, off.second, g_ship.angle);
        double wx = g_ship.pos_x + local.x;
        double wy = g_ship.pos_y + local.y;
        int gr = (int)(wy / GRID_CELL);
        int gc = (int)(wx / GRID_CELL);
        if(inGrid(gr,gc) && g_world[gr][gc].type == BLOCK_ARMOR){
            g_world[gr][gc].hp -= 1 + (rand()%3);
            if(g_world[gr][gc].hp <= 0){
                g_world[gr][gc].type = BLOCK_EMPTY; g_world[gr][gc].hp = 0; g_resources += 12; g_score += 8;
            }
        }
    }
}

void drones_update(){
    for(auto &d : g_drones){
        if(d.hp <= 0) continue;
        Vec2 playerPos(g_ship.pos_x, g_ship.pos_y);
        Vec2 dir = playerPos - Vec2(d.x,d.y);
        double dist = dir.len();
        dir = dir.normalized();
        double speed = 40.0;
        Vec2 desired = dir * speed;
        d.vel = d.vel + (desired - d.vel) * 0.06;
        d.x += d.vel.x * (1.0/60.0) * 60.0;
        d.y += d.vel.y * (1.0/60.0) * 60.0;

        if(dist < GRID_CELL*2.0){
            int br = (int)(d.y / GRID_CELL);
            int bc = (int)(d.x / GRID_CELL);
            if(inGrid(br,bc) && g_world[br][bc].type != BLOCK_EMPTY){
                g_world[br][bc].hp -= 6;
                if(g_world[br][bc].hp <= 0){ g_world[br][bc].type = BLOCK_EMPTY; g_world[br][bc].hp = 0; g_resources += 6; }
            } else {
                g_ship.vel = g_ship.vel + (g_ship.pos_x - d.x > 0 ? Vec2(2,0) : Vec2(-2,0));
                g_score -= 2;
            }
            d.hp -= 4;
        }
    }
    g_drones.erase(std::remove_if(g_drones.begin(), g_drones.end(), [](const Drone &d){ return d.hp <= 0; }), g_drones.end());
}

void world_collisions(){
    for(auto &off : g_ship.blocks){
        Vec2 local = rotateLocal(off.first, off.second, g_ship.angle);
        double wx = g_ship.pos_x + local.x;
        double wy = g_ship.pos_y + local.y;
        int gr = (int)(wy / GRID_CELL);
        int gc = (int)(wx / GRID_CELL);
        if(inGrid(gr,gc) && g_world[gr][gc].type != BLOCK_EMPTY){
            g_ship.vel = g_ship.vel * -0.3;
            g_world[gr][gc].hp -= 8;
            if(g_world[gr][gc].hp <= 0){ g_world[gr][gc].type = BLOCK_EMPTY; g_world[gr][gc].hp = 0; g_resources += 6; }
        }
    }
}

void game_update(){
    if(g_paused || g_gameOver) return;
    tickCount++;
    // inputs are handled in input.cpp and stored in module; apply forces using that input
    applyShipForces();
    shipMining();
    drones_update();
    world_collisions();

    if(tickCount % (60 * 6) == 0){
        if(rand()%100 < 40){
            Drone d; d.x = (WORLD_COLS - 4) * GRID_CELL; d.y = (rand() % WORLD_ROWS) * GRID_CELL; d.hp = 50;
            g_drones.push_back(d);
        }
    }

    if(g_resources >= 300){ g_gameOver = true; g_paused = true; }
    if(g_ship.vel.len() < 0.01 && tickCount > 60*50 && g_drones.size() > 20){ g_gameOver = true; }
}

// Simple world accessors for renderer & input
const Block* game_get_world_ptr(){ return &g_world[0][0]; }
const std::vector<Drone>& game_get_drones(){ return g_drones; }
void game_get_player_pos(double &x, double &y){ x = g_ship.pos_x; y = g_ship.pos_y; }
double game_get_player_angle(){ return g_ship.angle; }

// Input & utility hooks provided by input.cpp (declared in input.hpp)
extern Vec2 get_input_thrust();
extern bool input_is_paused();
