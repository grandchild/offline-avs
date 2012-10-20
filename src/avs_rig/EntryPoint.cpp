#include <math.h>
#include <stdio.h>
#include <windows.h>

#include "DummyWindow.h"
#include "FFT.h"
#include "FrameDump.h"
#include "WaveFileThingy.h"
#include "WinampShiz.h"

int avsHeight = 400;
int avsWidth = 400;

static void* s_pBuffer = NULL;
static unsigned int s_uBufferSize = 0;
static unsigned int s_uSamplesPerSec = 0;

#define DUMP_ON_C (0)

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
					GetBuffer( s_pBuffer, s_uBufferSize, s_uSamplesPerSec );
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
	float bufferPos = 0.0f;
	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );

		if( s_pBuffer )
		{
			// take 576 samples at 1/30s
			const float sampleLength = static_cast< float >( s_uBufferSize ) / static_cast< float >( s_uSamplesPerSec );
			const float baseSampleRate = sampleLength / ( 30.0f * 576.0f );

#define LERP (0)
			for( int i = 0; i < 576; ++i )
			{
				// linearly interpolate the source data
				const float pos = ( bufferPos + baseSampleRate * i ) * static_cast< float >( s_uBufferSize );
				const int idx1 = static_cast< int >( floorf( pos ) );
#if !LERP
				mod.waveformData[ 0 ][ i ] = static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ ( idx1 + i ) % s_uBufferSize ] ) / 256.0f /* 128.0f - 1.0f*/;
#else
				const float lerpAmount = pos - static_cast< float >( idx1 );
				const int idx2 = ( idx1 + 1 ) % s_uBufferSize;
				const float s1 = static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ idx1 ] );
				const float s2 = static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ idx2 ] );
				mod.waveformData[ 0 ][ i ] = static_cast< char >( ( s1 + lerpAmount * ( s2 - s1 ) ) / 256.0f /* 128.0f - 1.0f*/ );
#endif
			}

			mod.waveformNch = 1;
			
			float fftBuffer[ 576 * 4 ];
			for( int i = 0; i < 576; ++i )
			{
				fftBuffer[ 2 * i ] = static_cast< float >( mod.waveformData[ 0 ][ i ] ) / 255.f;
				fftBuffer[ 2 * i + 1 ] = 0.0f;
			}

			DanielsonLanczos< 576, float >::apply( fftBuffer );

			for( int i = 0; i < 576; ++i )
			{
				mod.spectrumData[ 0 ][ i ] = static_cast< unsigned char >( fftBuffer[ i * 2 ] * 255.f / fftBuffer[ 0 ] );
			}
			
			mod.spectrumNch = 1;

			bufferPos += ( 1.0f / 30.0f );
			while( bufferPos > sampleLength )
			{
				bufferPos -= sampleLength;
			}
		}

		// TODO: cram some audio data and forced timings in here

		mod.delayMs = 33;
		mod.latencyMs = 33;
		mod.sRate = s_uSamplesPerSec;
		mod.Render( &mod );

		FrameDump( mod.hwndParent, avsWidth, avsHeight );

		static bool oldcDown = false;
		const bool cDown = GetAsyncKeyState( 0x43 ) & 0x8000;

		if( oldcDown && !cDown )
		{
#if DUMP_ON_C
			StartFrameDump();
#endif
		}

		oldcDown = cDown;
	}

	mod.Quit( &mod );

	return 0;
}