#include "DummyWindow.h"
#include "WinampShiz.h"

static HWND s_hwnd = NULL;
static HWND s_visWindow = NULL;

HWND e( embedWindowState* state )
{
	return s_hwnd;
}

LRESULT WINAPI WndProc( HWND h, UINT msg, WPARAM w, LPARAM l );


HWND createDummyWindow()
{
	HWND       hWnd;
	WNDCLASSEXA WndClsEx;
	memset( &WndClsEx, 0, sizeof( WNDCLASSEXA ) );
	// Create the application window
	WndClsEx.cbSize        = sizeof( WNDCLASSEX );
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = WndProc;
	WndClsEx.lpszClassName = "Dummy";

	// Register the application
	RegisterClassExA(&WndClsEx);

	// Create the window object
	hWnd = CreateWindowA("Dummy",
			  "Dummy",
			  WS_OVERLAPPEDWINDOW,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  NULL,
			  NULL,
			  NULL,
			  NULL);
	
	// Find out if the window was created
	if( !hWnd ) // If the window was not created,
		return 0; // stop the application

	// Display the window to the user
	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	return s_hwnd = hWnd;
}

LRESULT WINAPI WndProc( HWND h, UINT msg, WPARAM w, LPARAM l )
{
	// winamp version check
	if( ( msg == WM_USER )
		&& ( w == 0 )
		&& ( l == 0 ) )
	{
		return 0x2900;
	}

	if( ( msg == WM_USER )
		&& ( l == 611 ) )
	{
		//return 0;
		//return (LRESULT)s_hwnd;
	}

	// get ini file
	if( ( msg == WM_USER )
		&& ( l == 334 ) )
	{
		return (LRESULT)"fuck_you.ini";
	}

	if( ( msg == WM_USER )
		&& ( l == IPC_GET_EMBEDIF ) )
	{
		return (LRESULT)e;
	}

	if( ( msg == WM_USER )
		&& ( l == IPC_SETVISWND ) )
	{
		s_visWindow = (HWND)w;

		SetWindowPos( s_visWindow, 0, 0, 0, avsWidth, avsHeight, 0 );
	}

	return DefWindowProcA( h, msg, w, l );
}