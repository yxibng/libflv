#include "amf.h"

namespace nx {
void amf_put_double( double value, AMF_BUFFER &buf ) {
    uint8_t type = AMFType::Number;
    buf.push_back( type );
    // to big endian double
    uint8_t *p = (uint8_t *)&value;
    for ( int i = 0; i < 8; i++ ) {
        buf.push_back( p[8 - i - 1] );
    }
}
void amf_put_bool( bool value, AMF_BUFFER &buf ) {
    uint8_t type = AMFType::Boolean;
    buf.push_back( type );
    uint8_t val = (uint8_t)value;
    buf.push_back( val );
}

void amf_put_string( const char *str, AMF_BUFFER &buf ) {
    // type
    uint8_t type = strlen( str ) > UINT16_MAX ? AMFType::LongString : AMFType::String;
    buf.push_back( type );
    // StringData length in bytes.
    if ( strlen( str ) > UINT16_MAX ) {
        // long string, length is UI32, big endian
        uint32_t length = strlen( str );
        uint8_t *p      = (uint8_t *)&length;
        for ( int i = 0; i < 4; i++ ) {
            buf.push_back( p[4 - i - 1] );
        }
    }
    else {
        // short string, length is UI16, big endian
        uint16_t length = strlen( str );
        uint8_t *p      = (uint8_t *)&length;
        for ( int i = 0; i < 2; i++ ) {
            buf.push_back( p[2 - i - 1] );
        }
    }
    // data
    uint8_t *p = (uint8_t *)str;
    for ( uint32_t i = 0; i < strlen( str ); i++ ) { // FIXME: low performance when deal with long string.
        buf.push_back( *( p + i ) );
    }
}
void amf_put_obj_end( AMF_BUFFER &buf ) {
    // 0, 0, 9
    buf.push_back( 0 );
    buf.push_back( 0 );
    buf.push_back( AMFType::ObjEnd );
}

void amf_put_named_double( const char *name, double value, AMF_BUFFER &buf ) {
    amf_put_string( name, buf );
    amf_put_double( value, buf );
}
void amf_put_named_bool( const char *name, bool value, AMF_BUFFER &buf ) {
    amf_put_string( name, buf );
    amf_put_bool( value, buf );
}
void amf_put_named_string( const char *name, const char *value, AMF_BUFFER &buf ) {
    amf_put_string( name, buf );
    amf_put_string( value, buf );
}

void amf_put_named_object( const char *name, const AMF_BUFFER &obj, AMF_BUFFER &buf ) {
    amf_put_string( name, buf );
    buf.insert( buf.end(), obj.begin(), obj.end() );
}
void amf_put_named_ecma_array( const char *name, uint32_t length, const AMF_BUFFER &content, AMF_BUFFER &buf ) {
    // name
    amf_put_string( name, buf );
    // big endian length
    uint8_t *p = (uint8_t *)&length;
    for ( int i = 0; i < 4; i++ ) {
        buf.push_back( p[4 - i - 1] );
    }
    // content
    buf.insert( buf.end(), content.begin(), content.end() );
    // obj end
    amf_put_obj_end( buf );
}
} // namespace nx