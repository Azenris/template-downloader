
#pragma once

#define MEMORY_ALIGNMENT		( sizeof( u64 ) )

using MemoryFlags = u32;
enum MEMORY_FLAGS : MemoryFlags
{
	MEMORY_FLAGS_INITIALISED			= 1 << 0,
	MEMORY_FLAGS_SEPARATE_ALLOCATIONS	= 1 << 1,
};

struct MemoryHeader
{
	u8 *prev;				// last block before this one
	u64 reqSize;			// amount allocated
	u64 size;				// size requested
	u8 *attachedTo;			// when free's it will rewind to this allocation
	u16 alignment;			// alignment of allocation
	u16 padding;			// padding used to gain the alignment
};

static_assert( sizeof( MemoryHeader ) % MEMORY_ALIGNMENT == 0 );

struct Allocator
{
	u64 capacity;
	u64 available;
	u8 *memory;
	u8 *lastAlloc;

	u8 *( *allocate_func )( Allocator *allocator, u64 size, bool clearZero, u16 alignment );
	u8 *( *reallocate_func )( Allocator *allocator, void *p, u64 size );
	void ( *shrink_func )( Allocator *allocator, void *p, u64 size );
	void ( *free_func )( Allocator *allocator, void *p );
	void ( *attach_func )( Allocator *allocator, void *p, void *to );

	// METHODS ////////////////////////////////////
	template <typename T> [[nodiscard]] inline T *allocate( bool clearZero = false );
	template <typename T> [[nodiscard]] inline T *allocate( u32 size, bool clearZero = false );
	template <typename T> [[nodiscard]] inline T *allocate( u32 size, bool clearZero, u16 alignment );
	template <typename T> [[nodiscard]] inline T *allocate( u64 size, bool clearZero = false );
	template <typename T> [[nodiscard]] inline T *allocate( u64 size, bool clearZero, u16 alignment );
	template <typename T> [[nodiscard]] inline T *reallocate( void *p, u64 size );
	inline void shrink( void *p, u64 size );
	inline void free( void *p );
	inline void attach( void *p, void *to );
};

struct MemoryArena
{
	bool init( u64 permanentSize, u64 transientSize, u64 fastBumpSize, bool clearZero = false, u16 alignment = MEMORY_ALIGNMENT );
	void free();
	void update();

	MemoryFlags flags = 0;
	u8 *memory = nullptr;
	Allocator permanent = {};
	Allocator transient = {};
	Allocator fastBump = {};
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
[[nodiscard]] inline T *Allocator::allocate( bool clearZero )
{
	return reinterpret_cast<T*>( allocate_func( this, sizeof( T ), clearZero, alignof( T ) ) );
}

template <typename T>
[[nodiscard]] inline T *Allocator::allocate( u32 size, bool clearZero )
{
	return reinterpret_cast<T*>( allocate_func( this, size * sizeof( T ), clearZero, alignof( T ) ) );
}

template <typename T>
[[nodiscard]] inline T *Allocator::allocate( u32 size, bool clearZero, u16 alignment )
{
	return reinterpret_cast<T*>( allocate_func( this, size * sizeof( T ), clearZero, alignment ) );
}

template <typename T>
[[nodiscard]] inline T *Allocator::allocate( u64 size, bool clearZero )
{
	return reinterpret_cast<T*>( allocate_func( this, size * sizeof( T ), clearZero, alignof( T ) ) );
}

template <typename T>
[[nodiscard]] inline T *Allocator::allocate( u64 size, bool clearZero, u16 alignment )
{
	return reinterpret_cast<T*>( allocate_func( this, size * sizeof( T ), clearZero, alignment ) );
}

template <typename T>
[[nodiscard]] inline T *Allocator::reallocate( void *p, u64 size )
{
	return reinterpret_cast<T*>( reallocate_func( this, p, size ) );
}

inline void Allocator::shrink( void *p, u64 size )
{
	return shrink_func( this, p, size );
}

inline void Allocator::free( void *p )
{
	return free_func( this, p );
}

inline void Allocator::attach( void *p, void *to )
{
	return attach_func( this, p, to );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool MemoryArena::init( u64 permanentSize, u64 transientSize, u64 fastBumpSize, bool clearZero, u16 alignment )
{
	constexpr const u64 permanentMinSize = sizeof( Allocator ) + sizeof( MemoryHeader );
	constexpr const u64 transientMinSize = sizeof( Allocator ) + sizeof( MemoryHeader );
	constexpr const u64 fastBumpMinSize = sizeof( Allocator );
	if ( permanentSize < permanentMinSize ) permanentSize = permanentMinSize;
	if ( transientSize < transientMinSize ) transientSize = transientMinSize;
	if ( fastBumpSize < fastBumpMinSize ) fastBumpSize = fastBumpMinSize;

	if ( flags & MEMORY_FLAGS_INITIALISED )
		free();

	u64 permanentReqSize = permanentSize + ( alignment - ( permanentSize & ( alignment - 1 ) ) );
	u64 transientReqSize = transientSize + ( alignment - ( transientSize & ( alignment - 1 ) ) );
	u64 fastBumpReqSize = fastBumpSize + ( alignment - ( fastBumpSize & ( alignment - 1 ) ) );
	u64 reqSize = permanentReqSize + transientReqSize + fastBumpReqSize;
	u8 *permanentMemory = nullptr;
	u8 *transientMemory = nullptr;
	u8 *fastBumpMemory = nullptr;

	memory = (u8 *)malloc( reqSize );

	// If the memory allocation fails, attempt to allocate
	// seperately for the memory blocks
	if ( !memory )
	{
		permanentMemory = (u8 *)malloc( permanentReqSize );
		transientMemory = (u8 *)malloc( transientReqSize );
		fastBumpMemory = (u8 *)malloc( fastBumpReqSize );
		memory = permanentMemory;
		flags |= MEMORY_FLAGS_SEPARATE_ALLOCATIONS;
	}
	else
	{
		permanentMemory = memory;
		transientMemory = permanentMemory + permanentReqSize;
		fastBumpMemory = transientMemory + transientReqSize;
		flags &= ~MEMORY_FLAGS_SEPARATE_ALLOCATIONS;
	}

	if ( !permanentMemory || !transientMemory || !fastBumpMemory )
	{
		return false;
	}

	if ( clearZero )
	{
		memset( permanentMemory, 0, permanentReqSize );
		memset( transientMemory, 0, transientReqSize );
		memset( fastBumpMemory, 0, fastBumpReqSize );
	}

	permanent.capacity = permanentSize;
	permanent.available = permanentSize;
	permanent.memory = permanentMemory;
	permanent.lastAlloc = nullptr;

	transient.capacity = transientSize;
	transient.available = transientSize;
	transient.memory = transientMemory;
	transient.lastAlloc = nullptr;

	fastBump.capacity = fastBumpSize;
	fastBump.available = fastBumpSize;
	fastBump.memory = fastBumpMemory;
	fastBump.lastAlloc = nullptr;

	flags |= MEMORY_FLAGS_INITIALISED;

	return true;
}

void MemoryArena::free()
{
	if ( flags & MEMORY_FLAGS_INITIALISED )
	{
		// Check if it was a single allocation or 2 seperate ones
		if ( flags & MEMORY_FLAGS_SEPARATE_ALLOCATIONS )
		{
			::free( permanent.memory );
			::free( transient.memory );
			::free( fastBump.memory );
		}
		else
		{
			::free( memory );
		}

		memset( this, 0, sizeof( *this ) );
	}
}

void MemoryArena::update()
{
	transient.available = transient.capacity;
	transient.lastAlloc = nullptr;

	fastBump.available = fastBump.capacity;
	fastBump.lastAlloc = nullptr;
}

// BUMP ALLOCATOR ////////////////////////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] u8 *memory_bump_allocate( Allocator *allocator, u64 size, bool clearZero, u16 alignment )
{
	assert( size );

	u8 *p = allocator->memory + ( allocator->capacity - allocator->available ) + sizeof( MemoryHeader );
	u64 padding = alignment - ( reinterpret_cast<u64>( p ) & ( alignment - 1 ) );

	// P now points to the data
	p += padding;

	// Total size that needs allocating
	u64 reqSize = padding + sizeof( MemoryHeader ) + size;

	if ( reqSize > allocator->available )
	{
		return nullptr;
	}

	MemoryHeader *header = reinterpret_cast<MemoryHeader *>( p - sizeof( MemoryHeader ) );
	header->prev = allocator->lastAlloc;
	header->reqSize = reqSize;
	header->size = size;
	header->attachedTo = nullptr;
	header->alignment = alignment;
	header->padding = static_cast<u16>( padding );

	allocator->available -= reqSize;
	allocator->lastAlloc = p;

	if ( clearZero )
		memset( allocator->lastAlloc, 0, size );

	return allocator->lastAlloc;
}

[[nodiscard]] u8 *memory_bump_reallocate( Allocator *allocator, void *p, u64 size )
{
	if ( !p )
		return allocator->allocate<u8>( size );

	assert( size );

	MemoryHeader *header = reinterpret_cast<MemoryHeader*>( static_cast<u8 *>( p ) - sizeof( MemoryHeader ) );

	// Same size
	if ( size == header->size )
		return static_cast<u8 *>( p );

	u64 oldSize = header->size;
	u64 oldReqSize = header->reqSize;

	// See if it was the last used allocation
	if ( allocator->lastAlloc == p )
	{
		u64 reqSize = header->padding + sizeof( MemoryHeader ) + size;

		// Either shrinking or same required memory
		if ( size < oldSize || reqSize == oldReqSize )
		{
			header->reqSize = reqSize;
			header->size = size;
			allocator->available += ( oldReqSize - reqSize );
			return static_cast<u8 *>( p );
		}

		u64 extraReqSizeNeeded = ( reqSize - oldReqSize );

		if ( extraReqSizeNeeded > allocator->available )
		{
			return nullptr;
		}

		header->reqSize = reqSize;
		header->size = size;

		// Remove the extra space required for this reallocation
		allocator->available -= extraReqSizeNeeded;

		return static_cast<u8 *>( p );
	}

	// Since it wasn't the last allocation, allocate a new block and copy the data over
	u8 *newMemory = allocator->allocate<u8>( size, false, header->alignment );

	if ( !newMemory )
	{
		return static_cast<u8 *>( p );
	}

	memcpy( newMemory, p, size < oldSize ? size : oldSize );

	allocator->free( p );

	return newMemory;
}

void memory_bump_shrink( Allocator *allocator, void *p, u64 size )
{
	assert( p );
	assert( size );

	MemoryHeader *header = reinterpret_cast<MemoryHeader*>( static_cast<u8 *>( p ) - sizeof( MemoryHeader ) );

	// Same size
	if ( size == header->size )
		return;

	u64 oldSize = header->size;
	u64 oldReqSize = header->reqSize;
	u64 reqSize = header->padding + sizeof( MemoryHeader ) + size;

	if ( size > oldSize )
	{
		return;
	}

	// If it was the last allocation, the required size can be shrunk too
	if ( allocator->lastAlloc == p )
	{
		// Give the memory back, and update the new required size
		header->reqSize = reqSize;
		allocator->available += ( oldReqSize - reqSize );
	}

	// Update the size of this block
	header->size = size;
}

void memory_bump_free( Allocator *allocator, void *p )
{
	if ( !p || allocator->lastAlloc != p )
		return;

	MemoryHeader *header = reinterpret_cast<MemoryHeader*>( static_cast<u8 *>( p ) - sizeof( MemoryHeader ) );
	if ( !header->attachedTo )
	{
		u64 reqSize = header->reqSize;
		allocator->available += reqSize;
		allocator->lastAlloc = header->prev;
		return;
	}

	// It requires rewinding until a certain allocation
	u8 *until = header->attachedTo;
	while ( p && until && p >= until )
	{
		header = reinterpret_cast<MemoryHeader*>( static_cast<u8 *>( p ) - sizeof( MemoryHeader ) );
		u64 reqSize = header->reqSize;
		allocator->available += reqSize;
		allocator->lastAlloc = header->prev;
		if ( header->attachedTo )
			until = ( until <= header->attachedTo ? until : header->attachedTo );
		p = header->prev;
	}
}

void memory_bump_attach( Allocator *allocator, void *p, void *to )
{
	if ( p && to && to < p )
	{
		MemoryHeader *header = reinterpret_cast<MemoryHeader*>( static_cast<u8 *>( p ) - sizeof( MemoryHeader ) );
		header->attachedTo = static_cast<u8 *>( to );
	}
}

// FAST BUMP ALLOCATOR ///////////////////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] u8 *memory_fast_bump_allocate( Allocator *allocator, u64 size, bool clearZero, u16 alignment )
{
	assert( size );

	u8 *p = allocator->memory + ( allocator->capacity - allocator->available );
	u64 padding = alignment - ( reinterpret_cast<u64>( p ) & ( alignment - 1 ) );

	// P now points to the data
	p += padding;

	// Total size that needs allocating
	u64 reqSize = padding + size;

	if ( reqSize > allocator->available )
	{
		return nullptr;
	}

	allocator->available -= reqSize;
	allocator->lastAlloc = p;

	if ( clearZero )
		memset( allocator->lastAlloc, 0, size );

	return allocator->lastAlloc;
}
