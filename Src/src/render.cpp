// src/render.cpp
#include "render.hpp"
#include "game.hpp"
#include <string>
#include <sstream>
#include <cmath>

static double camX = 0, camY = 0;
static const int GRID_CELL = 24;
static const int HUD_WIDTH = 260;

void render_init(HWND hwnd){ /* nothing yet */ }
void render_shutdown(){ /* nothing yet */ }

void render_draw_text(HDC hdc, int x, int y, const char* s, COLORREF color){ SetTextColor(hdc,color); SetBkMode(hdc,TRANSPARENT); TextOutA(hdc,x,y,s,(int)strlen(s)); }
void render_draw_text(HDC hdc, int x, int y, const std::string &s, COLORREF color){ render_draw_text(hdc,x,y,s.c_str(),color); }

void render_draw_world(HDC hdc){
    // camera follow player
    double pwx, pwy; game_get_player_pos(pwx, pwy);
    int window_w = game_get_window_width();
    int window_h = game_get_window_height();

    double targetCamX = pwx - (window_w - HUD_WIDTH)/2.0;
    double targetCamY = pwy - window_h/2.0;
    camX += (targetCamX - camX) * 0.12;
    camY += (targetCamY - camY) * 0.12;

    RECT rc; rc.left=0; rc.top=0; rc.right=window_w-HUD_WIDTH; rc.bottom=window_h;
    HBRUSH bg = CreateSolidBrush(RGB(10,10,28));
    FillRect(hdc, &rc, bg); DeleteObject(bg);

    const int WORLD_COLS = game_world_cols();
    const int WORLD_ROWS = game_world_rows();

    const Block* worldPtr = game_get_world_ptr();
    for(int r=0;r<WORLD_ROWS;r++){
        for(int c=0;c<WORLD_COLS;c++){
            int sx = (int)(c*GRID_CELL - camX);
            int sy = (int)(r*GRID_CELL - camY);
            RECT cellR = {sx, sy, sx+GRID_CELL, sy+GRID_CELL};
            HBRUSH tileBrush = CreateSolidBrush(RGB(14,14,24));
            FillRect(hdc, &cellR, tileBrush);
            DeleteObject(tileBrush);
            const Block &b = worldPtr[r*WORLD_COLS + c];
            if(b.type != BLOCK_EMPTY){
                HBRUSH br;
                switch(b.type){
                    case BLOCK_ARMOR: br = CreateSolidBrush(RGB(120,110,80)); break;
                    case BLOCK_THRUSTER: br = CreateSolidBrush(RGB(180,60,40)); break;
                    case BLOCK_MINER: br = CreateSolidBrush(RGB(100,180,200)); break;
                    default: br = CreateSolidBrush(RGB(180,180,180)); break;
                }
                FrameRect(hdc, &cellR, br);
                RECT inner = {sx+3, sy+3, sx+GRID_CELL-3, sy+GRID_CELL-3};
                FillRect(hdc, &inner, br);
                DeleteObject(br);
            }
        }
    }

    // draw ship blocks (simple)
    double sxp, syp; game_get_player_pos(sxp,syp);
    double angled = game_get_player_angle();
    // we don't have the ship block offsets here (they are internal), so render player as small ship icon
    int sx = (int)(sxp - camX);
    int sy = (int)(syp - camY);
    POINT pts[3];
    pts[0].x = sx; pts[0].y = sy - 12;
    pts[1].x = sx - 10; pts[1].y = sy + 12;
    pts[2].x = sx + 10; pts[2].y = sy + 12;
    HBRUSH sb = CreateSolidBrush(RGB(220,200,60));
    HBRUSH oldb = (HBRUSH)SelectObject(hdc, sb);
    Polygon(hdc, pts, 3);
    SelectObject(hdc, oldb);
    DeleteObject(sb);

    // draw drones
    const std::vector<Drone> &drones = game_get_drones();
    for(const auto &d: drones){
        int dx = (int)(d.x - camX);
        int dy = (int)(d.y - camY);
        HBRUSH db = CreateSolidBrush(RGB(180,80,90));
        RECT rr = {dx-8, dy-8, dx+8, dy+8};
        FillRect(hdc, &rr, db);
        DeleteObject(db);
    }
}

void render_draw_hud(HDC hdc){
    int left = game_get_window_width() - HUD_WIDTH;
    RECT r = {left,0,left+HUD_WIDTH, game_get_window_height()};
    HBRUSH bg = CreateSolidBrush(RGB(25,25,40));
    FillRect(hdc, &r, bg); DeleteObject(bg);

    std::ostringstream ss;
    ss << "SPACE ENGINEERS LITE (Mobile Demo)";
    render_draw_text(hdc,left+12,12, ss.str());
    ss.str(""); ss.clear();
    ss << "Resources: " << game_get_resources();
    render_draw_text(hdc,left+12,44, ss.str());
    ss.str(""); ss.clear();
    ss << "Score: " << game_get_score();
    render_draw_text(hdc,left+12,68, ss.str());

    render_draw_text(hdc,left+12,110, "Build Palette:");
    // draw boxes as placeholders
    RECT r1 = {left+12, 140, left+12+24, 140+24};
    HBRUSH b1 = CreateSolidBrush(RGB(120,110,80));
    FillRect(hdc, &r1, b1); DeleteObject(b1);
    render_draw_text(hdc,left+12+24+8,150, "1 - Armor (8 res)");
    RECT r2 = {left+12, 180, left+12+24, 180+24};
    HBRUSH b2 = CreateSolidBrush(RGB(200,80,40)); FillRect(hdc,&r2,b2); DeleteObject(b2);
    render_draw_text(hdc,left+12+24+8,190, "2 - Thruster (12 res)");
    RECT r3 = {left+12, 220, left+12+24, 220+24};
    HBRUSH b3 = CreateSolidBrush(RGB(80,160,200)); FillRect(hdc,&r3,b3); DeleteObject(b3);
    render_draw_text(hdc,left+12+24+8,230, "3 - Miner (10 res)");
}

static POINT joystickCenter = {120, 720 - 120};
static double joystickDirX = 0, joystickDirY = 0;
void render_set_joystick(double x, double y){ joystickDirX = x; joystickDirY = y; }

void render_draw_ui(HDC hdc){
    int cx = joystickCenter.x, cy = joystickCenter.y;
    HBRUSH jb = CreateSolidBrush(RGB(40,40,60));
    Ellipse(hdc, cx - 60, cy - 60, cx + 60, cy + 60);
    DeleteObject(jb);
    int kx = cx + (int)(joystickDirX * 60);
    int ky = cy + (int)(joystickDirY * 60);
    HBRUSH knob = CreateSolidBrush(RGB(120,180,200));
    Ellipse(hdc, kx-18, ky-18, kx+18, ky+18);
    DeleteObject(knob);
}
