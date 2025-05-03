#include <Resource/Resource.hpp>

namespace Resource {
    using namespace std;

    Shader LoadShader( const string & filename ) {
        ifstream ifs( filename, ios::binary | ios::ate );
        assert( ifs );
        
        Shader result;

        const auto end = ifs.tellg();
        ifs.seekg( 0, ios::beg );
        result.size = static_cast<ulong>( end - ifs.tellg() );

        result.byteCode = new uint[result.size / 4];
        assert( result.byteCode );

        assert( ifs.read( reinterpret_cast<char *>( result.byteCode ), result.size ).good() );
        return result;
    }

    Image LoadTexture( const string & filename ) {
        Image result;
        int width, height, comp;
        result.data = stbi_load( filename.c_str(), &width, &height, &comp, 4 );
        
        assert( result.data );

        result.width = static_cast<ushort>( width );
        result.height = static_cast<ushort>( height );
        result.components = static_cast<_byte>( comp );

        return result;
    }

}
