
#pragma once

template <typename Type, u64 Capacity>
struct Array
{
	u64 count = 0;
	Type data[ Capacity ];

	inline void add( Type &t )
	{
		assert( count < Capacity );
		data[ count++ ] = t;
	}

	inline void add( const Type &t )
	{
		assert( count < Capacity );
		data[ count++ ] = t;
	}

	inline void add_no_bounds_check( const Type &t )
	{
		data[ count++ ] = t;
	}

	template <typename ...Args>
	inline void add_no_bounds_check( const Type &t, Args&&... args )
	{
		data[ count++ ] = t;
		add_no_bounds_check( args... );
	}

	template <typename ...Args>
	void add( const Type &t, Args&&... args )
	{
		assert( count + sizeof...( Args ) <= Capacity );
		data[ count++ ] = t;
		add_no_bounds_check( args... );
	}

	inline void add_unique( Type &t )
	{
		if ( !has_value( t ) )
			add( t );
	}

	inline void append( const Type *t, u64 appentCount )
	{
		assert( count + appentCount <= Capacity );

		Type *p = &data[ count ];

		for ( u64 i = 0; i < appentCount; ++i )
			*p++ = *t++;

		count += appentCount;
	}

	inline void append_and_offset( const Type *t, const Type &offset, u64 appentCount )
	{
		assert( count + appentCount <= Capacity );

		Type *p = &data[ count ];

		for ( u64 i = 0; i < appentCount; ++i )
			*p++ = ( *t++ ) + offset;

		count += appentCount;
	}

	template <u64 otherArraySize>
	inline void append( const Array<Type, otherArraySize> &arr )
	{
		append( arr.data, arr.count );
	}

	template <u64 otherArraySize>
	inline void append_and_offset( const Array<Type, otherArraySize> &arr, const Type &offset )
	{
		append_and_offset( arr.data, offset, arr.count );
	}

	template <typename ...Args>
	inline void set( u64 idx, Type &t )
	{
		assert( idx < count );
		data[ idx ] = t;
	}

	template <typename ...Args>
	inline void set_no_bounds_check( u64 idx, Type &t )
	{
		data[ idx ] = t;
	}

	template <typename ...Args>
	inline void set( u64 idx, Type &t, Args&&... args )
	{
		assert( idx + sizeof...( Args ) <= count );
		data[ idx ] = t;
		set_no_bounds_check( idx + 1, args... );
	}

	template <typename ...Args>
	inline void set( u64 idx, const Type &t )
	{
		assert( idx < count );
		data[ idx ] = t;
	}

	template <typename ...Args>
	inline void set_no_bounds_check( u64 idx, const Type &t )
	{
		data[ idx ] = t;
	}

	template <typename ...Args>
	inline void set( u64 idx, const Type &t, const Args&&... args )
	{
		assert( idx + sizeof...( Args ) <= count );
		data[ idx ] = t;
		set_no_bounds_check( idx + 1, args... );
	}

	void set_blank( bool totalCapacity = false )
	{
		memset( this, 0, sizeof( *this ) );

		if ( totalCapacity )
			count = Capacity;
	}

	void set_all( Type &value, bool totalCapacity )
	{
		if ( totalCapacity )
			count = Capacity;

		for ( u64 i = 0; i < count; ++i )
			data[ i ] = value;
	}

	void set_all( const Type &value, bool totalCapacity )
	{
		if ( totalCapacity )
			count = Capacity;

		for ( u64 i = 0; i < count; ++i )
			data[ i ] = value;
	}

	inline void set_full()
	{
		count = Capacity;
	}

	inline void resize( u64 size )
	{
		assert( size <= Capacity );
		count = size;
	}

	inline void clear()
	{
		count = 0;
	}

	inline void swap_and_remove( u64 idx )
	{
		assert( idx < count );
		data[ idx ] = data[ --count ];
	}

	void remove( u64 idx )
	{
		assert( idx < count );
		// Maintains order by shuffling everything down 1
		for ( --count; idx < count; ++idx )
			data[ idx ] = data[ idx + 1 ];
	}

	[[nodiscard]] inline Type &at( u64 idx )
	{
		assert( idx < count );
		return data[ idx ];
	}

	[[nodiscard]] inline Type &at_no_bounds_check( u64 idx )
	{
		return data[ idx ];
	}

	[[nodiscard]] inline Type & operator[] ( u64 idx )
	{
		assert( idx < count );
		return data[ idx ];
	}

	[[nodiscard]] inline const Type & operator[] ( u64 idx ) const
	{
		assert( idx < count );
		return data[ idx ];
	}

	[[nodiscard]] inline Type *ptr()
	{
		assert( count > 0 );
		return &data[ 0 ];
	}

	[[nodiscard]] inline Type &first()
	{
		assert( count > 0 );
		return data[ 0 ];
	}

	[[nodiscard]] inline Type &last()
	{
		assert( count > 0 );
		return data[ count - 1 ];
	}

	[[nodiscard]] inline Type &top()
	{
		assert( count > 0 );
		return data[ count - 1 ];
	}

	[[nodiscard]] inline Type &back()
	{
		assert( count > 0 );
		return data[ count - 1 ];
	}

	inline Type &push()
	{
		assert( count < Capacity );
		return data[ count++ ];
	}

	inline Type &pop()
	{
		assert( count > 0 );
		return data[ --count ];
	}

	inline void pop_back()
	{
		assert( count > 0 );
		count -= 1;
	}

	inline Type &pop_index( u64 idx )
	{
		assert( idx < count );
		Type t = data[ idx ];
		data[ idx ] = data[ --count ];
		data[ count ] = t;
		return data[ count ];
	}

	[[nodiscard]] inline bool has_value( const Type &t ) const
	{
		const Type *p = data;
		for ( u64 i = 0; i < count; ++i )
			if ( *p++ == t )
				return true;
		return false;
	}

	bool find_and_remove_value_keep_order( const Type &t )
	{
		const Type *p = data;
		for ( u64 i = 0; i < count; ++i )
		{
			if ( *p++ == t )
			{
				remove( i );
				return true;
			}
		}
		return false;
	}

	bool find_and_remove_value( const Type &t )
	{
		const Type *p = data;
		for ( u64 i = 0; i < count; ++i )
		{
			if ( *p++ == t )
			{
				swap_and_remove( i );
				return true;
			}
		}
		return false;
	}

	u64 find_and_remove_all_values( const Type &t )
	{
		const Type *p = data;
		u64 startCount = count;
		for ( u64 i = 0; i < count; ++i )
			if ( *p++ == t )
				swap_and_remove( i-- );
		return startCount - count;
	}

	bool find_and_remove_address_keep_order( const Type *t )
	{
		const Type *p = data;
		for ( u64 i = 0; i < count; ++i )
		{
			if ( p++ == t )
			{
				remove( i );
				return true;
			}
		}
		return false;
	}

	bool find_and_remove_address( const Type *t )
	{
		const Type *p = data;
		for ( u64 i = 0; i < count; ++i )
		{
			if ( p++ == t )
			{
				swap_and_remove( i );
				return true;
			}
		}
		return false;
	}

	u64 find_and_remove_all_addresses( const Type *t )
	{
		const Type *p = data;
		u64 startCount = count;
		for ( u64 i = 0; i < count; ++i )
			if ( p++ == t )
				swap_and_remove( i-- );
		return startCount - count;
	}

	[[nodiscard]] inline bool empty() const
	{
		return count == 0;
	}

	[[nodiscard]] inline bool full() const
	{
		return count == Capacity;
	}

	[[nodiscard]] constexpr inline u64 capacity() const
	{
		return Capacity;
	}

	[[nodiscard]] inline u64 bytes() const
	{
		return sizeof( Type ) * count;
	}

	inline void swap( u64 entry1, u64 entry2 )
	{
		if ( entry1 == entry2 )
			return;

		assert( entry1 < count );
		assert( entry2 < count );

		Type temp = data[ entry1 ];
		data[ entry1 ] = data[ entry2 ];
		data[ entry2 ] = temp;
	}

	inline void sort()
	{
		if ( count > 0 )
			sort( 0, count - 1 );
	}

	void sort( u64 left, u64 right )
	{
		if ( left >= right )
			return;

		u64 last = left;

		// Pivot goes into the first position (so it isnt moved while going through all the values)
		swap( left, ( left + right ) / 2 );

		// Go through entries left+1 to right, put lower values
		// on the left of the pivot. last is the end of that left array
		// left + 1 because the pivot is stored at left
		for ( u64 i = left + 1; i <= right; ++i )
			if ( compare_value( data[ left ], data[ i ] ) > 0 )
				swap( i, ++last );

		// Put the pivot value to the end of the left-side (all the values are lower than it)
		swap( last, left );

		if ( last > 0 )
			sort( left, last - 1 );
		sort( last + 1, right );
	}
};