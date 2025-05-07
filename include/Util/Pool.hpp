#pragma once
#include <Util/Defines.hpp>
#include <assert.h>

#include <stack>
#include <vector>

namespace Util {
    using std::stack;
    using std::vector;

    // Generational Pools and Handles inspired by this presentation https://advances.realtimerendering.com/s2023/AaltonenHypeHypeAdvances2023.pdf
    // Some implementation details taken from https://github.com/corporateshark/lightweightvk/blob/master/lvk/Pool.h
    template<typename Type> class Handle final {
    public:
        Handle() = default;

        bool Valid( void ) const { return mGen != 0; }

        uint Index( void ) const { return mIndex; }
    
        bool operator ==( const Handle<Type> & other ) const { return mIndex == other.mIndex && mGen == other.mGen; }
        bool operator !=( const Handle<Type> & other ) const { return mIndex != other.mIndex || mGen != other.mGen; }
    
    private:
        template<typename Type, typename HotType, typename ColdType> friend class Pool;
        Handle( uint idx, uint gen ) : mIndex( idx ), mGen( gen ) {}
        
        uint mIndex = 0;
        uint mGen   = 0;
    };
    static_assert( sizeof( Handle<bool> ) == sizeof( ulong ) );

    // Forward declarations with dummy structures
    using TextureHandle        = Handle<struct _Texture>;
    using SamplerHandle        = Handle<struct _Sampler>;
    using BufferHandle         = Handle<struct _Buffer>;
    using ShaderHandle         = Handle<struct _Shader>;
    using RenderPipelineHander = Handle<struct _RenderPipeline>;
    using ModelHandle          = Handle<struct _Model>;

    template<typename Type, typename HotType, typename ColdType> class Pool final {
    private:
        struct HotPoolEntry final {
            explicit HotPoolEntry( HotType & obj ) : mObj( std::move( obj ) ) {}
            explicit HotPoolEntry() : mObj( {} ), mGen( 1 ) {}
            HotType mObj = {};
            uint    mGen = 1;
        };
        struct ColdPoolEntry final {
            explicit ColdPoolEntry( ColdType & obj ) : mObj( std::move( obj ) ) {}
            explicit ColdPoolEntry() : mObj( {} ), mGen( 1 ) {}
            ColdType mObj = {};
            uint     mGen = 1;
        };

        stack<uint>           mFreeList;
        vector<HotPoolEntry>  mHotObjects;
        vector<ColdPoolEntry> mColdObjects;
        uint                  mEntries;

        void Resize( uint newEntries ) {
            assert( false && "TODO: Implement resizing resource pools!" );
        }

    public:
        explicit Pool( uint entries, const char * resourceType = "UNDEFINED" ) {
            assert( entries > 0 && entries < UINT32_MAX );
            mEntries = 0;

            mHotObjects.resize( entries );
            mColdObjects.resize( entries );

            // Populate the free list so that the lowest indices are used first
            for ( uint i = entries; i-- > 0; )
                mFreeList.push( i );
            printf("[INFO] Created %s Pool with %u entries!\n", resourceType, entries);
        }

        Handle<Type> Create( HotType && hot, ColdType && cold ) {
            if ( mFreeList.empty() )
                Resize( 0 );
            
            const uint idx = mFreeList.top();
            mFreeList.pop();
            ++mEntries;

            const uint gen = mHotObjects[idx].mGen;
            assert( gen == mColdObjects[idx].mGen );

            mHotObjects[idx].mObj = std::move( hot );
            mColdObjects[idx].mObj = std::move( cold );

            return Handle<Type>( idx, gen );
        }

        HotType * Get( Handle<Type> handle ) {
            if ( !handle.Valid() )
                return nullptr;
            const uint idx = handle.mIndex;
            assert( handle.mGen == mHotObjects[idx].mGen );
            return &mHotObjects[idx].mObj;
        }

        ColdType * GetMetadata( Handle<Type> handle ) {
            if ( !handle.Valid() )
                return nullptr;
            const uint idx = handle.mIndex;
            assert( handle.mGen == mColdObjects[idx].mGen );
            return &mColdObjects[idx].mObj;
        }

        Handle<Type> GetHandle( uint index ) {
            assert( mHotObjects.size() == mColdObjects.size() );
            if ( index >= mHotObjects.size() )
                return {};
            assert( mHotObjects[index].mGen == mColdObjects[index].mGen );
            return Handle<Type>( index, mHotObjects[index].mGen );
        }

        void Delete( Handle<Type> handle ) {
            if ( !handle.Valid() )
                return;
            const uint idx = handle.mIndex;

            assert( handle.mGen == mHotObjects[idx].mGen );
            assert( handle.mGen == mColdObjects[idx].mGen );

            mHotObjects[idx].mObj = HotType{};
            mHotObjects[idx].mGen++;

            mColdObjects[idx].mObj = ColdType{};
            mColdObjects[idx].mGen++;

            mFreeList.push( idx );
            --mEntries;
        }

        void Clear( void ) {
            mHotObjects.clear();
            mColdObjects.clear();
            while ( !mFreeList.empty() )
                mFreeList.pop();
            mEntries = 0;
        }

        uint GetEntryCount( void ) const { return mEntries; }
        size_t GetObjectCount( void ) const {
            assert( mHotObjects.size() == mColdObjects.size() );
            return mHotObjects.size();
        }
        
    };
}
