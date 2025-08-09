// include/input.hpp
#pragma once
#include <windows.h>
#include "game.hpp"

void input_init(HWND hwnd);
void input_shutdown();
void input_handle_window_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// functions to query input state for game module
Vec2 get_input_thrust();
bool input_is_paused();
