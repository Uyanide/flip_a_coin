#ifndef UNICODE
#define UNICODE
#endif // UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif // _UNICODE

/*****************************************************/
/*                                                   */
/*                  Configuration                    */
/*                                                   */
/*****************************************************/

// Select a color similar to the main color of the GIF,
// but cannot be the same color of any pixel in the GIF.
// Used as the transparent color of the layered window.
#define BACKGROUND_COLOR RGB(66, 137, 255)
#define gif_count 2

#include <windows.h>
#include <stdexcept>
#include <random>
#include "resource.h"
#include "gif_play.h"

// const wchar_t* gif_path[] = { L"rsc\\c1.gif", L"rsc\\c2.gif" };
LPWSTR gif_path[] = {MAKEINTRESOURCE(IDB_GIF1), MAKEINTRESOURCE(IDB_GIF2)};

/*****************************************************/
/*                                                   */
/*               Function Prototypes                 */
/*                                                   */
/*****************************************************/

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND create_window_borderless(HINSTANCE hInstance, const wchar_t *class_name, const wchar_t *window_name, int width, int height);

/*****************************************************/
/*                                                   */
/*                  Random Generator                 */
/*                                                   */
/*****************************************************/

class RandomLH // Random Number Generator: Lower to Higher
{
private:
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<int> dis;

public:
    RandomLH(int min, int max) : gen(rd()), dis(min, max) {}
    int operator()() { return dis(gen); }
};

/*****************************************************/
/*                                                   */
/*                  Main Function                    */
/*                                                   */
/*****************************************************/

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    try
    {
        HWND hwnd = create_window_borderless(hInstance, L"coin", L"coin", 480, 480);

        // Set the transparency color, also works with the animation subwindow later
        SetLayeredWindowAttributes(hwnd, BACKGROUND_COLOR, 0, LWA_COLORKEY);

        ShowWindow(hwnd, nShowCmd);
        UpdateWindow(hwnd);

        MSG msg = {};

        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    catch (const std::logic_error &e)
    {
        MessageBoxA(nullptr, e.what(), "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    return 0;
}

/*****************************************************/
/*                                                   */
/*               Function Definitions                */
/*                                                   */
/*****************************************************/

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Get the instance of the GIF_PLAYER class from USERDATA
    auto *gif = reinterpret_cast<GIF_PLAYER *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (msg)
    {
    case WM_CREATE:
    {
        // Create a new instance of the GIF_PLAYER and play the GIF immediately
        gif = new GIF_PLAYER(hwnd, gif_path[RandomLH(0, gif_count - 1)()], true, BACKGROUND_COLOR);
        gif->start();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(gif));
    }
    break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
    {
        if (gif != nullptr)
        {
            gif->cease();
            delete gif;
            gif = nullptr;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(gif));
        }
        PostQuitMessage(0);
    }
    break;
    case WM_KEYDOWN:
        switch (wParam)
        {
            // Press ESC to close the window
        case VK_ESCAPE:
            if (gif != nullptr)
            {
                gif->cease();
                delete gif;
                gif = nullptr;
                SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(gif));
            }
            DestroyWindow(hwnd);
            break;
            // Press SPACE to pause the GIF
        case VK_SPACE:
            if (gif != nullptr)
            {
                gif->pause();
            }
            break;
            // Press R to restart the GIF, the GIF will be randomly selected
        case 'R':
        case 'r':
            if (gif != nullptr)
            {
                gif->cease();
                delete gif;
            }
            gif = new GIF_PLAYER(hwnd, gif_path[RandomLH(0, gif_count - 1)()], true, BACKGROUND_COLOR);
            gif->start();
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(gif));
            break;
            // Press Q to go to the last frame of the GIF
        case 'Q':
        case 'q':
            if (gif != nullptr)
            {
                if (gif->get_gif_state() != GIF_PLAYER::CEASE)
                {
                    if (gif->get_gif_state() == GIF_PLAYER::PLAY)
                        gif->pause(); // PLAY -> PAUSE
                    gif->set_curr_frame((INT)gif->get_frame_count() - 2);
                    gif->pause(); // PAUSE -> PLAY
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

HWND create_window_borderless(HINSTANCE hInstance, const wchar_t *class_name, const wchar_t *window_name, int width, int height)
{
    WNDCLASS wc = {
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0,
        0,
        hInstance,
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)), // Load icon from rc file
        LoadCursor(nullptr, IDC_ARROW),
        CreateSolidBrush(BACKGROUND_COLOR),
        nullptr,
        class_name};

    if (!RegisterClass(&wc))
        throw std::logic_error("Window Registration Failed!");

    // Calculate the position of the window, center of the screen
    int posX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    int posY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    if (posX < 0 || posY < 0)
        throw std::logic_error("Window Position Calculation Failed!");

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED,
        class_name,
        window_name,
        WS_POPUP,
        posX, posY, width, height,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (hwnd == nullptr)
        throw std::logic_error("Window Creation Failed!");

    return hwnd;
}