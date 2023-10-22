
[[nodiscard]] u64 convert_to_u64( const char *input, const char **output )
{
	const char *start = input;

	// remove leading whitespace
	while ( *input == ' ' )
		input += 1;

	// check it's a number or invalid input
	if ( ( *input < '0' || *input > '9' ) && *input != '+' )
	{
		if ( output )
			*output = start;
		return 0;
	}

	u64 base = 0;

	bool overflow = false;

	while ( ( *input >= '0' && *input <= '9' ) )
	{
		overflow = overflow || base > ( SIZE_MAX / 10 ) || ( base == ( SIZE_MAX / 10 ) && ( *input - '0' ) > 7 );

		base = 10 * base + ( *input++ - '0' );
	}

	if ( output )
		*output = input;

	return !overflow ? base : SIZE_MAX;
}

[[nodiscard]] i64 convert_to_i64( const char *input, const char **output )
{
	const char *start = input;

	// remove leading whitespace
	while ( *input == ' ' )
		input += 1;

	// check it's a number or invalid input
	if ( ( *input < '0' || *input > '9' ) && *input != '-' && *input != '+' )
	{
		if ( output )
			*output = start;
		return 0;
	}

	i64 sign = 1;
	i64 base = 0;

	// sign
	if ( *input == '-' || *input == '+' )
		sign = 1 - 2 * ( *input++ == '-' );

	bool overflow = false;

	while ( ( *input >= '0' && *input <= '9' ) )
	{
		overflow = overflow || base > ( INT_MAX / 10 ) || ( base == ( INT_MAX / 10 ) && ( *input - '0' ) > 7 );

		base = 10 * base + ( *input++ - '0' );
	}

	if ( output )
		*output = input;

	return !overflow ? ( base * sign ) : ( sign >= 0 ? INT64_MAX : INT64_MIN );
}

[[nodiscard]] f32 convert_to_float( const char *input, const char **output )
{
	const char *start = input;

	// remove leading whitespace
	while ( *input == ' ' )
		input += 1;

	f32 sign = 1.0f;
	f32 base = 0.0f;
	bool overflow = false;
	bool foundPoint = false;
	char c = *input;

	// check it's a number or invalid input
	if ( ( c < '0' || c > '9' ) && c != '.' && c != '-' && c != '+' )
	{
		if ( output )
			*output = start;
		return 0.f;
	}

	// sign
	if ( c == '-' || c == '+' )
		sign = 1.0f - 2.0f * ( *input++ == '-' );

	// starting . :eg: .5
	if ( *input == '.' )
	{
		input += 1;
		foundPoint = true;
	}

	c = *input++;

	while ( ( c >= '0' && c <= '9' ) || c == '.' )
	{
		if ( c == '.' )
		{
			c = *input++;

			if ( foundPoint )
			{
				return 0.f;
			}
			foundPoint = true;
			continue;
		}

		if ( foundPoint )
			sign /= 10.0f;

		overflow = overflow || base > ( FLT_MAX / 10.0f ) || ( base == ( FLT_MAX / 10.0f ) && ( c - '0' ) > 7 );

		base = 10.0f * base + ( c - '0' );

		c = *input++;
	}

	if ( output )
		*output = input - 1;

	return !overflow ? ( base * sign ) : ( sign >= 0 ? FLT_MAX : -FLT_MAX );
}

[[nodiscard]] bool convert_to_bool( const char *input, const char **output )
{
	const char *start = input;

	// remove leading whitespace
	while ( *input == ' ' )
		input += 1;

	const char *trueStr = "true";
	const char *falseStr = "false";
	bool value = ( ascii_char_lower( *input ) == *trueStr );
	const char *checkStr = value ? trueStr : falseStr;

	while ( *checkStr )
	{
		// If a character is different, return false, but don't advance output
		if ( ascii_char_lower( *input++ ) != *checkStr++ )
		{
			if ( output )
				*output = start;
			return false;
		}
	}

	// Advance the output if required
	if ( output )
		*output = input;

	return value;
}

[[nodiscard]] bool is_floating_point( const char *input )
{
	// remove leading whitespace
	while ( *input == ' ' )
		input += 1;

	// ignore leading +/-
	if ( *input == '-' || *input == '+' )
		input += 1;

	// search for non number
	while ( *input >= '0' && *input <= '9' )
		input += 1;

	// if it ended with a . , its a floating point number
	return *input == '.';
}

u64 convert_to_string( char *dest, u64 destSize, u64 value, i32 radix, i32 trailing )
{
	assert( radix <= 256 );
	assert( trailing <= 32 );

	char tmp[ 64 ];
	char *tp = tmp;

	char i;

	while ( value || tp == tmp )
	{
		i = static_cast<char>( value % radix );
		value /= radix;

		if ( i < 10 )
			*tp++ = i + '0';
		else
			*tp++ = i + 'a' - 10;
	}

	u64 len = tp - tmp;

	// Check if trailing 0's are needed
	while ( len < trailing )
	{
		*tp++ = '0';
		len += 1;
	}

	assert( destSize + 1 >= len );

	while ( tp > tmp )
		*dest++ = *--tp;

	*dest = '\0';

	// Digits of string
	return len;
}

u64 convert_to_string( char *dest, u64 destSize, i64 value, i32 radix, i32 trailing )
{
	assert( radix <= 256 );
	assert( trailing <= 32 );

	char tmp[ 64 ];
	char *tp = tmp;

	char i;
	u64 v;

	i64 sign = ( radix == 10 && value < 0 );
	if ( sign )
		v = -value;
	else
		v = static_cast<u64>( value );

	while ( v || tp == tmp )
	{
		i = static_cast<char>( v % radix );
		v /= radix;

		if ( i < 10 )
			*tp++ = i + '0';
		else
			*tp++ = i + 'a' - 10;
	}

	u64 len = tp - tmp;

	// Check if trailing 0's are needed
	while ( len < trailing )
	{
		*tp++ = '0';
		len += 1;
	}

	if ( sign )
	{
		*dest++ = '-';
		len += 1;
	}

	assert( destSize + 1 >= len );

	while ( tp > tmp )
		*dest++ = *--tp;

	*dest = '\0';

	// Digits of string
	return len;
}

u64 convert_to_string( char *dest, u64 destSize, f32 value, i32 fracDigits )
{
	char *start = dest;
	i32 integer = static_cast<i32>( value );
	f32 fraction = value - static_cast<f32>( integer );

	u64 size = convert_to_string( dest, destSize, integer, 10 );

	dest += size;

	if ( fracDigits != 0 )
	{
		*dest++ = '.';

		i32 fd = abs( fracDigits );

		u64 fracSize = convert_to_string( dest, destSize - ( size + 1 ), static_cast<i32>( fraction * pow( 10, fd ) ), 10, fd );
		dest += fracSize;

		// Negative fracDigits means to cut trailing zeros (2.0000000 > 2.0)
		if ( fracDigits < 0 )
		{
			dest -= 1;
			while ( *dest == '0' )
				dest -= 1;
			if ( *dest == '.' )
				dest += 1;
			*++dest = '\0';
		}
	}

	return dest - start;
}

u64 convert_to_string( char *dest, u64 destSize, bool value )
{
	return string_utf8_copy( dest, destSize, value ? "true" : "false" );
}

[[nodiscard]] const char *convert_to_string( u8 value, i32 radix, i32 trailing )
{
	Allocator *allocator = &app.memoryArena.transient;
	char *text = allocator->allocate<char>( MAX_CONVERT_TO_STRING_DIGITS );
	convert_to_string( text, MAX_CONVERT_TO_STRING_DIGITS, value, radix, trailing );
	allocator->shrink( text, string_utf8_bytes( text ) );
	return text;
}

[[nodiscard]] const char *convert_to_string( u16 value, i32 radix, i32 trailing )
{
	Allocator *allocator = &app.memoryArena.transient;
	char *text = allocator->allocate<char>( MAX_CONVERT_TO_STRING_DIGITS );
	convert_to_string( text, MAX_CONVERT_TO_STRING_DIGITS, value, radix, trailing );
	allocator->shrink( text, string_utf8_bytes( text ) );
	return text;
}

[[nodiscard]] const char *convert_to_string( u32 value, i32 radix, i32 trailing )
{
	Allocator *allocator = &app.memoryArena.transient;
	char *text = allocator->allocate<char>( MAX_CONVERT_TO_STRING_DIGITS );
	convert_to_string( text, MAX_CONVERT_TO_STRING_DIGITS, value, radix, trailing );
	allocator->shrink( text, string_utf8_bytes( text ) );
	return text;
}

[[nodiscard]] const char *convert_to_string( u64 value, i32 radix, i32 trailing )
{
	Allocator *allocator = &app.memoryArena.transient;
	char *text = allocator->allocate<char>( MAX_CONVERT_TO_STRING_DIGITS );
	convert_to_string( text, MAX_CONVERT_TO_STRING_DIGITS, value, radix, trailing );
	allocator->shrink( text, string_utf8_bytes( text ) );
	return text;
}

[[nodiscard]] const char *convert_to_string( i8 value, i32 radix, i32 trailing )
{
	Allocator *allocator = &app.memoryArena.transient;
	char *text = allocator->allocate<char>( MAX_CONVERT_TO_STRING_DIGITS );
	convert_to_string( text, MAX_CONVERT_TO_STRING_DIGITS, value, radix, trailing );
	allocator->shrink( text, string_utf8_bytes( text ) );
	return text;
}

[[nodiscard]] const char *convert_to_string( i16 value, i32 radix, i32 trailing )
{
	Allocator *allocator = &app.memoryArena.transient;
	char *text = allocator->allocate<char>( MAX_CONVERT_TO_STRING_DIGITS );
	convert_to_string( text, MAX_CONVERT_TO_STRING_DIGITS, value, radix, trailing );
	allocator->shrink( text, string_utf8_bytes( text ) );
	return text;
}

[[nodiscard]] const char *convert_to_string( i32 value, i32 radix, i32 trailing )
{
	Allocator *allocator = &app.memoryArena.transient;
	char *text = allocator->allocate<char>( MAX_CONVERT_TO_STRING_DIGITS );
	convert_to_string( text, MAX_CONVERT_TO_STRING_DIGITS, value, radix, trailing );
	allocator->shrink( text, string_utf8_bytes( text ) );
	return text;
}

[[nodiscard]] const char *convert_to_string( i64 value, i32 radix, i32 trailing )
{
	Allocator *allocator = &app.memoryArena.transient;
	char *text = allocator->allocate<char>( MAX_CONVERT_TO_STRING_DIGITS );
	convert_to_string( text, MAX_CONVERT_TO_STRING_DIGITS, value, radix, trailing );
	allocator->shrink( text, string_utf8_bytes( text ) );
	return text;
}

[[nodiscard]] const char *convert_to_string( f32 value, i32 fracDigits )
{
	Allocator *allocator = &app.memoryArena.transient;
	char *text = allocator->allocate<char>( MAX_CONVERT_TO_STRING_DIGITS );
	convert_to_string( text, MAX_CONVERT_TO_STRING_DIGITS, value, fracDigits );
	allocator->shrink( text, string_utf8_bytes( text ) );
	return text;
}

[[nodiscard]] const char *convert_to_string( bool value )
{
	Allocator *allocator = &app.memoryArena.transient;
	char *text = allocator->allocate<char>( MAX_CONVERT_TO_STRING_DIGITS );
	convert_to_string( text, MAX_CONVERT_TO_STRING_DIGITS, value );
	allocator->shrink( text, string_utf8_bytes( text ) );
	return text;
}