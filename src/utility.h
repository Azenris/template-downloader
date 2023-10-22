
#pragma once

#define KB( x ) 						( (u64)1024 * x )
#define MB( x ) 						( (u64)1024 * KB( x ) )
#define GB( x ) 						( (u64)1024 * MB( x ) )
#define MAX_CONVERT_TO_STRING_DIGITS	( static_cast<u64>( 32 ) )

template <typename T>
[[nodiscard]] inline T sign( T value )
{
	return value > 0 ? 1 : ( value < 0 ? -1 : 0 );
}

template <typename T>
[[nodiscard]] inline T min( T a, T b )
{
	return a <= b ? a : b;
}

template <typename T>
[[nodiscard]] inline T max( T a, T b )
{
	return a >= b ? a : b;
}

template <typename T>
[[nodiscard]] inline T clamp( T v, T a, T b )
{
	return max( min( v, max( a, b ) ), min( a, b ) );
}

[[nodiscard]] u64 convert_to_u64( const char *input, const char **output = nullptr );

[[nodiscard]] inline u32 convert_to_u32( const char *input, const char **output = nullptr )
{
	return static_cast<u32>( convert_to_u64( input, output ) );
}

[[nodiscard]] inline u16 convert_to_u16( const char *input, const char **output = nullptr )
{
	return static_cast<u16>( convert_to_u64( input, output ) );
}

[[nodiscard]] inline u8 convert_to_u8( const char *input, const char **output = nullptr )
{
	return static_cast<u8>( convert_to_u64( input, output ) );
}

[[nodiscard]] i64 convert_to_i64( const char *input, const char **output = nullptr );

[[nodiscard]] inline i32 convert_to_i32( const char *input, const char **output = nullptr )
{
	return static_cast<i32>( clamp( convert_to_i64( input, output ), (i64)INT_MIN, (i64)INT_MAX ) );
}

[[nodiscard]] inline i16 convert_to_i16( const char *input, const char **output = nullptr )
{
	return static_cast<i16>( clamp( convert_to_i64( input, output ), (i64)INT16_MIN, (i64)INT16_MAX ) );
}

[[nodiscard]] inline i8 convert_to_i8( const char *input, const char **output = nullptr )
{
	return static_cast<i8>( clamp( convert_to_i64( input, output ), (i64)INT8_MIN, (i64)INT8_MAX ) );
}

[[nodiscard]] inline i32 convert_to_int( const char *input, const char **output = nullptr )
{
	return convert_to_i32( input, output );
}

[[nodiscard]] f32 convert_to_float( const char *input, const char **output = nullptr );
[[nodiscard]] bool convert_to_bool( const char *input, const char **output = nullptr );
[[nodiscard]] bool is_floating_point( const char *input );

u64 convert_to_string( char *dest, u64 destSize, u64 value, i32 radix = 10, i32 trailing = 0 );

inline u64 convert_to_string( char *dest, u64 destSize, u32 value, i32 radix = 10, i32 trailing = 0 )
{
	return convert_to_string( dest, destSize, static_cast<u64>( value ), radix, trailing );
}

inline u64 convert_to_string( char *dest, u64 destSize, u16 value, i32 radix = 10, i32 trailing = 0 )
{
	return convert_to_string( dest, destSize, static_cast<u64>( value ), radix, trailing );
}

inline u64 convert_to_string( char *dest, u64 destSize, u8 value, i32 radix = 10, i32 trailing = 0 )
{
	return convert_to_string( dest, destSize, static_cast<u64>( value ), radix, trailing );
}

u64 convert_to_string( char *dest, u64 destSize, i64 value, i32 radix = 10, i32 trailing = 0 );

inline u64 convert_to_string( char *dest, u64 destSize, i32 value, i32 radix = 10, i32 trailing = 0 )
{
	return convert_to_string( dest, destSize, static_cast<i64>( value ), radix, trailing );
}

inline u64 convert_to_string( char *dest, u64 destSize, i16 value, i32 radix = 10, i32 trailing = 0 )
{
	return convert_to_string( dest, destSize, static_cast<i64>( value ), radix, trailing );
}

inline u64 convert_to_string( char *dest, u64 destSize, i8 value, i32 radix = 10, i32 trailing = 0 )
{
	return convert_to_string( dest, destSize, static_cast<i64>( value ), radix, trailing );
}

u64 convert_to_string( char *dest, u64 destSize, f32 value, i32 fracDigits = 2 );

inline u64 convert_to_string( char *dest, u64 destSize, bool value );

[[nodiscard]] const char *convert_to_string( u64 value, i32 radix = 10, i32 trailing = 0 );
[[nodiscard]] const char *convert_to_string( u32 value, i32 radix = 10, i32 trailing = 0 );
[[nodiscard]] const char *convert_to_string( u16 value, i32 radix = 10, i32 trailing = 0 );
[[nodiscard]] const char *convert_to_string( u8 value, i32 radix = 10, i32 trailing = 0 );

[[nodiscard]] const char *convert_to_string( i64 value, i32 radix = 10, i32 trailing = 0 );
[[nodiscard]] const char *convert_to_string( i32 value, i32 radix = 10, i32 trailing = 0 );
[[nodiscard]] const char *convert_to_string( i16 value, i32 radix = 10, i32 trailing = 0 );
[[nodiscard]] const char *convert_to_string( i8 value, i32 radix = 10, i32 trailing = 0 );

[[nodiscard]] const char *convert_to_string( f32 value, i32 fracDigits );
[[nodiscard]] const char *convert_to_string( bool value );