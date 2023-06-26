#ifndef __AMF_H__
#define __AMF_H__

#include <vector>

// refer to: https://rtmp.veriskope.com/pdf/amf0-file-format-specification.pdf

namespace nx {

enum AMFType {

    Number = 0,
    Boolean,
    String,
    Object,
    MovieClip, // reserved, not supported
    Null,
    Undefined,
    Reference,
    ECMAArray,
    ObjEnd = 9,
    StrictArray,
    Date,
    LongString
};

using AMF_BUFFER = std::vector<uint8_t>;

void amf_put_double( double value, AMF_BUFFER &buf );
void amf_put_bool( bool value, AMF_BUFFER &buf );
void amf_put_string( const char *str, AMF_BUFFER &buf );
void amf_put_obj_end( AMF_BUFFER &buf );

void amf_put_named_double( const char *name, double value, AMF_BUFFER &buf );
void amf_put_named_bool( const char *name, bool value, AMF_BUFFER &buf );
void amf_put_named_string( const char *name, const char *value, AMF_BUFFER &buf );
/**
 * @brief put amf object in buf
 *
 * @param name amf object name
 * @param obj obj buffer, include objce_end 009
 * @param buf dst buf
 */
void amf_put_named_object( const char *name, const AMF_BUFFER &obj, AMF_BUFFER &buf );
/**
 * @brief put ecma array in buf
 *
 * @param name ecma array name
 * @param length  ecma array length
 * @param content  ecma array content buf, without objce_end 009
 * @param buf dst buf, include objce_end 009
 */
void amf_put_named_ecma_array( const char *name, uint32_t length, const AMF_BUFFER &content, AMF_BUFFER &buf );

};     // namespace nx

#endif // __AMF_H__