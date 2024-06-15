#pragma once

#include <windows.h>
#include <stdexcept>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#define IDI_TIMER 1

LRESULT CALLBACK gif_proc(HWND, UINT, WPARAM, LPARAM);

class GIF_PLAYER
{
public:
	GIF_PLAYER(HWND, const wchar_t*, COLORREF, UINT = 0, UINT = 0, bool = false);
	GIF_PLAYER(HWND, LPWSTR, COLORREF, UINT = 0, UINT = 0, bool = false);
	~GIF_PLAYER();

private:
	void create_subwindow(HWND);

public:
	UINT get_frame_count();
	UINT get_frame_delay();
	Gdiplus::Rect get_gif_rect() const;

	void draw_frame(HDC, UINT);
	void draw_frame(HDC, UINT, bool DOUBLE_BUFFER);
	void start();
	void cease();
	void pause();

private:
	// information about the GIF resource
	Gdiplus::Image* gif_src;
	INT frame_count = -1;            // Number of frames in the GIF
	INT frame_delay = -1;            // Frame delay in milliseconds
	Gdiplus::Rect gif_rect;          // position and size of the GIF
	RECT gif_RECT;                   // converted to RECT
	Gdiplus::Color background_color; // color similar to the main color of the GIF

	// case load from rc
	IStream* pStream = NULL;

	// subwindow for the GIF
	HWND gif_hwnd;

public:
	// GIF playback control
	enum
	{
		PLAY,
		PAUSE,
		CEASE
	} gif_state = CEASE;
	INT current_frame = -1;
	bool is_loop = false;
};