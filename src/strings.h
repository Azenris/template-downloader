
#pragma once

[[nodiscard]] inline char ascii_char_upper( char c )
{
	return c >= 'a' && c <= 'z' ? c - 32 : c;
}

[[nodiscard]] inline char ascii_char_lower( char c )
{
	return c >= 'A' && c <= 'Z' ? c + 32 : c;
}

// UTF-8 ////////////////////////////////////////////////////////////////////////////////
struct utf8Character
{
	char data[ 8 ];
};

[[nodiscard]] inline u32 string_utf8_lower_codepoint( u32 codepoint )
{
	return codepoint >= 'A' && codepoint <= 'Z' ? codepoint + 32 : codepoint;
}

i32 string_utf8_format_args( char *destination, u64 destSize, const char *format, va_list args );

template <u64 destSize>
inline i32 string_utf8_format_args( char( &destination )[ destSize ], const char *format, va_list args )
{
	return string_utf8_format_args( destination, destSize, format, args );
}

template <u64 destSize>
i32 string_utf8_format( char( &destination )[ destSize ], const char *format, ... )
{
	va_list args;
	va_start( args, format );
	i32 result = string_utf8_format_args( destination, destSize, format, args );
	va_end( args );
	return result;
}

inline i32 string_utf8_format( char *destination, u64 destSize, const char *format, ... )
{
	va_list args;
	va_start( args, format );
	i32 result = string_utf8_format_args( destination, destSize, format, args );
	va_end( args );
	return result;
}

/// @desc Return the bytes of the string (INCLUDING the null terminator)
/// @return Bytes
[[nodiscard]] constexpr inline u64 string_utf8_bytes( const char *str )
{
	assert( str );

	u64 bytes = 0;

	while ( *str++ != '\0' )
		bytes += 1;

	return bytes + 1;
}

/// @desc Return the length of the string (NOT including the NULL terminator) (Note length != bytes)
/// @return Length
[[nodiscard]] u64 string_utf8_length( const char *str );

/// @desc Get the bytes and length of a utf8 string (NOT incluuding NULL terminator for length) (Including the NULL terminator for bytes)
void string_utf8_length_and_bytes( const char *str, u64 *length, u64 *bytes );

/// @return bytes written (NOT including the NULL terminator)
u64 string_utf8_copy( char *destination, u64 destSize, const char *source );

/// @return bytes written (NOT including the null terminator)
template <u64 destSize>
inline u64 string_utf8_copy( char( &destination )[ destSize ], const char *source )
{
	static_assert( destSize >= 1 );
	return string_utf8_copy( destination, destSize, source );
}

/// @desc Copies a number of bytes from source to destination and adds an ending null terminator. destSize should be at minimum bytes + 1
/// @return bytes written (NOT including the null terminator)
inline u64 string_utf8_copy( char *destination, u64 destSize, const char *source, u64 bytes )
{
	assert( destination );
	assert( destSize >= 1 );
	assert( source );
	assert( destSize >= bytes + 1 );

	memcpy( destination, source, bytes );
	destination[ bytes ] = '\0';

	return bytes;
}

/// @desc Copies a number of bytes from source to destination and adds an ending null terminator. destSize should be at minimum bytes + 1
/// @return bytes written (NOT including the null terminator)
template <u64 destSize>
inline u64 string_utf8_copy( char( &destination )[ destSize ], const char *source, u64 bytes )
{
	static_assert( destSize >= 1 );
	return string_utf8_copy( destination, destSize, source, bytes );
}

inline char *string_utf8_clone( const char *source, Allocator *allocator )
{
	u64 bytes = string_utf8_bytes( source );
	char *clone = allocator->allocate<char>( bytes );
	string_utf8_copy( clone, bytes, source, bytes - 1 );
	return clone;
}

template <u64 destSize>
char *string_utf8_copy_base_filename( char( &dst )[ destSize ], const char *str )
{
	assert( dst && str );

	const char *p = str++;
	char c = *p;

	while ( c != '\0' )
	{
		if ( c == '/' || c == '\\' )
			p = str;

		c = *str++;
	}

	u64 size = string_utf8_bytes( p );
	u64 len = size;
	const char *txt = p + len - 1;

	while ( len > 0 )
	{
		if ( *txt == '.' )
		{
			string_utf8_copy( dst, destSize, p, len - 1 );
			return dst;
		}

		len -= 1;
		txt -= 1;
	}

	string_utf8_copy( dst, destSize, p, size - 1 );

	return dst;
}

template <u64 destSize>
char *string_utf8_copy_without_ext( char( &dst )[ destSize ], const char *str )
{
	assert( dst && str );

	u64 size = string_utf8_bytes( str );
	u64 len = size;
	const char *txt = str + len - 1;

	while ( len > 0 )
	{
		if ( *txt == '.' )
		{
			string_utf8_copy( dst, destSize, str, len - 1 );
			return dst;
		}

		len -= 1;
		txt -= 1;
	}

	string_utf8_copy( dst, destSize, str, size - 1 );

	return dst;
}

[[nodiscard]] inline bool string_utf8_compare( const char *lhs, const char *rhs )
{
	assert( lhs );
	assert( rhs );

	while ( *lhs != '\0' )
		if ( *lhs++ != *rhs++ )
			return false;

	return *lhs == *rhs;
}

template <u64 lhsSize, u64 rhsSize>
[[nodiscard]] constexpr inline bool string_utf8_compare( const char( &lhs )[ lhsSize ], const char( &rhs )[ rhsSize ] )
{
	return string_utf8_compare( &lhs[ 0 ], &rhs[ 0 ] );
}

[[nodiscard]] u32 string_utf8_codepoint( const char *str, u32 *pSize );

i32 string_utf8_similarity( const char *lhs, const char *rhs );

[[nodiscard]] utf8Character string_utf8_encode( u32 codepoint );

[[nodiscard]] inline bool string_utf8_is_ascii( const char *str )
{
	return ( *str & 0b10000000 ) == 0;
}

[[nodiscard]] bool string_utf8_is_number( const char *str, bool *integer );

// utf8 first byte, if MSB is 0, its ASCII
// if the MSB is 1, then, code the 1's to determine the byte size
// eg. 110_ ____ , 10__ ____ the first byte shows there is 2 bytes ( 2 1's )
// the first byte 5 bits are used for the codepoint
// the seconds byte starts with a 10, with the remaining 6 bits for the codepoint
// 4 byte example : 1111 0___ , 10__ ____ , 10__ ____ , 10__ ____

[[nodiscard]] inline bool string_utf8_is_leading_byte( char c )
{
	bool isASCII = ( c & 0b10000000 ) == 0;				// if first bit is not set, then its an ASCII character ( always leading )
	bool isLeadingMultibyte = ( c & 0b01000000 ) != 0;	// non-leading multi-byte characters start with 10__ ____ ( so if 1 is set, it can't be leading )
	return isASCII || isLeadingMultibyte;
}

char *string_utf8_skip_codepoint( char *str, u32 *pSize, i32 num );

[[nodiscard]] bool string_utf8_compare_value( const char *lhs, const char *rhs );

void string_utf8_delete( char *str, i32 position );

void string_utf8_pop( char *str );

void string_utf8_pop( char *str, i32 num );

[[nodiscard]] const char *string_utf8_get_ext( const char *str );

void string_utf8_trim_ext( char *str );

[[nodiscard]] inline bool string_utf8_has_ext( const char *str );
[[nodiscard]] bool string_utf8_has_ext( const char *str, const char *ext );

[[nodiscard]] const char *string_utf8_get_filename( const char *str );

[[nodiscard]] char *string_utf8_filename( char *str );

const char *string_utf8_copy_path( char *dest, u64 destSize, const char *str );

[[nodiscard]] inline const char *string_utf8_get_path( const char *str, Allocator *allocator )
{
	u64 bytes = string_utf8_bytes( str );
	char *dest = allocator->allocate<char>( bytes );
	return string_utf8_copy_path( dest, bytes, str );
}

[[nodiscard]] inline char *string_utf8_base_filename( char *str )
{
	str = string_utf8_filename( str );
	string_utf8_trim_ext( str );
	return str;
}

inline void string_utf8_trim_path( char *str )
{
	assert( str );
	if ( str[ 0 ] != '\0' )
		string_utf8_copy( str, string_utf8_bytes( str ) - 1, string_utf8_get_filename( str ) );
}

[[nodiscard]] const char *string_utf8_past_start( const char *str, const char *start );

[[nodiscard]] char *string_utf8_past_start_case_insensitive( char *str, char *start );

[[nodiscard]] const char *string_utf8_past_start_case_insensitive( const char *str, const char *start );

[[nodiscard]] bool string_utf8_has_character( const char *str, const char *character );

template <u64 destSize>
inline u64 string_utf8_append( char( &destination )[ destSize ], const char *append )
{
	static_assert( destSize >= 1 );

	u64 p = string_utf8_bytes( destination ) - 1; // -1 is OK because only 1 requires a null terminator to count
	u64 appendBytes = string_utf8_bytes( append );

	// Check there is enough room to append
	if ( ( destSize - p ) < appendBytes )
		return 0;

	strcpy( destination + p, append );

	// Doesn't include null terminator
	return appendBytes - 1;
}

inline u64 string_utf8_append( char *destination, u64 destSize, const char *append )
{
	u64 p = string_utf8_bytes( destination ) - 1; // -1 is OK because only 1 requires a null terminator to count
	u64 appendBytes = string_utf8_bytes( append );

	// Check there is enough room to append
	if ( ( destSize - p ) < appendBytes )
		return 0;

	strcpy( destination + p, append );

	// Doesn't include null terminator
	return appendBytes - 1;
}

u64 string_utf8_insert( char *destination, u64 destSize, const char *insert, i32 index );

/// @desc Returns a count of characters until a delimiter is found
///       While marked as utf8 the delimiter should be ASCII only
[[nodiscard]] u64 string_utf8_string_span( const char *tok, const char *delim );

/// @desc Returns a count of characters until a non delimiter is found
///       While marked as utf8 the delimiter should be ASCII only
[[nodiscard]] u64 string_utf8_string_nspan( const char *tok, const char *delim );

/// @desc Output a string into token, a string split by the delimiters
///       While marked as utf8 the delimiter should be ASCII only
///       Also note it will cannibalise the input string ( do not use on string literals )
[[nodiscard]] char *string_utf8_tokenise( char *str, const char *delim, const char **token, char *found = nullptr );

/// @desc While marked as utf8 the delimiter should be ASCII only
///       The delimiter should include a newline to work as expected
template <u64 tokenLength, u64 maxTokens>
[[nodiscard]] char *string_utf8_tokenise_line( char *str, const char *delim, Array<Array<char, tokenLength>, maxTokens> &tokens )
{
	assert( string_utf8_has_character( delim, "\n" ) );

	tokens.clear();

	char delimFound;
	const char *token;

	str = string_utf8_tokenise( str, delim, &token, &delimFound );

	while ( token )
	{
		Array<char, tokenLength> *entry = &tokens.push();

		entry->resize( string_utf8_length( token ) + 1 );

		string_utf8_copy( entry->data, token );

		// if the newline was hit, process no more tokens
		if ( delimFound == '\n' )
			return str;

		str = string_utf8_tokenise( str, delim, &token, &delimFound );
	}

	return str;
}

char *string_utf8_replace_ascii_char( char *str, char find, char replace );

// UTF-16 ///////////////////////////////////////////////////////////////////////////////
#define SURROGATE_CODEPOINT_HIGH_START 		( 0xD800 )
#define SURROGATE_CODEPOINT_LOW_START 		( 0xDC00 )
#define SURROGATE_CODEPOINT_OFFSET 			( 0x010000 )
#define SURROGATE_CODEPOINT_MASK 			( 0x03FF )
#define SURROGATE_CODEPOINT_BITS 			( 10 )

struct utf16Character
{
	// high, low
	u16 data[ 3 ];
	u16 reserved;
};

[[nodiscard]] inline bool string_utf16_surrogate_pair_high( u16 c )
{
	return c >= SURROGATE_CODEPOINT_HIGH_START && c <= 0xDBFF;
}

[[nodiscard]] inline bool string_utf16_surrogate_pair_low( u16 c )
{
	return c >= SURROGATE_CODEPOINT_LOW_START && c <= 0xDFFF;
}

[[nodiscard]] u32 string_utf16_codepoint( const u16 *str, u32 *pSize );

[[nodiscard]] utf16Character string_utf16_encode( u32 codepoint );

// UTF-8 ////////////////////////////////////////////////////////////////////////////////
i32 string_utf8_format_args( char *destination, u64 destSize, const char *format, va_list args )
{
	assert( destination && format );

	i32 result = vsnprintf( destination, destSize, format, args );

	if ( result >= destSize )
	{
		destination[ 0 ] = '\0';
		return -1;
	}

	return result;
}

/// @desc Return the length of the string (NOT including the NULL terminator) (Note length != bytes)
/// @return Length
[[nodiscard]] u64 string_utf8_length( const char *str )
{
	u64 length = 0;
	i32 i = 0;
	char c = str[ i++ ];

	while ( c )
	{
		if ( c >= 0 && c < 127 )			// 1-byte : 0___ ____
		{
		}
		else if ( ( c & 0xE0 ) == 0xC0 )	// 2-byte : 11__ ____
		{
			i += 1;
		}
		else if ( ( c & 0xF0 ) == 0xE0 )	// 3-byte : 111_ ____
		{
			i += 2;
		}
		else if ( ( c & 0xF8 ) == 0xF0 )	// 4-byte : 1111 ____
		{
			i += 3;
		}
		else if ( ( c & 0xFC ) == 0xF8 )	// 5-byte : 1111 1___
		{
			i += 4;
		}
		else if ( ( c & 0xFE ) == 0xFC )	// 6-byte : 1111 1___
		{
			i += 5;
		}

		length += 1;
		c = str[ i++ ];
	}

	return length;
}

/// @desc Get the bytes and length of a utf8 string (NOT incluuding NULL terminator for length) (Including the NULL terminator for bytes)
void string_utf8_length_and_bytes( const char *str, u64 *length, u64 *bytes )
{
	assert( str && length && bytes );

	u64 len = 0;
	i32 i = 0;
	char c = str[ i++ ];

	while ( c )
	{
		if ( c >= 0 && c < 127 )			// 1-byte : 0___ ____
		{
		}
		else if ( ( c & 0xE0 ) == 0xC0 )	// 2-byte : 11__ ____
		{
			i += 1;
		}
		else if ( ( c & 0xF0 ) == 0xE0 )	// 3-byte : 111_ ____
		{
			i += 2;
		}
		else if ( ( c & 0xF8 ) == 0xF0 )	// 4-byte : 1111 ____
		{
			i += 3;
		}
		else if ( ( c & 0xFC ) == 0xF8 )	// 5-byte : 1111 1___
		{
			i += 4;
		}
		else if ( ( c & 0xFE ) == 0xFC )	// 6-byte : 1111 1___
		{
			i += 5;
		}

		len += 1;
		c = str[ i++ ];
	}

	*length = len;
	*bytes = i;
}

/// @return bytes written (NOT including the NULL terminator)
u64 string_utf8_copy( char *destination, u64 destSize, const char *source )
{
	assert( destSize >= 1 );
	assert( source );
	assert( destSize >= string_utf8_bytes( source ) );

	const char *sourceStart = source;

	while ( *source != '\0' )
		*destination++ = *source++;

	*destination = '\0';

	return source - sourceStart;
}

[[nodiscard]] u32 string_utf8_codepoint( const char *str, u32 *pSize )
{
	assert( pSize );

	char c = str[ 0 ];

	if ( c >= 0 && c < 127 )
	{
		// single-byte
		*pSize = 1;
		return c;
	}
	else if ( ( c & 0xE0 ) == 0xC0 )
	{
		// 2-byte
		u32 codepoint = ( ( str[ 0 ] & 0x1F ) << 6 )
			| ( ( str[ 1 ] & 0x3F ) );

		*pSize = 2;

		return codepoint;
	}
	else if ( ( c & 0xF0 ) == 0xE0 )
	{
		// 3-byte
		u32 codepoint = ( ( str[ 0 ] & 0xF ) << 12 )
			| ( ( str[ 1 ] & 0x3F ) << 6 )
			| ( ( str[ 2 ] & 0x3F ) );

		*pSize = 3;

		return codepoint;
	}
	else if ( ( c & 0xF8 ) == 0xF0 )
	{
		// 4-byte
		u32 codepoint = ( ( str[ 0 ] & 0x7 ) << 18 )
			| ( ( str[ 1 ] & 0x3F ) << 12 )
			| ( ( str[ 2 ] & 0x3F ) << 6 )
			| ( ( str[ 3 ] & 0x3F ) );

		*pSize = 4;

		return codepoint;
	}

	// NOTE : 5 & 6 byte characters not supported
	*pSize = 0;
	return 0;
}

i32 string_utf8_similarity( const char *lhs, const char *rhs )
{
	u64 lhsLength;
	u64 lhsBytes;
	u64 rhsLength;
	u64 rhsBytes;

	string_utf8_length_and_bytes( lhs, &lhsLength, &lhsBytes );
	string_utf8_length_and_bytes( rhs, &rhsLength, &rhsBytes );

	if ( lhsLength == 0 || rhsLength == 0 )
		return 0;

	i32 score = 0;
	u32 sequenceBonus = 0;
	u32 lhsIndex = 0;
	u32 rhsIndex = 0;
	u32 matches = 0;
	u32 matchSequential = 0;
	i32 completeLhsBonus = static_cast<i32>( lhsLength );
	u32 lhsSize;
	u32 rhsSize;
	u32 lhsCodepoint = string_utf8_lower_codepoint( string_utf8_codepoint( lhs, &lhsSize ) );
	u32 rhsCodepoint = string_utf8_lower_codepoint( string_utf8_codepoint( rhs, &rhsSize ) );

	if ( lhsCodepoint == rhsCodepoint )
		score += 1;

	while ( rhsIndex < rhsBytes )
	{
		rhsCodepoint = string_utf8_lower_codepoint( string_utf8_codepoint( &rhs[ rhsIndex ], &rhsSize ) );
		rhsIndex += rhsSize;

		while ( lhsIndex < lhsBytes )
		{
			lhsCodepoint = string_utf8_lower_codepoint( string_utf8_codepoint( &lhs[ lhsIndex ], &lhsSize ) );
			lhsIndex += lhsSize;

			if ( lhsCodepoint == rhsCodepoint )
			{
				matches += 1;

				score += 2 + sequenceBonus;
				sequenceBonus += 7;

				if ( ++matchSequential == lhsLength )
				{
					score += sequenceBonus + completeLhsBonus;
					if ( lhsSize == rhsSize )
						score *= 2;
				}

				if ( rhsIndex >= rhsBytes )
					break;

				rhsCodepoint = string_utf8_lower_codepoint( string_utf8_codepoint( &rhs[ rhsIndex ], &rhsSize ) );
				rhsIndex += rhsSize;
			}
			else
			{
				matchSequential = 0;
				sequenceBonus = 0;
			}
		}

		lhsIndex = 0;
	}

	return score;
}

[[nodiscard]] utf8Character string_utf8_encode( u32 codepoint )
{
	utf8Character character;

	if ( codepoint <= 0x7F )
	{
		// single-byte
		character.data[ 0 ] = static_cast<char>( codepoint );
		character.data[ 1 ] = '\0';
		return character;
	}
	else if ( codepoint <= 0x07FF )
	{
		// 2-byte
		character.data[ 0 ] = static_cast<char>( ( ( codepoint >> 6 ) & 0x1F ) | 0xC0 );
		character.data[ 1 ] = static_cast<char>( ( ( codepoint >> 0 ) & 0x3F ) | 0x80 );
		character.data[ 2 ] = '\0';
		return character;
	}
	else if ( codepoint <= 0xFFFF )
	{
		// 3-byte
		character.data[ 0 ] = static_cast<char>( ( ( codepoint >> 12 ) & 0x0F ) | 0xE0 );
		character.data[ 1 ] = static_cast<char>( ( ( codepoint >> 6 ) & 0x3F ) | 0x80 );
		character.data[ 2 ] = static_cast<char>( ( ( codepoint >> 0 ) & 0x3F ) | 0x80 );
		character.data[ 3 ] = '\0';
		return character;
	}
	else if ( codepoint <= 0x10FFFF )
	{
		// 4-byte
		character.data[ 0 ] = static_cast<char>( ( ( codepoint >> 18 ) & 0x07) | 0xF0 );
		character.data[ 1 ] = static_cast<char>( ( ( codepoint >> 12 ) & 0x3F) | 0x80 );
		character.data[ 2 ] = static_cast<char>( ( ( codepoint >> 6 ) & 0x3F) | 0x80 );
		character.data[ 3 ] = static_cast<char>( ( ( codepoint >> 0 ) & 0x3F) | 0x80 );
		character.data[ 4 ] = '\0';
		return character;
	}

	// NOTE : 5 & 6 byte characters not supported
	character.data[ 0 ] = '\0';
	return character;
}

[[nodiscard]] bool string_utf8_is_number( const char *str, bool *integer )
{
	if ( !str || *str == '\0' )
	{
		*integer = false;
		return false;
	}

	char c = *str++;
	i32 digits = 0;
	bool frac = false;

	// While its ascii keep checking
	while ( ( c & 0b10000000 ) == 0 )
	{
		if ( c >= '0' && c <= '9' )
		{
			digits += !frac;
		}
		else if ( c == '.' )
		{
			frac = true;
		}
		else if ( c == '\0' )
		{
			*integer = ( digits > 0 && !frac );
			return digits > 0;
		}
		else
		{
			*integer = false;
			return false;
		}

		c = *str++;
	}

	*integer = false;
	return false;
}

char *string_utf8_skip_codepoint( char *str, u32 *pSize, i32 num )
{
	if ( num <= 0 )
	{
		*pSize = 0;
		return str;
	}

	i32 length = 0;
	i32 i = 0;
	char c = str[ i++ ];

	while ( c )
	{
		if ( c >= 0 && c < 127 )			// 1-byte : 0___ ____
		{
		}
		else if ( ( c & 0xE0 ) == 0xC0 )	// 2-byte : 11__ ____
		{
			i += 1;
		}
		else if ( ( c & 0xF0 ) == 0xE0 )	// 3-byte : 111_ ____
		{
			i += 2;
		}
		else if ( ( c & 0xF8 ) == 0xF0 )	// 4-byte : 1111 ____
		{
			i += 3;
		}
		else if ( ( c & 0xFC ) == 0xF8 )	// 5-byte : 1111 1___
		{
			i += 4;
		}
		else if ( ( c & 0xFE ) == 0xFC )	// 6-byte : 1111 1___
		{
			i += 5;
		}

		if ( ++length >= num )
		{
			*pSize = i;
			return &str[ i ];
		}

		c = str[ i++ ];
	}

	*pSize = i;

	return &str[ i - 1 ];
}

[[nodiscard]] bool string_utf8_compare_value( const char *lhs, const char *rhs )
{
	assert( lhs );
	assert( rhs );

	u32 size;

	while ( *lhs != '\0' )
	{
		u32 codepointA = string_utf8_codepoint( lhs, &size );
		lhs += size;

		u32 codepointB = string_utf8_codepoint( rhs, &size );
		rhs += size;

		u32 diff = codepointA - codepointB;

		if ( diff != 0 )
			return diff;
	}

	return *lhs == *rhs;
}

void string_utf8_delete( char *str, i32 position )
{
	u32 size;
	str = string_utf8_skip_codepoint( str, &size, position );
	string_utf8_copy( str, string_utf8_bytes( str ), string_utf8_skip_codepoint( str, &size, 1 ) );
}

void string_utf8_pop( char *str )
{
	u64 len = string_utf8_bytes( str ) - 1;
	char *txt = str + len;

	while ( !string_utf8_is_leading_byte( *txt-- ) )
		len -= 1;
	len -= 1;

	if ( len >= 0 )
		str[ len ] = '\0';
}

void string_utf8_pop( char *str, i32 num )
{
	u64 len = string_utf8_bytes( str ) - 1;
	char *txt = str + len;

	for ( i32 i = 0; i < num; ++i )
	{
		while ( !string_utf8_is_leading_byte( *txt-- ) )
			len -= 1;

		if ( --len == 0 )
			break;

		txt -= 1;
	}

	if ( len >= 0 )
		str[ len ] = '\0';
}

[[nodiscard]] const char *string_utf8_get_ext( const char *str )
{
	if ( str == nullptr || *str == '\0' )
		return nullptr;

	u64 len = string_utf8_bytes( str );
	const char *strStart = str;
	str += len;

	while ( --str != strStart )
		if ( *str == '.' )
			return str + 1;

	return ( *str == '.' ? str + 1 : nullptr );
}

void string_utf8_trim_ext( char *str )
{
	u64 len = string_utf8_bytes( str );
	char *txt = str + len - 1;

	while ( len > 0 )
	{
		if ( *txt == '.' )
		{
			*txt = '\0';
			return;
		}

		len -= 1;
		txt -= 1;
	}
}

[[nodiscard]] inline bool string_utf8_has_ext( const char *str )
{
	return string_utf8_get_ext( str ) != nullptr;
}

[[nodiscard]] bool string_utf8_has_ext( const char *str, const char *ext )
{
	u64 len = string_utf8_bytes( str );
	const char *txt = str + len - 1;

	while ( len > 0 )
	{
		if ( *txt == '.' )
			return ext != nullptr && string_utf8_compare( txt += ( ext[ 0 ] != '.' ), ext );

		len -= 1;
		txt -= 1;
	}

	return ext == nullptr;
}

[[nodiscard]] const char *string_utf8_get_filename( const char *str )
{
	assert( str );

	const char *p = str++;
	char c = *p;

	while ( c != '\0' )
	{
		if ( c == '/' || c == '\\' )
			p = str;

		c = *str++;
	}

	return p;
}

[[nodiscard]] char *string_utf8_filename( char *str )
{
	assert( str );

	char *p = str++;
	char c = *p;

	while ( c != '\0' )
	{
		if ( c == '/' || c == '\\' )
			p = str;

		c = *str++;
	}

	return p;
}

const char *string_utf8_copy_path( char *dest, u64 destSize, const char *str )
{
	assert( dest && str );

	const char *p = str;
	const char *found = nullptr;
	char c = *p;

	while ( c != '\0' )
	{
		if ( c == '/' || c == '\\' )
			found = p;

		c = *++p;
	}

	if ( found )
	{
		string_utf8_copy( dest, destSize, str, ( found - str ) + 1 );
		return dest;
	}

	return "";
}

[[nodiscard]] u64 string_utf8_find_first( const char *str, const char *find )
{
	assert( str && find );

	const char *start = str;

	u32 size;
	u32 codePoint;

	u32 findSize;
	u32 findCodePoint = string_utf8_codepoint( find, &findSize );

	while ( *str != '\0' )
	{
		codePoint = string_utf8_codepoint( str, &size );
		if ( size == findSize && codePoint == findCodePoint )
			return str - start;
		str += size;
	}

	return UINT64_MAX;
}

[[nodiscard]] const char *string_utf8_until( const char *str, const char *until, Allocator *allocator )
{
	assert( str && until );

	u64 find = string_utf8_find_first( str, until );
	if ( find == UINT64_MAX )
		return str;

	char *newString = allocator->allocate<char>( find );
	string_utf8_copy( newString, find + 1, str, find );
	return newString;
}

[[nodiscard]] const char *string_utf8_past_start( const char *str, const char *start )
{
	assert( str && start );

	while ( *str == *start++ )
	{
		if ( *str == '\0' )
			return str;
		str += 1;
	}

	return str;
}

[[nodiscard]] char *string_utf8_past_start_case_insensitive( char *str, char *start )
{
	assert( str && start );

	while ( ascii_char_lower( *str ) == ascii_char_lower( *start++ ) )
	{
		if ( *str == '\0' )
			return str;
		str += 1;
	}

	return str;
}

[[nodiscard]] const char *string_utf8_past_start_case_insensitive( const char *str, const char *start )
{
	assert( str && start );

	while ( ascii_char_lower( *str ) == ascii_char_lower( *start++ ) )
	{
		if ( *str == '\0' )
			return str;
		str += 1;
	}

	return str;
}

[[nodiscard]] bool string_utf8_has_character( const char *str, const char *character )
{
	assert( str && character );

	u32 size, charSize;
	u32 charCodepoint = string_utf8_codepoint( character, &charSize );

	while ( *str )
	{
		if ( string_utf8_is_leading_byte( *str ) )
		{
			if ( charCodepoint == string_utf8_codepoint( str, &size ) )
				return true;

			str += size;
		}
		else
		{
			str += 1;
		}
	}

	return false;
}

u64 string_utf8_insert( char *destination, u64 destSize, const char *insert, i32 index )
{
	u64 p = string_utf8_bytes( destination ) - 1; // -1 is OK because only 1 requires a null terminator to count
	u64 insertBytes = string_utf8_bytes( insert );

	// Check there is enough room to insert
	if ( ( destSize - p ) < insertBytes )
		return 0;

	u32 insertIndexSize;
	char *newDest = string_utf8_skip_codepoint( destination, &insertIndexSize, index );
	assert( newDest );

	insertBytes -= 1; // Don't include the null terminator

	// Shuffle the destination up from where the insert will go (start at the end and work back)
	char *src = destination + p;
	char *dst = src + insertBytes;
	while ( src >= newDest )
		*dst-- = *src--;

	// Insert the new data
	u64 bytesToInsert = insertBytes;
	while ( bytesToInsert-- )
		*newDest++ = *insert++;

	// Doesn't include null terminator
	return insertBytes;
}

/// @desc Returns a count of characters until a delimiter is found
///       While marked as utf8 the delimiter should be ASCII only
[[nodiscard]] u64 string_utf8_string_span( const char *tok, const char *delim )
{
	assert( tok && delim );

	u64 i, j;

	for ( i = 0; tok[ i ] != '\0'; ++i )
		for ( j = 0; delim[ j ] != '\0'; ++j )
			if ( tok[ i ] == delim[ j ] )
				return i;

	return i;
}

/// @desc Returns a count of characters until a non delimiter is found
///       While marked as utf8 the delimiter should be ASCII only
[[nodiscard]] u64 string_utf8_string_nspan( const char *tok, const char *delim )
{
	assert( tok && delim );

	u64 i, j;

	for ( i = 0; tok[ i ] != '\0'; ++i )
	{
		bool delimFound = false;

		for ( j = 0; delim[ j ] != '\0'; ++j )
		{
			if ( tok[ i ] == delim[ j ] )
			{
				delimFound = true;
				break;
			}
		}

		if ( !delimFound )
			return i;
	}

	return i;
}

/// @desc Output a string into token, a string split by the delimiters
///       While marked as utf8 the delimiter should be ASCII only
///       Also note it will cannibalise the input string ( do not use on string literals )
[[nodiscard]] char *string_utf8_tokenise( char *str, const char *delim, const char **token, char *found )
{
	// Invalid input
	if ( !str )
	{
		*token = nullptr;
		return nullptr;
	}

	// Add a count of characters until a NON delimiter is found
	str += string_utf8_string_nspan( str, delim );

	// No NON delimiter found, reached end of string
	if ( *str == '\0' )
	{
		*token = nullptr;
		return nullptr;
	}

	*token = str;

	// Set e to str + a count of characters until a delimiter IS found
	char *e = str + string_utf8_string_span( str, delim );

	// If there was a break on \r && the next char is \n && you was looking for \r\n
	// Then return the delim found as \n
	u64 crlfSkip = 0;
	if ( *e == '\r' && *( e + 1 ) == '\n' )
	{
		crlfSkip = 1;
		if ( found )
			*found = '\n';
	}
	// Which delimiter was found
	else if ( found )
		*found = *e;

	// If it didn't reached the end of the string
	// it will need a null terminator inserting and
	// the new position of the string returned
	if ( *e != '\0' )
	{
		*e = '\0';
		return e + 1 + crlfSkip;
	}

	return nullptr;
}

char *string_utf8_replace_ascii_char( char *str, char find, char replace )
{
	assert( str );

	char *start = str;
	u32 size;
	u32 findCodepoint = static_cast<u32>( find );

	while ( *str )
	{
		if ( string_utf8_is_leading_byte( *str ) )
		{
			if ( findCodepoint == string_utf8_codepoint( str, &size ) )
			{
				*str = replace;
			}

			str += size;
		}
		else
		{
			str += 1;
		}
	}

	return start;
}

// UTF-16 ///////////////////////////////////////////////////////////////////////////////
[[nodiscard]] u32 string_utf16_codepoint( const u16 *str, u32 *pSize )
{
	if ( !string_utf16_surrogate_pair_high( *str ) )
	{
		*pSize = 1;
		return *str;
	}

	u16 high = *str++;
	u16 low = *str++; 

	assert( string_utf16_surrogate_pair_low( low ) );

	*pSize = 2;

	u32 codepoint = static_cast<u32>( high & SURROGATE_CODEPOINT_MASK ) << SURROGATE_CODEPOINT_BITS;
	codepoint |= low & SURROGATE_CODEPOINT_MASK;
	return codepoint + SURROGATE_CODEPOINT_OFFSET;
}

[[nodiscard]] utf16Character string_utf16_encode( u32 codepoint )
{
	utf16Character character;

	// BMP
	if ( codepoint <= 0xFFFF )
	{
		character.data[ 0 ] = static_cast<u16>( codepoint );
		character.data[ 1 ] = '\0';
		return character;
	}

	// SURROGATE
	codepoint -= SURROGATE_CODEPOINT_OFFSET;
	character.data[ 0 ] = SURROGATE_CODEPOINT_HIGH_START + static_cast<u16>( ( codepoint >> SURROGATE_CODEPOINT_BITS ) & SURROGATE_CODEPOINT_MASK );
	character.data[ 1 ] = SURROGATE_CODEPOINT_LOW_START + static_cast<u16>( codepoint & SURROGATE_CODEPOINT_MASK );
	character.data[ 2 ] = '\0';

	return character;
}