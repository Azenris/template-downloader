
#pragma once

#define INVALID_MAP_INDEX		( UINT64_MAX )

// TRANSFORM ////////////////////////////////////////////////////////////////////
template <typename T>
struct MapTransformKey
{
	using Type = T;
};

template <u64 Size>
struct MapTransformKey<char [Size]>
{
	using Type = const char *;
};

template <u64 Size>
struct MapTransformKey<const char [Size]>
{
	using Type = const char *;
};

// PAIR /////////////////////////////////////////////////////////////////////////
template <typename First, typename Second>
struct Pair
{
	First first;
	Second second;
};

template <typename First, typename Second>
[[nodiscard]] inline bool operator == ( const Pair<First, Second> &lhs, const Pair<First, Second> &rhs )
{
	return lhs.first == rhs.first && lhs.second == rhs.second;
}

// HASHERS //////////////////////////////////////////////////////////////////////
template <typename Key>
struct MapHash
{
};

template <>
struct MapHash<u64>
{
	static u64 create( u64 key )
	{
		return key;
	}
};

template <>
struct MapHash<u32>
{
	static u64 create( u32 key )
	{
		return key;
	}
};

template <>
struct MapHash<i32>
{
	static u64 create( i32 key )
	{
		return key;
	}
};

template <>
struct MapHash<u16>
{
	static u64 create( u16 key )
	{
		return key;
	}
};

template <>
struct MapHash<Pair<u32, u32>>
{
	static u64 create( const Pair<u32, u32> &key )
	{
		return MapHash<u32>::create( key.first ) ^ MapHash<u32>::create( key.second );
	}
};

template <>
struct MapHash<Pair<u64, u64>>
{
	static u64 create( const Pair<u64, u64> &key )
	{
		return MapHash<u64>::create( key.first ) ^ MapHash<u64>::create( key.second );
	}
};

template <>
struct MapHash<Pair<i32, i32>>
{
	static u64 create( const Pair<i32, i32> &key )
	{
		return MapHash<i32>::create( key.first ) ^ MapHash<i32>::create( key.second );
	}
};

template <>
struct MapHash<char *>
{
	// djb2
	static u64 create( const char *key )
	{
		u64 hash = 5381;
		i32 c = *key++;

		while ( c )
		{
			hash = ( ( hash << 5 ) + hash ) + c;
			c = *key++;
		}

		return hash;
	}
};

template <>
struct MapHash<const char *>
{
	// djb2
	static u64 create( const char *key )
	{
		u64 hash = 5381;
		i32 c = *key++;

		while ( c )
		{
			hash = ( ( hash << 5 ) + hash ) + c;
			c = *key++;
		}

		return hash;
	}
};

// COMPARERS ////////////////////////////////////////////////////////////////////
template <typename Key>
struct MapKeyCompare
{
	static bool compare( const Key &lhs, const Key &rhs )
	{
		return lhs == rhs;
	}
};

template <>
struct MapKeyCompare<char *>
{
	static bool compare( const char *lhs, const char *rhs )
	{
		return string_utf8_compare( lhs, rhs );
	}
};

template <>
struct MapKeyCompare<const char *>
{
	static bool compare( const char *lhs, const char *rhs )
	{
		return string_utf8_compare( lhs, rhs );
	}
};

// ASSIGNMENT ///////////////////////////////////////////////////////////////////
template <typename Key>
struct MapKeyAssignment
{
	static void assign( Key &lhs, const Key &rhs )
	{
		lhs = rhs;
	}
};

template <>
struct MapKeyAssignment<const char *>
{
	template <u64 Size>
	static void assign( char( &lhs )[ Size ], const char *rhs )
	{
		string_utf8_copy( lhs, Size, rhs );
	}

	static void assign( const char *&lhs, const char *rhs )
	{
		lhs = rhs;
	}
};

// MAP //////////////////////////////////////////////////////////////////////////
template <typename Key, typename Value, u64 Capacity, u64 Buckets = Capacity>
struct Map
{
	using KeyType = MapTransformKey<Key>::Type;
	using KeyHash = MapHash<KeyType>;
	using KeyCompare = MapKeyCompare<KeyType>;
	using KeyAssign = MapKeyAssignment<KeyType>;

	struct Entry
	{
		Key key;				// the key used in hash
		Value value;			// the actual value
		u64 bucket;				// bucket id
		u64 prev;				// prev entry in values [same bucket]
		u64 next;				// next entry in values [same bucket]
		u64 idx;				// entry in values
	};

	Array<Entry, Capacity> values;
	Array<u64, Buckets> entries;

	[[nodiscard]] Entry *push( const KeyType &key )
	{
		u64 hash = KeyHash::create( key ) % Buckets;

		if ( hash >= entries.count )
			for ( u64 i = entries.count; i <= hash; ++i )
				entries.add( INVALID_MAP_INDEX );

		// Check if this key already exists
		u64 prev = entries[ hash ];
		while ( prev != INVALID_MAP_INDEX )
		{
			Entry *entry = &values[ prev ];
			if ( KeyCompare::compare( entry->key, key ) )
				return entry;
			prev = entry->next;
		}

		if ( values.full() )
			return nullptr;

		Entry *entry = &values.push();
		u64 idx = values.count - 1;
		u64 next = entries[ hash ];

		// Tell the previous root entry this one is now root
		if ( next != INVALID_MAP_INDEX )
		{
			assert( values[ next ].prev == INVALID_MAP_INDEX );
			values[ next ].prev = idx;
		}

		// Setup the new entry
		KeyAssign::assign( entry->key, key );
		entry->bucket = hash;
		entry->prev = INVALID_MAP_INDEX;
		entry->next = next;
		entry->idx = idx;

		// This new entry becomes the root
		entries[ hash ] = idx;

		return entry;
	}

	Value *push_get( const KeyType &key )
	{
		Entry *entry = push( key );
		if ( !entry )
			return nullptr;
		return &entry->value;
	}

	Value *insert_get( const KeyType &key, const Value &value )
	{
		Entry *entry = push( key );
		if ( !entry )
			return nullptr;
		entry->value = value;
		return &entry->value;
	}

	bool insert( const KeyType &key, const Value &value )
	{
		Entry *entry = push( key );
		if ( !entry )
			return false;
		entry->value = value;
		return true;
	}

	bool change_key( const KeyType &oldKey, const KeyType &newKey )
	{
		if ( !remove( oldKey ) )
			return false;

		// After removing the old data will be at the last position
		// use .data[ x ] to avoid validation of index (will be out of bounds)
		bool inserted = insert( newKey, values.data[ values.count ].value );

		assert( inserted );

		return inserted;
	}

	bool remove( const KeyType &key )
	{
		u64 hash = KeyHash::create( key ) % Buckets;

		if ( hash >= entries.count )
			return false;

		u64 idx = entries[ hash ];

		while ( idx != INVALID_MAP_INDEX )
		{
			Entry *entry = &values[ idx ];

			if ( KeyCompare::compare( entry->key, key ) )
			{
				// First inform data about the value being removed
				// Inform the previous entry (if there is one) it no longer exists. Or make next the root entry
				if ( entry->prev != INVALID_MAP_INDEX )
					values[ entry->prev ].next = entry->next;
				else
					entries[ hash ] = entry->next;

				// Inform the next entry (if there is one) it no longer exists
				if ( entry->next != INVALID_MAP_INDEX )
					values[ entry->next ].prev = entry->prev;

				Entry *other = &values.top();

				// Don't swap with itself, it can just be popped
				if ( entry != other )
				{
					// Now inform data about the value that will get swapped to fill that removed gap
					// Update the previous' next
					if ( other->prev != INVALID_MAP_INDEX )
						values[ other->prev ].next = idx;
					else
						entries[ other->bucket ] = idx;

					// Inform the next' prev
					if ( other->next != INVALID_MAP_INDEX )
						values[ other->next ].prev = idx;

					// Update its own Idx
					other->idx = idx;
				}

				// Everything is ready, Make the swap & pop
				values.swap_and_remove( idx );

				return true;
			}

			idx = entry->next;
		}

		return false;
	}

	bool remove_keep_order( const KeyType &key )
	{
		u64 hash = KeyHash::create( key ) % Buckets;

		if ( hash >= entries.count )
			return false;

		u64 idx = entries[ hash ];

		while ( idx != INVALID_MAP_INDEX )
		{
			Entry *entry = &values[ idx ];

			if ( KeyCompare::compare( entry->key, key ) )
			{
				// First inform data about the value being removed
				// Inform the previous entry (if there is one) it no longer exists. Or make next the root entry
				if ( entry->prev != INVALID_MAP_INDEX )
					values[ entry->prev ].next = entry->next;
				else
					entries[ hash ] = entry->next;

				// Inform the next entry (if there is one) it no longer exists
				if ( entry->next != INVALID_MAP_INDEX )
					values[ entry->next ].prev = entry->prev;

				// Loop through all values after the one being removed and shuffle them down 1
				// Also update their prev->next and next->prev down 1 too
				for ( u64 i = idx + 1; idx < values.count; ++i )
				{
					Entry *e = &values[ i ];
					// If its prev is INVALID_MAP_INDEX, then its a root entry
					if ( e->prev != INVALID_MAP_INDEX )
						values[ e->prev ].next -= 1;
					else
						entries[ e->bucket ] -= 1;
					if ( e->next != INVALID_MAP_INDEX )
						values[ e->next ].prev -= 1;
					e->idx -= 1;
					values[ i - 1 ] = *e;
				}

				// Everythign is shuffled down, there is now 1 less entry
				values.count -= 1;

				return true;
			}

			idx = entry->next;
		}

		return false;
	}

	inline void clear()
	{
		values.clear();
		entries.clear();
	}

	[[nodiscard]] Value *get_value( const KeyType &key )
	{
		Entry *entry = find( key );
		return entry ? &entry->value : nullptr;
	}

	[[nodiscard]] Entry *find( const KeyType &key )
	{
		u64 hash = KeyHash::create( key ) % Buckets;

		if ( hash >= entries.count )
			return nullptr;

		u64 idx = entries[ hash ];

		while ( idx != INVALID_MAP_INDEX )
		{
			Entry *entry = &values[ idx ];
			if ( KeyCompare::compare( entry->key, key ) )
				return entry;
			idx = entry->next;
		}

		return nullptr;
	}

	[[nodiscard]] Entry *prev( const KeyType &key, bool allowWrap )
	{
		if ( empty() )
			return nullptr;

		Entry *entry = find( key );

		// This key doesn't even exist
		if ( !entry )
			return nullptr;

		// On first entry
		if ( entry->idx == 0 )
			return allowWrap ? &values[ values.count - 1 ] : nullptr;

		// Prev value
		return &values[ entry->idx - 1 ];
	}

	[[nodiscard]] Entry *next( const KeyType &key, bool allowWrap )
	{
		if ( empty() )
			return nullptr;

		Entry *entry = find( key );

		// This key doesn't even exist
		if ( !entry )
			return nullptr;

		// On last entry
		if ( entry->idx >= values.count - 1 )
			return allowWrap ? &values[ 0 ] : nullptr;

		// Next value
		return &values[ entry->idx + 1 ];
	}

	[[nodiscard]] inline Entry *operator[] ( const KeyType &key )
	{
		return find( key );
	}

	[[nodiscard]] inline const Entry *operator[] ( const KeyType &key ) const
	{
		return find( key );
	}

	[[nodiscard]] inline u64 count() const
	{
		return values.count;
	}

	[[nodiscard]] inline bool empty() const
	{
		return values.count == 0;
	}

	[[nodiscard]] inline bool full() const
	{
		return values.count == Capacity;
	}
};