// src/input.cpp
#include "input.hpp"
#include <cstring>

static bool keysDown[256] = {0};
static POINT mousePos = {0,0};
static bool mouseLeftDown = false;
static bool mouseRightDown = false;
static POINT joystickCenter = {120, 720 - 120};
static bool usingJoystick = false;
static double joystickX = 0, joystickY = 0;
static bool g_initialized = false;

void input_init(HWND hwnd){ g_initialized = true; }
void input_shutdown(){ g_initialized = false; }

Vec2 get_input_thrust(){
    Vec2 dir(0,0);
    if(keysDown['W'] || keysDown[VK_UP]) dir.y -= 1;
    if(keysDown['S'] || keysDown[VK_DOWN]) dir.y += 1;
    if(keysDown['A'] || keysDown[VK_LEFT]) dir.x -= 1;
    if(keysDown['D'] || keysDown[VK_RIGHT]) dir.x += 1;
    if(usingJoystick){ dir.x = joystickX; dir.y = joystickY; }
    return dir;
}
bool input_is_paused(){ return keysDown['P'] || keysDown['p']; }

void input_handle_window_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_LBUTTONDOWN:
            mouseLeftDown = true;
            mousePos.x = LOWORD(lParam); mousePos.y = HIWORD(lParam);
            {
                int dx = mousePos.x - joystickCenter.x;
                int dy = mousePos.y - joystickCenter.y;
                if(dx*dx + dy*dy <= 60*60){
                    usingJoystick = true;
                    joystickX = (double)dx / 60.0;
                    joystickY = (double)dy / 60.0;
                } else {
                    // click world / hud handled by game directly via public accessors if needed
                }
            }
            break;
        case WM_LBUTTONUP:
            mouseLeftDown = false; usingJoystick = false; joystickX=joystickY=0;
            break;
        case WM_RBUTTONDOWN:
            mouseRightDown = true; SetCapture(hwnd);
            break;
        case WM_RBUTTONUP:
            mouseRightDown = false; ReleaseCapture();
            break;
        case WM_MOUSEMOVE:
            mousePos.x = LOWORD(lParam); mousePos.y = HIWORD(lParam);
            if(usingJoystick && mouseLeftDown){
                int dx = mousePos.x - joystickCenter.x;
                int dy = mousePos.y - joystickCenter.y;
                joystickX = (double)dx / 60.0;
                joystickY = (double)dy / 60.0;
                double len = sqrt(joystickX*joystickX + joystickY*joystickY);
                if(len > 1.0){ joystickX/=len; joystickY/=len; }
            }
            break;
        case WM_KEYDOWN:
            keysDown[wParam & 0xFF] = true;
            if(wParam == VK_ESCAPE) PostQuitMessage(0);
            break;
        case WM_KEYUP:
            keysDown[wParam & 0xFF] = false;
            break;
        default: break;
    }
}
