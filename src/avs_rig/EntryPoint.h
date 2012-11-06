#ifndef ENTRY_POINT_H
#define ENTRY_POINT_H

#include <windows.h>

void SendWavePacket();
void SetStereo( bool bStereo );

float HanningWindow( short in, size_t i, size_t s );

#endif
