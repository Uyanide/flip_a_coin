#include "gif_play.h"
using namespace Gdiplus;

// Timer ID
constexpr UINT IDI_TIMER = 1;

// Gdiplus initialization
static bool is_GDIplus_initialized = false;
static int GIF_instance_count = 0;
static GdiplusStartupInput gdiplus_startup_input;
static ULONG_PTR gdiplus_token;

// Subwindow class registration
static bool is_gif_wc_registered = false;
static WNDCLASS gif_wc = {
    CS_HREDRAW | CS_VREDRAW,
    gif_proc,
    0,
    0,
    GetModuleHandle(NULL),
    NULL,
    NULL,
    NULL,
    NULL,
    L"GIF_SUBWINDOW"};

/********************************************/
/*                                          */
/*       Constructor and Destructor         */
/*                                          */
/********************************************/

GIF_PLAYER::GIF_PLAYER(HWND hwnd, const wchar_t *path, COLORREF background_rbg, UINT position_x, UINT position_y, bool is_loop)
    : is_loop(is_loop)
{
    try
    {
        // Initialize GDI+, if not already initialized
        if (!is_GDIplus_initialized)
        {
            if (GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, NULL) != Ok)
                throw std::logic_error("GDI+ initialization failed!");
            is_GDIplus_initialized = true;
        }

        // Register the subwindow class, if not already registered
        if (!is_gif_wc_registered)
        {
            if (!RegisterClass(&gif_wc))
                throw std::logic_error("Subwindow Class Registration Failed!");
            is_gif_wc_registered = true;
        }

        // Load image basic information
        gif_src = new Image(path);
        if (!gif_src || gif_src->GetLastStatus() != Ok)
            throw std::logic_error("Image loading failed!");
        gif_rect = Rect(position_x, position_y, gif_src->GetWidth(), gif_src->GetHeight());

        gif_RECT.left = gif_rect.X;
        gif_RECT.top = gif_rect.Y;
        gif_RECT.right = gif_rect.X + gif_rect.Width;
        gif_RECT.bottom = gif_rect.Y + gif_rect.Height;

        get_frame_count();
        get_frame_delay();

        background_color.SetFromCOLORREF(background_rbg);

        // Create a subwindow for the GIF
        create_subwindow(hwnd);
    }
    catch (const std::logic_error &e)
    {
        MessageBoxA(NULL, e.what(), "Error!", MB_ICONEXCLAMATION | MB_OK);
        throw;
    }

    // Increase the instance count, if the constructor is successful
    GIF_instance_count++;
}

// Case load from rc
GIF_PLAYER::GIF_PLAYER(HWND hwnd, LPWSTR path, COLORREF background_rbg, UINT position_x, UINT position_y, bool is_loop)
    : is_loop(is_loop)
{
    try
    {
        // Initialize GDI+, if not already initialized
        if (!is_GDIplus_initialized)
        {
            if (GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, NULL) != Ok)
                throw std::logic_error("GDI+ initialization failed!");
            is_GDIplus_initialized = true;
        }

        // Register the subwindow class, if not already registered
        if (!is_gif_wc_registered)
        {
            if (!RegisterClass(&gif_wc))
                throw std::logic_error("Subwindow Class Registration Failed!");
            is_gif_wc_registered = true;
        }

        // Load image basic information, case load from rc
        HMODULE hModule = GetModuleHandle(NULL);
        HRSRC hResource = FindResource(hModule, path, L"GIF");
        if (hResource == NULL)
            throw std::logic_error("Resource not found!");
        HGLOBAL hMemory = LoadResource(hModule, hResource);
        if (hMemory == NULL)
            throw std::logic_error("Resource loading failed!");
        LPVOID pResource = LockResource(hMemory);
        if (pResource == NULL)
            throw std::logic_error("Resource locking failed!");
        DWORD dwSize = SizeofResource(hModule, hResource);
        if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) != S_OK)
            throw std::logic_error("Stream creation failed!");
        if (pStream->Write(pResource, dwSize, NULL) != S_OK)
            throw std::logic_error("Stream writing failed!");
        gif_src = new Image(pStream);

        gif_rect = Rect(position_x, position_y, gif_src->GetWidth(), gif_src->GetHeight());

        gif_RECT.left = gif_rect.X;
        gif_RECT.top = gif_rect.Y;
        gif_RECT.right = gif_rect.X + gif_rect.Width;
        gif_RECT.bottom = gif_rect.Y + gif_rect.Height;

        get_frame_count();
        get_frame_delay();

        background_color.SetFromCOLORREF(background_rbg);

        // Create a subwindow for the GIF
        create_subwindow(hwnd);
    }
    catch (const std::logic_error &e)
    {
        MessageBoxA(NULL, e.what(), "Error!", MB_ICONEXCLAMATION | MB_OK);
        throw;
    }

    // Increase the instance count, if the constructor is successful
    GIF_instance_count++;
}

GIF_PLAYER::~GIF_PLAYER()
{
    delete gif_src;
    // Release the stream, if it is created
    if (pStream)
        pStream->Release();
    // Decrease the instance count and shut down GDI+, if the last instance is destroyed
    if (--GIF_instance_count == 0)
    {
        GdiplusShutdown(gdiplus_token);
        is_GDIplus_initialized = false;
    }
}

/********************************************/
/*                                          */
/*      GIF information init and get        */
/*                                          */
/********************************************/

UINT GIF_PLAYER::get_frame_count()
{
    if (frame_count == -1)
        frame_count = gif_src->GetFrameCount(&FrameDimensionTime);
    return frame_count;
}

UINT GIF_PLAYER::get_frame_delay()
{
    if (frame_delay == -1)
    {
        UINT propSize = gif_src->GetPropertyItemSize(PropertyTagFrameDelay);
        PropertyItem *propItemDelay = (PropertyItem *)malloc(propSize);
        if (propItemDelay == NULL)
            throw std::logic_error("Memory allocation failed!");
        gif_src->GetPropertyItem(PropertyTagFrameDelay, propSize, propItemDelay);
        frame_delay = *(UINT *)propItemDelay->value * 10;
        free(propItemDelay);
    }
    return frame_delay;
}

Rect GIF_PLAYER::get_gif_rect() const
{
    return gif_rect;
}

void GIF_PLAYER::create_subwindow(HWND hwnd)
{
    HWND subwindow = CreateWindowEx(
        WS_EX_TRANSPARENT,
        L"GIF_SUBWINDOW",
        NULL,
        WS_CHILD | WS_VISIBLE,
        gif_rect.X, gif_rect.Y, gif_rect.Width, gif_rect.Height,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        this);

    if (subwindow == NULL)
    {
        throw std::logic_error("Subwindow Creation Failed!");
    }

    ShowWindow(subwindow, SW_SHOW);

    // Set the focus to the main window
    SetFocus(hwnd);

    gif_hwnd = subwindow;
}

/********************************************/
/*                                          */
/*           GIF playback control           */
/*                                          */
/********************************************/

// Draw the frame on the main window, without double buffering
void GIF_PLAYER::draw_frame(HDC hdc, UINT frameIndex)
{
    Graphics graphics(hdc);
    graphics.Clear(background_color);
    gif_src->SelectActiveFrame(&FrameDimensionTime, frameIndex);
    graphics.DrawImage(gif_src, gif_rect);
}

// Draw the frame on the subwindow, with double buffering
void GIF_PLAYER::draw_frame(HDC hdc, UINT frameIndex, bool)
{
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBMP = CreateCompatibleBitmap(hdc, gif_rect.Width, gif_rect.Height);
    HBITMAP oldBMP = (HBITMAP)SelectObject(memDC, memBMP);

    gif_src->SelectActiveFrame(&FrameDimensionTime, frameIndex);
    Graphics graphics(memDC);
    graphics.Clear(background_color);
    graphics.DrawImage(gif_src, gif_rect);

    BitBlt(hdc, gif_rect.X, gif_rect.Y, gif_rect.Width, gif_rect.Height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBMP);
    DeleteObject(memBMP);
    DeleteDC(memDC);
}

void GIF_PLAYER::start()
{
    if (gif_state != PLAY)
    {
        gif_state = PLAY;
        if (!SetTimer(gif_hwnd, IDI_TIMER, frame_delay, NULL))
            throw std::logic_error("Timer Creation Failed!");
    }
}

void GIF_PLAYER::cease()
{
    if (gif_state != CEASE)
    {
        current_frame = -1;
        // Only destroy the timer if it is still running
        if (gif_state == PLAY && !KillTimer(gif_hwnd, IDI_TIMER))
            throw std::logic_error("Timer Destruction Failed!");
        // Destroy the subwindow, not always the case but is acceptable in this program
        if (!DestroyWindow(gif_hwnd))
            throw std::logic_error("Subwindow Destruction Failed!");
        gif_state = CEASE;
        gif_hwnd = NULL;
    }
}

void GIF_PLAYER::pause()
{
    // Switch between PLAY and PAUSE
    if (gif_state == PLAY)
    {
        gif_state = PAUSE;
        if (!KillTimer(gif_hwnd, IDI_TIMER))
            throw std::logic_error("Timer Destruction Failed!");
    }
    else if (gif_state == PAUSE)
    {
        start();
    }
}

/********************************************/
/*                                          */
/*           Subwindow Procedure            */
/*                                          */
/********************************************/

LRESULT CALLBACK gif_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Get the GIF_PLAY instance from the subwindow
    GIF_PLAYER *gif = reinterpret_cast<GIF_PLAYER *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_CREATE:
    {
        // Save the GIF_PLAY instance in the USERDATA of the subwindow
        CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        gif->draw_frame(hdc, gif->current_frame, true);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_TIMER:
    {
        // Draw the next frame, if meets the frame count, pause the GIF
        if (++(gif->current_frame) == gif->get_frame_count())
        {
            if (gif->is_loop)
                gif->current_frame = 0;
            else
                gif->pause();
        }
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        // GIF_PLAY instance should be deleted in the main window procedure
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}