#include <math.h>
#include <stdio.h>
#include <windows.h>
#include <iostream>

#include "EntryPoint.h"
#include "DummyWindow.h"
#include "FFT.h"
#include "FrameDump.h"
#include "WaveFileThingy.h"
#include "WinampShiz.h"

int avsHeight = 400;
int avsWidth = 400;

using namespace std;

static void* s_pBuffer = NULL;
static unsigned int s_uBufferSize = 0;
static unsigned int s_uSamplesPerSec = 0;

float fTimeNowMS = 0.0f;
winampVisModule* mod;
bool bStereoAudio = false;

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
	mod = hdr.getModule( 0 );
	mod->hwndParent = createDummyWindow();
	mod->hDllInstance = hmod;
	mod->Init(mod);

	MSG msg;
	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	//mod.Quit( &mod );

	return 0;
}

void SendWavePacket()
{
	const float sampleLength = static_cast< float >( s_uBufferSize ) / static_cast< float >( s_uSamplesPerSec );

	if( s_pBuffer )
	{
		const float fSampleNow = ( fTimeNowMS / 1000.0f ) * static_cast< float >( s_uSamplesPerSec );
		const float pos = fSampleNow;
		const int idx1 = static_cast< int >( floorf( pos ) );

		if ( bStereoAudio )
		{
			for( int i = 0; i < 576; ++i )
			{
				mod->waveformData[ 0 ][ i ] = static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ ( idx1 + ( i * 2 ) ) ] ) / 256.0f;
				mod->waveformData[ 1 ][ i ] = static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ ( idx1 + ( i * 2 + 1 ) ) ] ) / 256.0f;

				//fftBuffer[ 2 * i ] = static_cast< float >( mod->waveformData[ 0 ][ i ] ) / 255.f;
				//fftBuffer[ 2 * i + 1 ] = 0.0f;
			}
			mod->waveformNch = 2;
		}
		else
		{
			for( int i = 0; i < 576; ++i )
			{
				mod->waveformData[ 0 ][ i ] = static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ ( idx1 + ( i * 2 ) ) ] ) / 256.0f;
			}

			mod->waveformNch = 1;
		}

		// Spectrum Data
		/*
		if ( bStereoAudio )
		{
			for( int i = 0; i < 576; ++i )
			{
				fftBuffer[ 2 * i ] = static_cast< float >( mod->waveformData[ 0 ][ i ] ) / 255.f;
				fftBuffer[ 2 * i + 1 ] = 0.0f;
			}
		}

		float fftBuffer[ 576 * 4 ];
	

		DanielsonLanczos< 576, float >::apply( fftBuffer );

		for( int i = 0; i < 576; ++i )
		{
			mod->spectrumData[ 0 ][ i ] = static_cast< unsigned char >( fftBuffer[ i * 2 ] * 255.f / fftBuffer[ 0 ] );
		}
		
		mod->spectrumNch = 1;
		*/
	}

	// TODO: cram some audio data and forced timings in here

	cout << "PASSING NEW WAVE DATA: " << bStereoAudio + 1 << "ch. @ " << fTimeNowMS << " ms\n";

	mod->delayMs = 33;
	mod->latencyMs = 33;
	mod->sRate = s_uSamplesPerSec;
	mod->Render( mod );

	fTimeNowMS += ( 1000.0f / 30.0f ) * ( bStereoAudio ? 2.0f : 1.0f );
}

void SetStereo( bool bStereo )
{
	bStereoAudio = bStereo;
}