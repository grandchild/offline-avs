#ifndef FRAME_DUMP_H
#define FRAME_DUMP_H

#include <windows.h>

void StartFrameDump();
void FrameDump( HWND hwnd, const int width, const int height );

#endif
