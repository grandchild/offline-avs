#include "FrameDump.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb/stb_image_write.h"

#include <windows.h>

static bool hacketyHack = true;

void StartFrameDump()
{
	hacketyHack = false;
}

void FrameDump( HWND hwnd, const int width, const int height )
{
	if( hacketyHack )
	{
		return;
	}

	HDC hDC = GetDC( hwnd );
	HDC memDC = CreateCompatibleDC( hDC );


    HBITMAP hBitmap = CreateCompatibleBitmap( hDC, width, height );
    SelectObject( memDC, hBitmap );
    BitBlt( memDC, 0, 0, width, height, hDC, 0, 0, SRCCOPY );

	BITMAPINFO info;
	memset( &info.bmiHeader, 0, sizeof( BITMAPINFOHEADER ) );
    info.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
	
	const int pixelCount = width * height;
	unsigned int* const puData = new unsigned int [ width * height ];
    GetDIBits( memDC , hBitmap , 0 , height , puData , &info , DIB_RGB_COLORS );
	// fill in alpha properly...
	for( int i = 0; i < pixelCount; ++i )
	{
		puData[ i ] |= 0xff000000;
	}
	static int counter = 0;
	static char outPath[ 256 ];
	sprintf( outPath, "image%05d.png", counter );

	stbi_write_png( outPath, width, height, 4, puData, 4 * width );

	++counter;

	delete[] puData;
	ReleaseDC( hwnd, memDC );
}