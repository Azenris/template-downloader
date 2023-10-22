
// System Includes
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

// Platform Specific Includes
#ifdef PLATFORM_WINDOWS
#include <direct.h>
#else
#include <sys/stat.h>
#endif

// Third Party Includes
#include "zlib/zlib.h"
#include "curl/curl.h"
#include "zip.h"

// Includes
#include "types.h"
#include "array.h"
#include "map.h"

// --------------------------------------------------------------------------------

enum RESULT_CODE
{
	RESULT_CODE_SUCCESS,
	RESULT_CODE_INSUFFICIENT_ARGUMENTS,
	RESULT_CODE_FAILED_TO_PARSE_ARGUMENTS,
	RESULT_CODE_UNKNOWN_ARGUMENT_COMMAND,
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
	"RESULT_CODE_INSUFFICIENT_ARGUMENTS",
	"RESULT_CODE_FAILED_TO_PARSE_ARGUMENTS",
	"RESULT_CODE_UNKNOWN_ARGUMENT_COMMAND",
	"RESULT_CODE_CURL_FAILED_INIT",
	"RESULT_CODE_FAILED_TO_OPEN_FILE",
	"RESULT_CODE_FAILED_TO_RETRIEVE_DATA",
	"RESULT_CODE_FAILED_TO_OPEN_ARCHIVE",
	"RESULT_CODE_FAILED_TO_UNZIP_ARCHIVE",
};

constexpr const i64 MAX_COMMANDS = 32;
constexpr const i64 MAX_FILEPATH = 4096;
constexpr const char *TEMP_ARCHIVE_FILE = "file.zip";

struct Options
{
	bool verbose = false;
	char destFolder[ MAX_FILEPATH ] = "\0";
	char sourceRepo[ MAX_FILEPATH ] = "\0";

} options;

static i32 usage( i32 error )
{
	printf( "---------------------------------------------------------------------------------------------------------" );
	printf( "  Error( %d ): %s", error, RESULT_CODE_NAME[ error ] );
	printf( "---------------------------------------------------------------------------------------------------------" );
	printf( "  Usage." );
	printf( "---------------------------------------------------------------------------------------------------------" );
	printf( "  template-downloader -o <dest-folder> -s <source-github>" );
	printf( "  EG." );
	printf( "  template-downloader -o . -s Azenris/game-template" );
	printf( "  template-downloader -o C:/projects/my_new_project -s Azenris/game-template" );
	printf( "---------------------------------------------------------------------------------------------------------" );
	printf( "  Options:" );
	printf( "    -o           = Output Folder" );
	printf( "    -s           = Github Source" );
	printf( "    -v           = Verbose Output" );
	printf( "---------------------------------------------------------------------------------------------------------" );

	return error;
}

static void log( const char *message, ... )
{
	if ( !options.verbose )
		return;

	va_list list;
	va_start( list, message );
	vprintf( message, list );
	va_end( list );
	printf( "\n" );
}

static void log_error( const char *message, ... )
{
	va_list list;
	va_start( list, message );
	vfprintf( stderr, message, list );
	va_end( list );
	fprintf( stderr, "\n" );
}

static u64 write_data( void *data, u64 size, u64 nmemb, void *stream )
{
	u64 written = fwrite( data, size, nmemb, (FILE *)stream );
	return written;
}

static bool extract_all_files( zip_t *zip, const char *path )
{
	struct zip_stat st;
	char filePath[ 4096 ];
	char buf[ 1024 ];

	for ( zip_int64_t i = 0, fileCount = zip_get_num_entries( zip, 0 ); i < fileCount; ++i )
	{
		i32 err = zip_stat_index( zip, i, 0, &st );
		if ( err != 0 )
		{
			log_error( "Error getting file stat." );
			return false;
		}

		snprintf( filePath, sizeof( filePath ), "%s/%s", path, st.name );

		if ( st.name[ strlen( st.name ) - 1 ] != '/' )
		{
			// File
			FILE *fp = fopen( filePath, "wb" );
			if ( !fp )
			{
				log_error( "Error opening file: %s", filePath );
				return false;
			}

			// Extract the file contents.
			zip_file_t *zf = zip_fopen_index( zip, i, 0 );
			if ( !zf )
			{
				log_error( "Error opening file in archive: %s", st.name );
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
		else
		{
			// Folder
			#ifdef PLATFORM_WINDOWS
				_mkdir( filePath );
			#else
				mkdir( filePath, 0777 );
			#endif
		}
	}

	return true;
}

// ----------------------------------------
// ENTRY
// ----------------------------------------
int main( int argc, const char *argv[] )
{
	if ( argc < 3 )
	{
		return usage( RESULT_CODE_INSUFFICIENT_ARGUMENTS );
	}

	// ----------------------------------------
	// Arguments / Options
	// ----------------------------------------
	Map<const char*, bool (*)( i32 &index, int argc, const char *argv[] ), MAX_COMMANDS> commands;

	// Display in log the received arguments: -ra
	commands.insert( "-ra", [] ( i32 &index, int argc, const char *argv[] )
	{
		printf( "Arguments received [#%d]\n", argc );
		for ( i32 i = 0; i < argc; ++i )
			printf( " [%d] = %s\n", i, argv[ i ] );
		return true;
	} );

	// Set the output folder
	commands.insert( "-o", []( i32 &index, int argc, const char *argv[] )
	{
		if ( index >= argc )
			return false;
		strcpy( options.destFolder, argv[ ++index ] );
		return true;
	} );

	// Set the source repo
	commands.insert( "-s", []( i32 &index, int argc, const char *argv[] )
	{
		if ( index >= argc )
			return false;
		strcpy( options.sourceRepo, argv[ ++index ] );
		return true;
	} );

	// Enable verbose logging
	commands.insert( "-v", []( i32 &index, int argc, const char *argv[] )
	{
		options.verbose = true;
		return true;
	} );

	// Process the option commands
	for ( i32 i = 1; i < argc; ++i )
	{
		auto f = commands.find( argv[ i ] );

		if ( f )
		{
			if ( !f->value( i, argc, &argv[ 0 ] ) )
				return usage( RESULT_CODE_FAILED_TO_PARSE_ARGUMENTS );
		}
		else
		{
			printf( "Unknown argument command: %s", argv[ i ] );
			return RESULT_CODE_UNKNOWN_ARGUMENT_COMMAND;
		}
	}

	log( "Running template-downloader" );
	log( "Destination: %s", options.destFolder );
	log( "Github Source: %s", options.sourceRepo );

	// ----------------------------------------
	// Download the achive from github
	// ----------------------------------------
	curl_global_init( CURL_GLOBAL_ALL );

	CURL *handle = curl_easy_init();
	if ( !handle )
	{
		log_error( "Failed to open the curl_easy_init." );
		return usage( RESULT_CODE_CURL_FAILED_INIT );
	}

	i32 dlAttempts = 6;
	CURLcode res;
	char curlErrorString[ CURL_ERROR_SIZE ];

	do
	{
		FILE *file = fopen( TEMP_ARCHIVE_FILE, "wb" );
		if ( !file )
		{
			log_error( "Failed to open the file: %s.", TEMP_ARCHIVE_FILE );
			return usage( RESULT_CODE_FAILED_TO_OPEN_FILE );
		}

		curl_easy_setopt( handle, CURLOPT_URL, "https://github.com/Azenris/timer/archive/master.zip" );
		curl_easy_setopt( handle, CURLOPT_VERBOSE, 0 );
		curl_easy_setopt( handle, CURLOPT_FOLLOWLOCATION, 1 );
		curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, write_data );
		curl_easy_setopt( handle, CURLOPT_WRITEDATA, file );
		curl_easy_setopt( handle, CURLOPT_ERRORBUFFER, curlErrorString );

		res = curl_easy_perform( handle );

		fclose( file );
	}
	while ( --dlAttempts >= 0 && res != CURLE_OK );

	if ( res != CURLE_OK )
	{
		log_error( "A problem has occured." );
		log_error( "Error: %s", curlErrorString );
		return usage( RESULT_CODE_FAILED_TO_RETRIEVE_DATA );
	}

	curl_easy_cleanup( handle );

	log( "Archive downloaded." );

	// ----------------------------------------
	// Unzip the archive
	// ----------------------------------------
	i32 err = 0;
	zip *z = zip_open( TEMP_ARCHIVE_FILE, ZIP_RDONLY, &err );

	if ( !z )
	{
		zip_error_t error;
		zip_error_init_with_code( &error, err );
		log_error( "Cannot open zip archive '%s': %s", TEMP_ARCHIVE_FILE, zip_error_strerror( &error ) );
		zip_error_fini( &error );
		zip_close( z );
		return usage( RESULT_CODE_FAILED_TO_OPEN_ARCHIVE );
	}

	if ( !extract_all_files( z, "." ) )
	{
		log_error( "Error unzipping archive." );
		return usage( RESULT_CODE_FAILED_TO_UNZIP_ARCHIVE );
	}

	zip_close( z );

	remove( TEMP_ARCHIVE_FILE );

	log( "Unzip Complete." );

	// ----------------------------------------
	// Setup
	// ----------------------------------------


	log( "Setup Complete." );

	// ----------------------------------------
	// Clean up
	// ----------------------------------------

	log( "Cleaning Up." );

	curl_global_cleanup();

	return RESULT_CODE_SUCCESS;
}