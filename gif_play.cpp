#include "gif_play.h"

// Timer ID
constexpr UINT IDI_TIMER = 1;

// Gdiplus initialization
static bool is_GDIplus_initialized = false;
static int GIF_instance_count = 0;
static Gdiplus::GdiplusStartupInput gdiplus_startup_input;
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

// Initialize GDI+, if not already initialized
Gdiplus::Status GIF_PLAYER::gdiplus_init()
{
    if (!is_GDIplus_initialized)
    {
        if (GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, NULL) != Gdiplus::Ok)
            return Gdiplus::GenericError;
        is_GDIplus_initialized = true;
    }
    return Gdiplus::Ok;
}

// Load GIF from resource
void GIF_PLAYER::load_gif_from_rc(LPWSTR path)
{
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
    gif_src = new Gdiplus::Image(pStream);
}

// Get GIF information, including the number of frames, frame delay, and the position and size of the GIF
void GIF_PLAYER::get_gif_info(UINT position_x, UINT position_y)
{
    frame_count = gif_src->GetFrameCount(&Gdiplus::FrameDimensionTime);

    UINT propSize = gif_src->GetPropertyItemSize(PropertyTagFrameDelay);
    Gdiplus::PropertyItem *propItemDelay = (Gdiplus::PropertyItem *)malloc(propSize);
    if (propItemDelay == NULL)
        throw std::logic_error("Memory allocation failed!");
    gif_src->GetPropertyItem(PropertyTagFrameDelay, propSize, propItemDelay);
    frame_delay = *(UINT *)propItemDelay->value * 10;
    free(propItemDelay);

    gif_rect = Gdiplus::Rect(position_x, position_y, gif_src->GetWidth(), gif_src->GetHeight());
    gif_RECT.left = gif_rect.X;
    gif_RECT.top = gif_rect.Y;
    gif_RECT.right = gif_rect.X + gif_rect.Width;
    gif_RECT.bottom = gif_rect.Y + gif_rect.Height;
}

GIF_PLAYER::GIF_PLAYER(HWND hwnd, LPWSTR path, bool is_rc, COLORREF background_rbg, UINT position_x, UINT position_y, bool is_loop)
    : is_loop(is_loop)
{
    try
    {
        // Initialize GDI+, if not already initialized
        if (gdiplus_init() != Gdiplus::Ok)
            throw std::logic_error("GDI+ Initialization Failed!");

        // Register the subwindow class, if not already registered
        if (!is_gif_wc_registered)
        {
            if (!RegisterClass(&gif_wc))
                throw std::logic_error("Subwindow Class Registration Failed!");
            is_gif_wc_registered = true;
        }

        // Load image basic information
        if (!is_rc)
        {
            gif_src = new Gdiplus::Image(path);
        }
        else
        {
            load_gif_from_rc(path);
        }
        if (!gif_src || gif_src->GetLastStatus() != Gdiplus::Ok)
            throw std::logic_error("Image loading failed!");

        get_gif_info(position_x, position_y);

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
    if (gif_hwnd)
    {
        if (gif_state == PLAY)
            KillTimer(gif_hwnd, IDI_TIMER);
        DestroyWindow(gif_hwnd);
        gif_hwnd = NULL;
    }
    delete gif_src;
    // Release the stream, if it is created
    if (pStream)
        pStream->Release();
    // Decrease the instance count and shut down GDI+, if the last instance is destroyed
    if (--GIF_instance_count == 0)
    {
        Gdiplus::GdiplusShutdown(gdiplus_token);
        is_GDIplus_initialized = false;
    }
}

/********************************************/
/*                                          */
/*           Getters and Setters            */
/*                                          */
/********************************************/

UINT GIF_PLAYER::get_frame_count() const
{
    return frame_count;
}

UINT GIF_PLAYER::get_frame_delay() const
{
    return frame_delay;
}

Gdiplus::Rect GIF_PLAYER::get_gif_rect() const
{
    return gif_rect;
}

GIF_PLAYER::STATE GIF_PLAYER::get_gif_state() const
{
    return gif_state;
}

INT GIF_PLAYER::get_current_frame() const
{
    return current_frame;
}

bool GIF_PLAYER::get_is_loop() const
{
    return is_loop;
}

void GIF_PLAYER::next_frame()
{
    if (gif_state == CEASE || gif_state == PAUSE)
        return;
    if (++current_frame == frame_count)
    {
        if (is_loop)
            current_frame = 0;
        else
            pause();
    }
    InvalidateRect(gif_hwnd, NULL, FALSE);
}

void GIF_PLAYER::set_curr_frame(INT frame)
{
    if (frame < 0 || frame >= frame_count)
        throw std::logic_error("Frame out of range!");
    current_frame = frame;
    InvalidateRect(gif_hwnd, NULL, FALSE);
}

HWND GIF_PLAYER::get_gif_hwnd() const
{
    return gif_hwnd;
}

void GIF_PLAYER::clear_gif_hwnd()
{
    gif_hwnd = NULL;
}

/********************************************/
/*                                          */
/*           GIF playback control           */
/*                                          */
/********************************************/

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

// Draw the frame on the main window, without double buffering
void GIF_PLAYER::draw_curr_frame(HDC hdc)
{
    Gdiplus::Graphics graphics(hdc);
    graphics.Clear(background_color);
    gif_src->SelectActiveFrame(&Gdiplus::FrameDimensionTime, current_frame);
    graphics.DrawImage(gif_src, gif_rect);
}

// Draw the frame on the subwindow, with double buffering
void GIF_PLAYER::draw_curr_frame(HDC hdc, bool)
{
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBMP = CreateCompatibleBitmap(hdc, gif_rect.Width, gif_rect.Height);
    HBITMAP oldBMP = (HBITMAP)SelectObject(memDC, memBMP);

    gif_src->SelectActiveFrame(&Gdiplus::FrameDimensionTime, current_frame);
    Gdiplus::Graphics graphics(memDC);
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
        gif_state = CEASE;
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
        gif->draw_curr_frame(hdc, true);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_TIMER:
    {
        gif->next_frame();
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        gif->clear_gif_hwnd();
        break;
    case WM_DESTROY:
        // GIF_PLAY instance should be deleted in the main window procedure
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}