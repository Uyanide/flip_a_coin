#ifndef GIF_PLAY_H
#define GIF_PLAY_H

#ifndef UNICODE
#define UNICODE
#endif // UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif // _UNICODE

#include <windows.h>
#include <stdexcept>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

LRESULT CALLBACK gif_proc(HWND, UINT, WPARAM, LPARAM);

class GIF_PLAYER
{
public:
    enum STATE
    {
        PLAY,
        PAUSE,
        CEASE
    };

    // Constructor
    GIF_PLAYER(HWND, LPWSTR, bool, COLORREF, UINT = 0, UINT = 0, bool = false);

    // Destructor
    ~GIF_PLAYER();

private:
    // Private methods
    void create_subwindow(HWND);
    Gdiplus::Status gdiplus_init();
    void load_gif_from_rc(LPWSTR);
    void get_gif_info(UINT, UINT);

public:
    // Public methods
    UINT get_frame_count() const;
    UINT get_frame_delay() const;
    Gdiplus::Rect get_gif_rect() const;
    STATE get_gif_state() const;
    INT get_current_frame() const;
    bool get_is_loop() const;

    void next_frame();
    void set_curr_frame(INT);

    void draw_curr_frame(HDC);
    void draw_curr_frame(HDC, bool DOUBLE_BUFFER);
    void start();
    void cease();
    void pause();

private:
    // Information about the GIF resource
    Gdiplus::Image *gif_src;
    INT frame_count = -1;            // Number of frames in the GIF
    INT frame_delay = -1;            // Frame delay in milliseconds
    Gdiplus::Rect gif_rect;          // Position and size of the GIF
    RECT gif_RECT;                   // Converted to RECT
    Gdiplus::Color background_color; // Color similar to the main color of the GIF

    // Case load from rc
    IStream *pStream = NULL;

    // Subwindow for the GIF
    HWND gif_hwnd;

    // GIF playback control
    STATE gif_state = CEASE;
    INT current_frame = -1;
    bool is_loop = false;
};

#endif // GIF_PLAY_H
