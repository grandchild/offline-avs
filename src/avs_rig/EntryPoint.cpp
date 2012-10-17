#include <windows.h>
#include <stdio.h>

#include "DummyWindow.h"
#include "WaveFileThingy.h"
#include "WinampShiz.h"

int avsHeight = 400;
int avsWidth = 400;

static void* s_pBuffer = NULL;
static unsigned int s_uBufferSize = 0;

int main( const unsigned int count, const char* const* const pszCommandLine )
{
	const char* dllPath = pszCommandLine[ 0 ];

	// SE - 17/10/2012: note fall through is intentional
	switch( count )
	{
	case 1:
	case 0:		{
					printf( "Need at least one parameter..." );
					return -1;
				}
	default:
	case 5:		{
					LoadWaveFileThingy( pszCommandLine[ 4 ] );
					GetBuffer( s_pBuffer, s_uBufferSize );
					s_uBufferSize >>= 1;
	case 4:			avsHeight = atoi( pszCommandLine[ 3 ] );
	case 3:			avsWidth = atoi( pszCommandLine[ 2 ] );
	case 2:			dllPath = pszCommandLine[ 1 ];
					break;
				}
	}
	
	HMODULE hmod = LoadLibraryA( dllPath );

	winampVisHeader* (*pfn)() = reinterpret_cast< winampVisHeader* (*)() >( GetProcAddress( hmod, "winampVisGetHeader" ) );

	winampVisHeader& hdr = *pfn();
	winampVisModule& mod = *( hdr.getModule( 0 ) );
	mod.hwndParent = createDummyWindow();
	mod.hDllInstance = hmod;
	mod.Init(&mod);

	MSG msg;
	u_int uBufferPos = 0;
	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );

		if( s_pBuffer )
		{
			for( int i = 0; i < 576; ++i )
			{
				const int sourceIndex = ( uBufferPos + i ) % s_uBufferSize;

				mod.waveformData[ 0 ][ i ] = reinterpret_cast< unsigned short* >( s_pBuffer )[ sourceIndex ] >> 8;
			}

			mod.waveformNch = 1;

			uBufferPos += 576;
			uBufferPos %= s_uBufferSize;
		}

		// TODO: cram some audio data and forced timings in here

		mod.Render( &mod );
	}

	return 0;
}