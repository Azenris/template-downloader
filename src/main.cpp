
// System Includes
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <dirent.h>

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

// Remove Thirdparty/Platform Libs making min.max macros...
#undef min
#undef max

// Includes
#include "types.h"
#include "memory_arena.h"
#include "array.h"
#include "strings.h"
#include "map.h"
#include "utility.h"

// --------------------------------------------------------------------------------

enum RESULT_CODE
{
	RESULT_CODE_SUCCESS,
	RESULT_CODE_FAILED_TO_INITIALISE_MEMORY_ARENA,
	RESULT_CODE_INSUFFICIENT_ARGUMENTS,
	RESULT_CODE_FAILED_TO_PARSE_ARGUMENTS,
	RESULT_CODE_UNKNOWN_ARGUMENT_COMMAND,
	RESULT_CODE_MISSING_PROJECT_NAME,
	RESULT_CODE_MISSING_SOURCE_REPO,
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
	"RESULT_CODE_FAILED_TO_INITIALISE_MEMORY_ARENA",
	"RESULT_CODE_INSUFFICIENT_ARGUMENTS",
	"RESULT_CODE_FAILED_TO_PARSE_ARGUMENTS",
	"RESULT_CODE_UNKNOWN_ARGUMENT_COMMAND",
	"RESULT_CODE_MISSING_PROJECT_NAME",
	"RESULT_CODE_MISSING_SOURCE_REPO",
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
	char destFolder[ MAX_FILEPATH ] = ".";
	char projectName[ MAX_FILEPATH ] = "";
	char sourceRepo[ MAX_FILEPATH ] = "";
	char rootFolder[ MAX_FILEPATH ] = "";
	char finalProjectFolder[ MAX_FILEPATH ] = "";
	i32 attempts = 6;
	u64 permanentSize = 0;
	u64 transientSize = MB( 4 );
	u64 fastBumpSize = 0;

} options;

struct App
{
	MemoryArena memoryArena;

} app;

static i32 usage( i32 error )
{
	printf( "---------------------------------------------------------------------------------------------------------" );
	printf( "  Error( %d ): %s", error, RESULT_CODE_NAME[ error ] );
	printf( "---------------------------------------------------------------------------------------------------------" );
	printf( "  Usage." );
	printf( "---------------------------------------------------------------------------------------------------------" );
	printf( "  template-downloader -p <name> -o <dest-folder> -s <source-github>" );
	printf( "  EG." );
	printf( "  template-downloader -p ld99 -s Azenris/game-template" );
	printf( "  template-downloader -p ld99 -o C:/projects/my_new_project -s Azenris/game-template" );
	printf( "---------------------------------------------------------------------------------------------------------" );
	printf( "  Options:" );
	printf( "    -p                = Project Name (required)" );
	printf( "    -o                = Destination Folder (default .)" );
	printf( "    -s                = Github Source (required)" );
	printf( "    -v                = Verbose Output" );
	printf( "    -attempts <num>   = Number of attempts to download archive. (default 6)" );
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

static bool make_directory( const char *directory )
{
	char pathMem[ MAX_FILEPATH ];
	if ( string_utf8_copy( pathMem, directory ) == 0 )
		return false;

	char *path = pathMem;
	const char *token;
	const char *delimiters = "./\\";
	char delim;
	char dir[ MAX_FILEPATH ] = "\0";

	if ( *path == '.' )
	{
		path += 1;

		if ( *path == '/' || *path == '\\' )
		{
			// ./ (current directory)
			string_utf8_append( dir, "./" );
		}
		else if ( *path == '.' )
		{
			path += 1;

			// ../ (moving up from current directory)
			if ( *path == '/' || *path == '\\' )
			{
				// ./ (current directory)
				string_utf8_append( dir, "../" );
			}
		}
	}

	path = string_utf8_tokenise( path, delimiters, &token, &delim );

	while ( token )
	{
		if ( delim == '.' )
		{
			// file ext
			return true;
		}

		string_utf8_append( dir, token );
		string_utf8_append( dir, "/" );

		#ifdef PLATFORM_WINDOWS
			_mkdir( dir );
		#else
			mkdir( dir, 0777 );
		#endif

		path = string_utf8_tokenise( path, delimiters, &token, &delim );
	}

	return true;
}

static bool delete_directory( const char *directory )
{
	DIR *dir = opendir( directory );
	if ( !dir )
		return false;

	struct dirent *entry;

	while ( ( entry = readdir( dir ) ) != NULL)
	{
		if ( string_utf8_compare( entry->d_name, "." ) || string_utf8_compare( entry->d_name, ".." ) )
			continue;

		if ( entry->d_type == DT_DIR )
		{
			char subdirectoryPath[ MAX_FILEPATH ];
			string_utf8_copy( subdirectoryPath, directory );
			string_utf8_append( subdirectoryPath, "/" );
			string_utf8_append( subdirectoryPath, entry->d_name );

			delete_directory( subdirectoryPath );
		}
	}

	closedir( dir );

	dir = opendir( directory );
	if ( !dir )
		return false;

	while ( ( entry = readdir( dir ) ) != NULL )
	{
		if ( string_utf8_compare( entry->d_name, "." ) || string_utf8_compare( entry->d_name, ".." ) )
			continue;

		char filePath[ MAX_FILEPATH ];
		string_utf8_copy( filePath, directory );
		string_utf8_append( filePath, "/" );
		string_utf8_append( filePath, entry->d_name );

		#ifdef PLATFORM_WINDOWS
			_unlink( filePath );
		#else
			unlink( filePath );
		#endif
	}

	closedir( dir );

	i32 result = 0;

	#ifdef PLATFORM_WINDOWS
		result = _rmdir( directory );
	#else
		result = rmdir( directory );
	#endif

	return result == 0;
}

static bool extract_all_files( zip_t *zip, const char *path, char *rootFolder, u64 maxRootFolder )
{
	if ( !make_directory( path ) )
		return false;

	struct zip_stat st;
	char prePath[ MAX_FILEPATH ];
	char filePath[ MAX_FILEPATH ];
	char buf[ 1024 ];

	i32 err = zip_stat_index( zip, 0, 0, &st );
	if ( err != 0 )
	{
		log_error( "Error getting file stat." );
		return false;
	}

	if ( st.name[ strlen( st.name ) - 1 ] == '/' )
	{
		string_utf8_copy( rootFolder, maxRootFolder, path );
		string_utf8_append( rootFolder, maxRootFolder, st.name );
	}

	string_utf8_copy( prePath, path );

	for ( zip_int64_t i = 0, fileCount = zip_get_num_entries( zip, 0 ); i < fileCount; ++i )
	{
		err = zip_stat_index( zip, i, 0, &st );
		if ( err != 0 )
		{
			log_error( "Error getting file stat." );
			return false;
		}

		string_utf8_copy( filePath, prePath );
		string_utf8_append( filePath, st.name );

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
			if ( !make_directory( filePath ) )
				return false;
		}
	}

	return true;
}

static void replace_entry_in_file( const char *file, const char *findString, const char *newString )
{
	char filePath[ MAX_FILEPATH ];

	string_utf8_copy( filePath, options.finalProjectFolder );
	string_utf8_append( filePath, file );

	// Read the file into memory
	FILE *fp = fopen( filePath, "rb" );
	if ( !fp )
	{
		log_error( "Failed to open file for reading: %s", filePath );
		return;
	}

	fseek( fp, 0, SEEK_END );
	u64 fileSize = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	u64 bufferDestSize = fileSize * 2;
	char *bufferSrc = app.memoryArena.transient.allocate<char>( fileSize );
	char *bufferDest = app.memoryArena.transient.allocate<char>( bufferDestSize );
	char *bufferSrcStart = bufferSrc;
	char *bufferDestStart = bufferDest;

	fread( bufferSrc, fileSize, 1, fp );

	fclose( fp );

	u64 findStringSize = string_utf8_bytes( findString ) - 1;
	u64 newStringSize = string_utf8_bytes( newString ) - 1;
	u64 destIdx = 0;

	for ( u64 i = 0; i < fileSize; ++i )
	{
		// Most won't match, check and move on
		if ( bufferSrc[ i ] != findString[ 0 ] )
		{
			bufferDest[ destIdx++ ] = bufferSrc[ i ];
			continue;
		}

		u64 matchStart = i;
		u64 look = i;
		bool matched = true;

		for ( u64 k = 1; k < findStringSize; ++k )
		{
			if ( bufferSrc[ ++look ] != findString[ k ] )
			{
				matched = false;
				break;
			}
		}

		if ( matched )
		{
			i += newStringSize;

			if ( i > bufferDestSize )
			{
				bufferDestSize *= 2;
				bufferDestStart = app.memoryArena.transient.reallocate<char>( bufferDestStart, bufferDestSize );
				if ( !bufferDestStart )
				{
					app.memoryArena.transient.free( bufferDestStart );
					app.memoryArena.transient.free( bufferSrcStart );
					log_error( "Buffer not big enough to handle insertion (failed to allocate more)." );
					return;
				}
			}

			for ( u64 newIdx = 0; newIdx < newStringSize; ++newIdx )
			{
				bufferDest[ destIdx++ ] = newString[ newIdx ];
			}
		}
		else
		{
			for ( u64 size = i + findStringSize; i < size; ++i )
			{
				bufferDest[ destIdx++ ] = bufferSrc[ i ];
			}
		}
	}

	// Writing the updated data back to file
	fp = fopen( file, "wb" );
	if ( !fp )
	{
		log_error( "Failed to open file for writing: %s", file );
		return;
	}

	fwrite( bufferDest, destIdx, 1, fp );

	fclose( fp );

	app.memoryArena.transient.free( bufferDestStart );
	app.memoryArena.transient.free( bufferSrcStart );
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

	// Memory
	app.memoryArena =
	{
		.flags = 0,
		.memory = nullptr,
		.permanent =
		{
			.capacity = 0,
			.available = 0,
			.memory = nullptr,
			.lastAlloc = nullptr,
			.allocate_func = memory_bump_allocate,
			.reallocate_func = memory_bump_reallocate,
			.shrink_func = memory_bump_shrink,
			.free_func = memory_bump_free,
			.attach_func = memory_bump_attach,
		},
		.transient =
		{
			.capacity = 0,
			.available = 0,
			.memory = nullptr,
			.lastAlloc = nullptr,
			.allocate_func = memory_bump_allocate,
			.reallocate_func = memory_bump_reallocate,
			.shrink_func = memory_bump_shrink,
			.free_func = memory_bump_free,
			.attach_func = memory_bump_attach,
		},
		.fastBump =
		{
			.capacity = 0,
			.available = 0,
			.memory = nullptr,
			.lastAlloc = nullptr,
			.allocate_func = memory_fast_bump_allocate,
			.attach_func = nullptr,
		},
	};

	if ( !app.memoryArena.init( options.permanentSize, options.transientSize, options.fastBumpSize, true ) )
	{
		return usage( RESULT_CODE_FAILED_TO_INITIALISE_MEMORY_ARENA );
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

	// Set the project name
	commands.insert( "-p", []( i32 &index, int argc, const char *argv[] )
	{
		if ( index >= argc )
			return false;
		string_utf8_copy( options.projectName, argv[ ++index ] );
		return true;
	} );

	// Set the output folder
	commands.insert( "-o", []( i32 &index, int argc, const char *argv[] )
	{
		if ( index >= argc )
			return false;
		string_utf8_copy( options.destFolder, argv[ ++index ] );
		char last = options.destFolder[ string_utf8_bytes( options.destFolder ) - 1 ];
		if ( last != '\\' && last != '/' )
		{
			string_utf8_append( options.destFolder, "/" );
		}
		return true;
	} );

	// Set the source repo
	commands.insert( "-s", []( i32 &index, int argc, const char *argv[] )
	{
		if ( index >= argc )
			return false;
		string_utf8_copy( options.sourceRepo, "https://github.com/" );
		string_utf8_append( options.sourceRepo, argv[ ++index ] );
		string_utf8_append( options.sourceRepo, "/archive/master.zip" );
		return true;
	} );

	// Enable verbose logging
	commands.insert( "-v", []( i32 &index, int argc, const char *argv[] )
	{
		options.verbose = true;
		return true;
	} );

	// Set the number of attempts to download the archive
	commands.insert( "-attempts", []( i32 &index, int argc, const char *argv[] )
	{
		options.attempts = convert_to_i32( argv[ ++index ] );
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
			return usage( RESULT_CODE_UNKNOWN_ARGUMENT_COMMAND );
		}
	}

	if ( options.projectName[ 0 ] == '\0' )
	{
		return usage( RESULT_CODE_MISSING_PROJECT_NAME );
	}

	if ( options.sourceRepo[ 0 ] == '\0' )
	{
		return usage( RESULT_CODE_MISSING_SOURCE_REPO );
	}

	string_utf8_copy( options.finalProjectFolder, options.destFolder );
	string_utf8_append( options.finalProjectFolder, options.projectName );

	char last = options.finalProjectFolder[ string_utf8_bytes( options.finalProjectFolder ) - 1 ];
	if ( last != '\\' && last != '/' )
	{
		string_utf8_append( options.finalProjectFolder, "/" );
	}

	log( "Running template-downloader" );
	log( "Project: %s", options.projectName );
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

	i32 dlAttempts = options.attempts;
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

		curl_easy_setopt( handle, CURLOPT_URL, options.sourceRepo );
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
	zip *z = zip_open( TEMP_ARCHIVE_FILE, 0, &err );

	if ( !z )
	{
		zip_error_t error;
		zip_error_init_with_code( &error, err );
		log_error( "Cannot open zip archive '%s': %s", TEMP_ARCHIVE_FILE, zip_error_strerror( &error ) );
		zip_error_fini( &error );
		zip_close( z );
		return usage( RESULT_CODE_FAILED_TO_OPEN_ARCHIVE );
	}

	if ( !extract_all_files( z, options.destFolder, options.rootFolder, sizeof( options.rootFolder ) ) )
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

	log( "Renaming %s to %s", options.rootFolder, options.finalProjectFolder );

	delete_directory( options.finalProjectFolder );
	rename( options.rootFolder, options.finalProjectFolder );

	replace_entry_in_file( "run.bat", "__GAME_TEMPLATE_NAME__", options.projectName );
	replace_entry_in_file( "build.bat", "__GAME_TEMPLATE_NAME__", options.projectName );

	log( "Setup Complete." );

	// ----------------------------------------
	// Clean up
	// ----------------------------------------

	log( "Cleaning Up." );

	curl_global_cleanup();

	return RESULT_CODE_SUCCESS;
}

// --------------------------------------------------------------------------------
// Unity build
#include "utility.cpp"