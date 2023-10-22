
// System Includes
#include <stdint.h>
#include <stdio.h>

// Third Party Includes
#include "zlib/zlib.h"
#include "curl/curl.h"
#include "zip.h"

// Includes
#include "types.h"

// --------------------------------------------------------------------------------

enum RESULT_CODE
{
	RESULT_CODE_SUCCESS,
	RESULT_CODE_CURL_FAILED_INIT,
	RESULT_CODE_FAILED_TO_OPEN_FILE,
	RESULT_CODE_FAILED_TO_RETRIEVE_DATA,
	RESULT_CODE_FAILED_TO_ALLOCATE_HEAP_MEMORY,
	RESULT_CODE_FAILED_TO_OPEN_ARCHIVE,
	RESULT_CODE_FAILED_TO_UNZIP_ARCHIVE,
};

static constexpr const char *RESULT_CODE_NAME[] = 
{
	"RESULT_CODE_SUCCESS",
	"RESULT_CODE_CURL_FAILED_INIT",
	"RESULT_CODE_FAILED_TO_OPEN_FILE",
	"RESULT_CODE_FAILED_TO_RETRIEVE_DATA",
	"RESULT_CODE_FAILED_TO_OPEN_ARCHIVE",
	"RESULT_CODE_FAILED_TO_UNZIP_ARCHIVE",
};

static u64 write_data( void *data, u64 size, u64 nmemb, void *stream )
{
	u64 written = fwrite( data, size, nmemb, (FILE *)stream );
	return written;
}

constexpr const char *archiveFile = "file.zip";

bool extract_all_files( zip_t *zip, const char *path )
{
	struct zip_stat st;
	char filePath[ 4096 ];
	char buf[ 1024 ];

	// Iterate over all the files in the archive.
	for ( zip_int64_t i = 0, fileCount = zip_get_num_entries( zip, 0 ); i < fileCount; ++i )
	{
		// Get the file name and size.
		int err = zip_stat_index( zip, i, 0, &st );
		if ( err != 0 )
		{
			fprintf( stderr, "Error getting file stat.\n" );
			return false;
		}

		// Create a new file on disk.
		snprintf( filePath, sizeof( filePath ), "%s/%s", path, st.name );
		FILE *fp = fopen( filePath, "wb" );
		if ( !fp )
		{
			fprintf( stderr, "Error opening file: %s\n", filePath );
			return false;
		}

		// Extract the file contents.
		zip_file_t *zf = zip_fopen_index( zip, i, 0 );
		if ( !zf )
		{
			fprintf( stderr, "Error opening file in archive: %s\n", st.name );
			return false;
		}

		u64 nread;

		while ( ( nread = zip_fread( zf, buf, sizeof( buf ) ) ) > 0 )
		{
			fwrite( buf, 1, nread, fp );
		}

		// Close the files.
		fclose( fp );
		zip_fclose( zf );
	}

	return true;
}

int main( int argc, const char *argv[] )
{
	printf( "template-downloader\n" );

	// ----------------------------------------
	// Download the achive from github
	// ----------------------------------------
	curl_global_init( CURL_GLOBAL_ALL );

	CURL *handle = curl_easy_init();
	if ( !handle )
	{
		fprintf( stderr, "Failed to open the curl_easy_init.\n" );
		return RESULT_CODE_CURL_FAILED_INIT;
	}

	int dlAttempts = 3;
	CURLcode res;

	do
	{
		FILE *file = fopen( archiveFile, "wb" );
		if ( !file )
		{
			fprintf( stderr, "Failed to open the file: %s.\n", archiveFile );
			return RESULT_CODE_FAILED_TO_OPEN_FILE;
		}

		char curlErrorString[ CURL_ERROR_SIZE ];
		curlErrorString[ 0 ] = '\0';

		curl_easy_setopt( handle, CURLOPT_URL, "https://github.com/Azenris/timer/archive/master.zip" );
		curl_easy_setopt( handle, CURLOPT_VERBOSE, 0 );
		curl_easy_setopt( handle, CURLOPT_FOLLOWLOCATION, 1 );
		curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, write_data );
		curl_easy_setopt( handle, CURLOPT_WRITEDATA, file );
		curl_easy_setopt( handle, CURLOPT_ERRORBUFFER, curlErrorString );

		res = curl_easy_perform( handle );

		fclose( file );

		if ( res != CURLE_OK )
		{
			fprintf( stderr, "\nA problem has occured.\n" );
			fprintf( stderr, "Error: %s\n", curlErrorString );
			return RESULT_CODE_FAILED_TO_RETRIEVE_DATA;
		}
	}
	while( dlAttempts > 0 && res != CURLE_OK );

	curl_easy_cleanup( handle );

	// ----------------------------------------
	// Unzip the archive
	// ----------------------------------------
	int err = 0;
	zip *z = zip_open( archiveFile, ZIP_RDONLY, &err );

	if ( !z )
	{
		zip_error_t error;
		zip_error_init_with_code( &error, err );
		fprintf( stderr, "Cannot open zip archive '%s': %s\n", archiveFile, zip_error_strerror( &error ) );
		zip_error_fini( &error );
		zip_close( z );
		return RESULT_CODE_FAILED_TO_OPEN_ARCHIVE;
	}

	if ( !extract_all_files( z, "." ) )
	{
		fprintf( stderr, "Error unzipping archive.\n" );
		return RESULT_CODE_FAILED_TO_UNZIP_ARCHIVE;
	}

	zip_close( z );

	// ----------------------------------------

	curl_global_cleanup();

	return RESULT_CODE_SUCCESS;
}