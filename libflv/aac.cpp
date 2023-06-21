#include "aac.h"
#include "get_bits.h"
#include "put_bits.h"

namespace nx {

adts_header::adts_header( Profile profile, uint32_t sample_rate, uint8_t channelCount, int size ) {
    /*
    refer to https://www.igac.gov.co/sites/all/libraries/ffmpeg/libavformat/adtsenc.c
    static int adts_write_frame_header(AVFormatContext *s, int size);
    */
    fixed_header.syncword              = 0xFFF;
    fixed_header.id                    = 1; // MPEG-4
    fixed_header.layer                 = 0;
    fixed_header.protection_absent     = 1;
    fixed_header.profile               = profile;
    fixed_header.channel_configuration = sample_rate_index( sample_rate );
    fixed_header.private_bit           = 0;
    fixed_header.channel_configuration = channelCount;
    fixed_header.original_copy         = 0;
    fixed_header.home                  = 0;

    variable_header.copyright_identification_bit   = 0;
    variable_header.copyright_identification_start = 0;

    const int ADTS_HEADER_SIZE                         = 7;
    variable_header.aac_frame_length                   = ADTS_HEADER_SIZE + size;
    variable_header.adts_buffer_fullness               = 0x7FF;
    variable_header.number_of_raw_data_blocks_in_frame = 0;
}

uint8_t adts_header::sample_rate_index( uint32_t sample_rate ) {
    uint32_t sample_rates[] = {
        96000, // 0x0
        88200, // 0x1
        64000, // 0x2
        48000, // 0x3
        44100, // 0x4
        32000, // 0x5
        24000, // 0x6
        22050, // 0x7
        16000, // 0x8
        12000, // 0x9
        11025, // 0xa
        8000   // 0xb
    };
    uint32_t count = sizeof( sample_rates ) / sizeof( uint32_t );
    for ( int i = 0; i < count; i++ ) {
        if ( sample_rates[i] == sample_rate ) {
            return i;
        }
    }
    // TODO: handle error
    return 0;
}

void adts_header::adts_header_to_buf( const adts_header &header, uint8_t buf[7] ) {
    PutBitsContext context = PutBitsContext( buf, 7 );
    // fixed header
    context.put_bits( 12, header.fixed_header.syncword );
    context.put_bits( 1, header.fixed_header.id );
    context.put_bits( 2, header.fixed_header.layer );
    context.put_bits( 1, header.fixed_header.protection_absent );
    context.put_bits( 2, header.fixed_header.profile );
    context.put_bits( 4, header.fixed_header.sampling_frequency_index );
    context.put_bits( 1, header.fixed_header.private_bit );
    context.put_bits( 3, header.fixed_header.channel_configuration );
    context.put_bits( 1, header.fixed_header.original_copy );
    context.put_bits( 1, header.fixed_header.home );
    // variable header
    context.put_bits( 1, header.variable_header.copyright_identification_bit );
    context.put_bits( 1, header.variable_header.copyright_identification_start );
    context.put_bits( 13, header.variable_header.aac_frame_length );
    context.put_bits( 11, header.variable_header.adts_buffer_fullness );
    context.put_bits( 2, header.variable_header.number_of_raw_data_blocks_in_frame );
}

adts_header adts_header::parse_adts_header( const uint8_t buf[7] ) {

    adts_header   header;
    GetBitContext context = GetBitContext( buf, 7 );
    // fixed header
    header.fixed_header.syncword                 = context.get_bits( 12 ); // 12bits
    header.fixed_header.id                       = context.get_bit1();     // 1bit
    header.fixed_header.layer                    = context.get_bits( 2 );  // 2bits
    header.fixed_header.protection_absent        = context.get_bit1();     // 1bit
    header.fixed_header.profile                  = context.get_bits( 2 );  // 2bits
    header.fixed_header.sampling_frequency_index = context.get_bits( 4 );  // 4bits
    header.fixed_header.private_bit              = context.get_bit1();     // 1bits
    header.fixed_header.channel_configuration    = context.get_bits( 3 );  // 3bit
    header.fixed_header.original_copy            = context.get_bit1();     // 1bit
    header.fixed_header.home                     = context.get_bit1();     // 1bit
    // variable header
    header.variable_header.copyright_identification_bit       = context.get_bit1();     // 1bit
    header.variable_header.copyright_identification_start     = context.get_bit1();     // 1bit
    header.variable_header.aac_frame_length                   = context.get_bits( 13 ); // 13bits
    header.variable_header.adts_buffer_fullness               = context.get_bits( 11 ); // 11bits
    header.variable_header.number_of_raw_data_blocks_in_frame = context.get_bits( 2 );  // 11bits
    return header;
}

AudioSpecificConfig::AudioSpecificConfig( uint8_t adts_header_buf[7] ) {
    adts_header header           = adts_header::parse_adts_header( adts_header_buf );
    this->audioObjectType        = header.fixed_header.profile + 1;
    this->samplingFrequencyIndex = header.fixed_header.sampling_frequency_index;
    this->channelConfiguration   = header.fixed_header.channel_configuration;
    this->frameLengthFlag        = 0;
    this->dependsOnCoreCoder     = 0;
    this->extensionFlag          = 0;
}

void AudioSpecificConfig::to_buf( uint8_t buf[2] ) {
    PutBitsContext context = PutBitsContext( buf, 2 );
    context.put_bits( 5, audioObjectType );
    context.put_bits( 4, samplingFrequencyIndex );
    context.put_bits( 4, channelConfiguration );
    context.put_bits( 1, frameLengthFlag );
    context.put_bits( 1, dependsOnCoreCoder );
    context.put_bits( 1, extensionFlag );
}

}; // namespace nx