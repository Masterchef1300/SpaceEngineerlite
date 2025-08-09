// src/WinMain.cpp
// Entry point for Space Engineers Lite (uses window + game API)
#include <windows.h>
#include "game.hpp"
#include "render.hpp"
#include "input.hpp"
#include <ctime>

const int WINDOW_W = 1280;
const int WINDOW_H = 720;
const int HUD_WIDTH = 260;

// Forward declaration (we keep main loop here)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// A pointer to the window so input & rendering can reference it if needed
static HWND g_hwnd = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // double buffered drawing
            HDC mem = CreateCompatibleDC(hdc);
            HBITMAP bmp = CreateCompatibleBitmap(hdc, WINDOW_W, WINDOW_H);
            HBITMAP oldbmp = (HBITMAP)SelectObject(mem, bmp);

            // draw everything to mem DC
            render_draw_world(mem);
            render_draw_hud(mem);
            render_draw_ui(mem);

            if(game_is_paused()){
                RECT rc = {0,0,WINDOW_W-HUD_WIDTH, WINDOW_H};
                SetBkMode(mem, TRANSPARENT);
                SetTextColor(mem, RGB(255,255,255));
                HFONT hf = CreateFontA(48,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Consolas");
                HFONT oldf = (HFONT)SelectObject(mem,hf);
                render_draw_text(mem, 220, 120, "PAUSED", RGB(200,200,255));
                SelectObject(mem,oldf);
                DeleteObject(hf);
            }

            if(game_is_over()){
                HFONT hf = CreateFontA(56,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Arial");
                HFONT oldf = (HFONT)SelectObject(mem,hf);
                render_draw_text(mem, 160, 120, "GAME OVER", RGB(255,80,80));
                SelectObject(mem, oldf);
                DeleteObject(hf);
            }

            // finalize: blit mem to window
            BitBlt(hdc, 0, 0, WINDOW_W, WINDOW_H, mem, 0, 0, SRCCOPY);
            // cleanup
            SelectObject(mem, oldbmp);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(hwnd, &ps);
        } break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            input_handle_window_event(hwnd, msg, wParam, lParam);
            InvalidateRect(hwnd, NULL, FALSE);
        } break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
    srand((unsigned)time(NULL));
    // register class
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "SpaceEngineersLiteClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    // create window
    HWND hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        "Space Engineers Lite (Mobile Demo)",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_W, WINDOW_H,
        NULL, NULL, hInstance, NULL
    );
    if(!hwnd) return -1;
    g_hwnd = hwnd;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // initialize subsystems
    game_init();
    render_init(hwnd);
    input_init(hwnd);

    // high-resolution loop
    LARGE_INTEGER perfFreq, lastTime, curTime;
    QueryPerformanceFrequency(&perfFreq);
    QueryPerformanceCounter(&lastTime);
    double accumulator = 0.0;
    const double DT = 1.0 / 60.0;
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while(game_is_running()){
        // process messages
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
            if(msg.message == WM_QUIT){ game_request_quit(); break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if(!game_is_running()) break;
        QueryPerformanceCounter(&curTime);
        double elapsed = (double)(curTime.QuadPart - lastTime.QuadPart) / (double)perfFreq.QuadPart;
        lastTime = curTime;
        accumulator += elapsed;

        // fixed update steps
        while(accumulator >= DT){
            game_update();
            accumulator -= DT;
        }

        // request redraw
        InvalidateRect(hwnd, NULL, FALSE);
        Sleep(4);
    }

    // shutdown
    render_shutdown();
    game_shutdown();
    input_shutdown();

    return 0;
}
