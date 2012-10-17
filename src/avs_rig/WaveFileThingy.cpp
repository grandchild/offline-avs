#include <stdio.h>
#include <windows.h>

typedef unsigned int u_int;

#define GLTOY_FOURCC(ch0, ch1, ch2, ch3) \
                    ((unsigned int)(unsigned char)(ch0) | ((unsigned int)(unsigned char)(ch1) << 8) | \
                    ((unsigned int)(unsigned char)(ch2) << 16) | ((unsigned int)(unsigned char)(ch3) << 24 ))


const u_int uGLTOY_4CC_DATA = GLTOY_FOURCC( 'd', 'a', 't', 'a' );
const u_int uGLTOY_4CC_FMT  = GLTOY_FOURCC( 'f', 'm', 't', ' ' );
const u_int uGLTOY_4CC_RIFF = GLTOY_FOURCC( 'R', 'I', 'F', 'F' );
const u_int uGLTOY_4CC_WAVE = GLTOY_FOURCC( 'W', 'A', 'V', 'E' );

const u_int uGLTOY_WAVE_READ_SIZE = 1024 * 16;

static WAVEFORMATEX s_xFormat;
static void* s_pBuffer = NULL;
static unsigned int s_uBufferSize;

struct Riff_Chunk
{
    u_int uChunkType;
    u_int uChunkSize;
};

const WAVEFORMATEX& GetWaveFormat()
{
	return s_xFormat;
}

void GetBuffer( void*& pBuffer, unsigned int& uBufferSize )
{
	uBufferSize = s_uBufferSize;
	pBuffer = s_pBuffer;
}

bool ParseChunk( FILE* pxFile )
{
	Riff_Chunk xRiff;

	if( fread( &xRiff, sizeof( Riff_Chunk ), 1, pxFile ) == 0 )
	{
			return false;
	}

	switch( xRiff.uChunkType )
	{
		case uGLTOY_4CC_DATA:
		{
			s_uBufferSize = xRiff.uChunkSize;
			s_pBuffer = new char[ s_uBufferSize ];

			unsigned int uToRead = s_uBufferSize;

			while( uToRead > 0 )
			{
				u_int uReadSize = uGLTOY_WAVE_READ_SIZE;
				if( uToRead < uGLTOY_WAVE_READ_SIZE )
				{
						uReadSize = uToRead;
				}

				if( fread( ( reinterpret_cast<char*>(s_pBuffer) + ( s_uBufferSize - uToRead ) ), uReadSize, 1, pxFile ) == 0 )
				{
						return false;
				}

				uToRead -= uReadSize;
			}

			return true;
		}
		break;

		case uGLTOY_4CC_FMT:
		{
			char acBuffer[512];

			if( fread( &acBuffer, xRiff.uChunkSize, 1, pxFile ) == 0 )
			{
				return false;
			}

			memcpy( &s_xFormat, &acBuffer[0], sizeof( WAVEFORMATEX ) );
	        
			// Must be 16 bit PCM
			if( ( s_xFormat.wFormatTag != WAVE_FORMAT_PCM ) || ( s_xFormat.wBitsPerSample != 16 ) )
			{
				return false;
			}

			return true;
		}
		break;

		default:
		{
			// Unknown chunk, skip it
			fseek( pxFile, xRiff.uChunkSize, SEEK_CUR );
			return true;
		}
	}

	// shouldnt get here
	return false;
}

void LoadWaveFileThingy( const char* const szFilename )
{
	FILE* pxFile = fopen( szFilename, "rb" );

	if( !pxFile )
	{
		return;
	}

	unsigned int uFileSize = 0;

	// Read RIFF header
	{
		Riff_Chunk xRiff;

		if( fread( &xRiff, sizeof( Riff_Chunk ), 1, pxFile ) == 0 )
		{
			fclose( pxFile );
			return;
		}

		if( xRiff.uChunkType != uGLTOY_4CC_RIFF ) 
		{
			fclose( pxFile );
			return;
		}

		// File size is RIFF chunk size + the above chunk header
		uFileSize = xRiff.uChunkSize + 8;
	}

	// RIFF chunk contents are the subtype FOURCC then subtype-dependent data

	unsigned int uSubType;
	if( fread( &uSubType, sizeof( unsigned int ), 1, pxFile ) == 0 )
	{
		fclose( pxFile );
		return;
	}

	if( uSubType != uGLTOY_4CC_WAVE ) 
	{
		fclose( pxFile );
		return;
	}

	while( ftell( pxFile ) < static_cast<int>( uFileSize ) )
	{
		if( !ParseChunk( pxFile ) )
		{
			fclose( pxFile );
			return;
		}
	}

	fclose( pxFile );
}
