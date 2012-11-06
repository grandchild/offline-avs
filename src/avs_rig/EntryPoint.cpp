#include <math.h>
#include <stdio.h>
#include <windows.h>
#include <limits.h>
#include <iostream>

#include "EntryPoint.h"
#include "DummyWindow.h"
#include "kiss_fft.h"
#include "kiss_fftr.h"
#include "FrameDump.h"
#include "WaveFileThingy.h"
#include "WinampShiz.h"

int avsHeight = 400;
int avsWidth = 400;

using namespace std;

static void* s_pBuffer = NULL;
static unsigned int s_uBufferSize = 0;
static unsigned int s_uSamplesPerSec = 0;
static signed short s_sFftBuffer[ 2048 ];

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

	// Message loop
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
	const int fftBufferSize = 576 * 2;

	const float sampleLength = static_cast< float >( s_uBufferSize ) / static_cast< float >( s_uSamplesPerSec );
	
	if( s_pBuffer )
	{
		const float fSampleNow = ( fTimeNowMS / 1000.0f ) * static_cast< float >( s_uSamplesPerSec );
		const float pos = fSampleNow;
		const int idx1 = static_cast< int >( floorf( pos ) );
		
		if ( bStereoAudio )
		{	
			// STEREO

			kiss_fftr_cfg cfgL = kiss_fftr_alloc( fftBufferSize, 0, 0, 0 );
			kiss_fftr_cfg cfgR = kiss_fftr_alloc( fftBufferSize, 0, 0, 0 );
 
			kiss_fft_scalar fftInputL[ fftBufferSize ];
			kiss_fft_scalar fftInputR[ fftBufferSize ];
			kiss_fft_cpx fftOutputL[ fftBufferSize ];
			kiss_fft_cpx fftOutputR[ fftBufferSize ];

			for ( int i = 0; i < fftBufferSize; i++ )
			{
				fftInputL[ i ] = HanningWindow( static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ idx1 + ( i * 2 ) ] ), i, 1024 );
				fftInputR[ i ] = HanningWindow( static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ idx1 + ( i * 2 ) + 1 ] ), i, 1024 );
			}

			kiss_fftr( cfgL, fftInputL, fftOutputL );
			kiss_fftr( cfgR, fftInputR, fftOutputR );

			for( int i = 0; i < 576; ++i )
			{
				mod->waveformData[ 0 ][ i ] = static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ idx1 + ( i * 2 ) ] ) / 256.0f;
				mod->waveformData[ 1 ][ i ] = static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ idx1 + ( i * 2 ) + 1 ] ) / 256.0f;

				kiss_fft_cpx cL = fftOutputL[ i ];
				kiss_fft_cpx cR = fftOutputR[ i ];
				float ampL = sqrt( cL.r * cL.r + cL.i * cL.i ) / ( double )SHRT_MAX;
				float ampR = sqrt( cR.r * cR.r + cR.i * cR.i ) / ( double )SHRT_MAX;

				mod->spectrumData[ 0 ][ i ] = ampL;
				mod->spectrumData[ 1 ][ i ] = ampR;
			}

			kiss_fft_cleanup();
			free( cfgL );
			free( cfgR );
		}
		else
		{
			// MONO

			kiss_fftr_cfg cfg = kiss_fftr_alloc( fftBufferSize, 0, 0, 0 );

			kiss_fft_scalar fftInput[ fftBufferSize ];
			kiss_fft_cpx fftOutput[ fftBufferSize ];

			for ( int i = 0; i < fftBufferSize; i++ )
			{
				fftInput[ i ] = HanningWindow( static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ idx1 + ( i * 2 ) ] ), i, 1024 );
			}

			kiss_fftr( cfg, fftInput, fftOutput );

			for( int i = 0; i < 576; ++i )
			{
				mod->waveformData[ 0 ][ i ] = static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ ( idx1 + ( i * 2 ) ) ] ) / 256.0f;
				mod->waveformData[ 1 ][ i ] = static_cast< float >( reinterpret_cast< unsigned short* >( s_pBuffer )[ ( idx1 + ( i * 2 ) ) ] ) / 256.0f;
				
				// Normalize fft samples
				kiss_fft_cpx c = fftOutput[ i ];
				float amp = sqrt( c.r * c.r + c.i * c.i ) / ( double )SHRT_MAX;

				mod->spectrumData[ 0 ][ i ] = amp;
				mod->spectrumData[ 1 ][ i ] = amp;
			}

			kiss_fft_cleanup();
			free( cfg );
		}
	}

	cout << "PASSING NEW WAVE DATA: " << bStereoAudio + 1 << "ch. @ " << fTimeNowMS << " ms\n";

	mod->delayMs = 33;
	mod->latencyMs = 33;
	mod->sRate = s_uSamplesPerSec;
	mod->Render( mod );

	fTimeNowMS += 33.333f * ( bStereoAudio ? 2.0f : 1.0f );
}

void SetStereo( bool bStereo )
{
	bStereoAudio = bStereo;
}

float HanningWindow( short in, size_t i, size_t s )
{
	return in * 0.5f * ( 1.0f - cos( 2.0f * 3.14159f * ( float )( i ) / ( float )( s - 1.0f ) ) );
}