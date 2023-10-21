
// System Includes
#include <stdint.h>
#include <stdio.h>

// Third Party Includes
#include "zlib/zlib.h"
#include "curl/curl.h"

// Includes
#include "types.h"

// --------------------------------------------------------------------------------

enum RESULT_CODE
{
	RESULT_CODE_SUCCESS,
	RESULT_CODE_CURL_FAILED_INIT,
	RESULT_CODE_FAILED_TO_OPEN_FILE,
	RESULT_CODE_FAILED_TO_RETRIEVE_DATA,
};

static constexpr const char *RESULT_CODE_NAME[] = 
{
	"RESULT_CODE_SUCCESS",
	"RESULT_CODE_CURL_FAILED_INIT",
	"RESULT_CODE_FAILED_TO_OPEN_FILE",
	"RESULT_CODE_FAILED_TO_RETRIEVE_DATA",
};

static u64 write_data( void *data, u64 size, u64 nmemb, void *stream )
{
	u64 written = fwrite( data, size, nmemb, (FILE *)stream );
	return written;
}

int main( int argc, const char *argv[] )
{
	printf( "template-downloader\n" );

	curl_global_init( CURL_GLOBAL_ALL );

	CURL *handle = curl_easy_init();
	if ( !handle )
	{
		return RESULT_CODE_CURL_FAILED_INIT;
	}

	FILE *file = fopen( "file.zip", "wb" );
	if ( !file )
	{
		return RESULT_CODE_FAILED_TO_OPEN_FILE;
	}

	char error[ CURL_ERROR_SIZE ];
	error[ 0 ] = '\0';

	curl_easy_setopt( handle, CURLOPT_URL, "https://github.com/Azenris/timer/archive/master.zip" );
	curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
	curl_easy_setopt( handle, CURLOPT_FOLLOWLOCATION, 1 );
	curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, write_data );
	curl_easy_setopt( handle, CURLOPT_WRITEDATA, file );
	curl_easy_setopt( handle, CURLOPT_ERRORBUFFER, error );

	CURLcode res = curl_easy_perform( handle );

	if ( res != CURLE_OK )
	{
		printf( "A problem has occured.\n" );
		printf( "Error: %s\n", error );
		return RESULT_CODE_FAILED_TO_RETRIEVE_DATA;
	}

	curl_easy_cleanup( handle );

	fclose( file );

	curl_global_cleanup();

	return RESULT_CODE_SUCCESS;
}