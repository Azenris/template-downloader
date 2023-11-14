
cmake_minimum_required( VERSION 3.26.3 )

project(
	td
	VERSION 1.0
	LANGUAGES CXX
)

add_executable( td src/main.cpp )

set_target_properties(
	td
	PROPERTIES
	OUTPUT_NAME                      "template-downloader"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/builds/debug"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/builds/release"
)

target_include_directories( td PRIVATE "third_party/" )

target_compile_definitions( td PRIVATE C_PLUS_PLUS )
target_compile_definitions( td PRIVATE "$<$<CONFIG:Debug>:DEBUG>" )
target_compile_definitions( td PRIVATE "$<$<CONFIG:Release>:NDEBUG>" )

target_compile_features( td PRIVATE cxx_std_20 )

option( ZLIB_USE_STATIC_LIBS "" ON )
find_package( ZLIB REQUIRED )
target_link_libraries( td PRIVATE ZLIB::ZLIB )

find_package( LIBZIP REQUIRED )
target_link_libraries( td PRIVATE libzip::zip )

find_package( CURL REQUIRED )
target_link_libraries( td PRIVATE CURL::libcurl )

if ( MSVC )
	target_compile_definitions( td PRIVATE _CRT_SECURE_NO_WARNINGS )
	target_compile_options( td PRIVATE -WX -W4 -wd4100 -wd4201 -wd4706 -Zc:preprocessor -Zc:strictStrings -GR- )
	target_compile_options( td PRIVATE $<$<CONFIG:Debug>:-Z7 -FC -MDd> )
	target_compile_options( td PRIVATE $<$<CONFIG:Release>:-MD -O2 -Ot -GF> )
endif()

if ( CMAKE_COMPILER_IS_GNUCC )
	target_compile_options( td PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-uninitialized -Wno-non-virtual-dtor -fno-rtti )
	target_compile_options( td PRIVATE $<$<CONFIG:Debug>:-OO -g> )
	target_compile_options( td PRIVATE $<$<CONFIG:Release>:-O2> )
endif()

if ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
	target_compile_definitions( td PRIVATE "PLATFORM_WINDOWS" )
endif()
if ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
	target_compile_definitions( td PRIVATE "PLATFORM_LINUX" )
endif()
if ( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
	target_compile_definitions( td PRIVATE "PLATFORM_MAC" )
endif()