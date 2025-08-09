// include/render.hpp
#pragma once
#include <windows.h>
#include <vector>
#include "game.hpp"

void render_init(HWND hwnd);
void render_shutdown();

void render_draw_world(HDC hdc);
void render_draw_hud(HDC hdc);
void render_draw_ui(HDC hdc);
void render_draw_text(HDC hdc, int x, int y, const char* s, COLORREF color = RGB(255,255,255));
void render_draw_text(HDC hdc, int x, int y, const std::string &s, COLORREF color = RGB(255,255,255));
