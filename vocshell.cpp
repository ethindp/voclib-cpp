#include "voclib.hpp"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <exception>
#include <iostream>
using namespace std;
int write_wave_file ( const char* filename, unsigned int sample_rate, unsigned int channels, float* buffer, drwav_uint64 frames, float gain )
{
    drwav_uint64 i;
    drwav_int16* converted;
    drwav* outfile;
    drwav_data_format format;
    format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
    format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
    format.channels = channels;
    format.sampleRate = sample_rate;
    format.bitsPerSample = 16;

    for ( i = 0; i < frames * channels; ++i )
    {
        buffer[i] *= gain;
        if ( buffer[i] < -1.0f )
        {
            buffer[i] = -1.0f;
        }
        else if ( buffer[i] > 1.0f )
        {
            buffer[i] = 1.0f;
        }
    }

    converted = ( drwav_int16* ) malloc ( ( frames * channels ) * sizeof ( drwav_int16 ) );
    if ( converted == NULL )
    {
        return 0;
    }
    drwav_f32_to_s16 ( converted, buffer, frames * channels );

    outfile = drwav_open_file_write_sequential ( filename, &format, frames * channels );
    if ( outfile == NULL )
    {
        return 0;
    }

    if ( drwav_write ( outfile, frames * channels, &converted[0] ) != frames * channels )
    {
        drwav_close ( outfile );
        return 0;
    }

    drwav_close ( outfile );

    return 1;
}

int main ( int argc, const char** argv )
{
    unsigned long bands = 24;
    unsigned long filters_per_band = 4;
    double reaction_time = 0.03;
    double formant_shift = 1.0;
    drwav_uint64 carrier_frames, modulator_frames;
    unsigned int carrier_sample_rate, carrier_channels, modulator_sample_rate, modulator_channels;
    float* carrier_buffer;
    float* modulator_buffer;
    const char* carrier_filename = NULL;
    const char* modulator_filename = NULL;
    const char* output_filename = NULL;
    int i;

    if ( argc == 1 )
    {
        cout << "Usage:" << endl << "Mandatory parameters:" << endl << "-c carrier file" << endl << "-m modulator file" << endl << "-o output file" << endl << "Optional parameters:" << endl << "-b number of bands" << endl << "-f filters per band" << endl << "-r reaction time (in seconds)" << endl << "-s formant shift (in octaves)." << endl;
        return 0;
    }

    for ( i = 1; i < argc; ++i )
    {
        if ( strcmp ( argv[i], "-c" ) == 0 && i < ( argc - 1 ) )
        {
            ++i;
            carrier_filename = argv[i];
            continue;
        }
        if ( strcmp ( argv[i], "-m" ) == 0 && i < ( argc - 1 ) )
        {
            ++i;
            modulator_filename = argv[i];
            continue;
        }
        if ( strcmp ( argv[i], "-o" ) == 0 && i < ( argc - 1 ) )
        {
            ++i;
            output_filename = argv[i];
            continue;
        }
        if ( strcmp ( argv[i], "-b" ) == 0 && i < ( argc - 1 ) )
        {
            ++i;
            bands = strtoul ( argv[i], NULL, 10 );
            if ( bands < 4 || bands > VOCLIB_MAX_BANDS )
            {
                cerr << "Error: The bands parameter is out of range." << endl << "The allowed range is between 4 and " << VOCLIB_MAX_BANDS << " (inclusive)." << endl;
                return 1;
            }
            continue;
        }
        if ( strcmp ( argv[i], "-f" ) == 0 && i < ( argc - 1 ) )
        {
            ++i;
            filters_per_band = strtoul ( argv[i], NULL, 10 );
            if ( filters_per_band < 1 || filters_per_band > VOCLIB_MAX_FILTERS_PER_BAND )
            {
                cerr << "Error: The filters per band parameter is out of range." << endl << "The allowed range is between 1 and " << VOCLIB_MAX_FILTERS_PER_BAND << " (inclusive)." << endl;
                return 1;
            }
            continue;
        }

        if ( strcmp ( argv[i], "-r" ) == 0 && i < ( argc - 1 ) )
        {
            ++i;
            reaction_time = strtod ( argv[i], NULL );
            if ( reaction_time < 0.002 || reaction_time > 2.0 )
            {
                cerr << "Error: The reaction time parameter is out of range." << endl << "The allowed range is between 0.002 and 2.0 (inclusive)." << endl;
                return 1;
            }
            continue;
        }

        if ( strcmp ( argv[i], "-s" ) == 0 && i < ( argc - 1 ) )
        {
            ++i;
            formant_shift = strtod ( argv[i], NULL );
            if ( formant_shift < 0.25 || formant_shift > 4.0 )
            {
                cerr << "Error: The formant shift parameter is out of range." << endl << "The allowed range is between 0.25 and 4.0 (inclusive)." << endl;
                return 1;
            }
            continue;
        }

    }

    if ( carrier_filename == NULL )
    {
        cerr << "Error: Missing carrier file (use the -c option followed by a filename)." << endl;
        return 1;
    }
    if ( modulator_filename == NULL )
    {
        cerr << "Error: Missing modulator file (use the -m option followed by a filename)." << endl;
        return 1;
    }
    if ( output_filename == NULL )
    {
        cerr << "Error: Missing output file (use the -o option followed by a filename)." << endl;
        return 1;
    }

    carrier_buffer = drwav_open_file_and_read_pcm_frames_f32 ( carrier_filename, &carrier_channels, &carrier_sample_rate, &carrier_frames );
    if ( carrier_buffer == NULL )
    {
        cerr << "Error: Could not read carrier file \"" << carrier_filename << "\"." << endl;
        return 1;
    }

    modulator_buffer = drwav_open_file_and_read_pcm_frames_f32 ( modulator_filename, &modulator_channels, &modulator_sample_rate, &modulator_frames );
    if ( modulator_buffer == NULL )
    {
        free ( carrier_buffer );
        cerr << "Error: Could not read modulator file \"" << modulator_filename << "\"." << endl;
        return 1;
    }

    if ( carrier_channels > 1 )
    {
        free ( carrier_buffer );
        free ( modulator_buffer );
        cerr << "Error: The carrier has " << carrier_channels << " channels." << endl << "The maximum allowed number of channels is 1." << endl << "This restriction may be lifted in a future version." << endl;
        return 1;
    }
    if ( modulator_channels > 1 )
    {
        free ( modulator_buffer );
        free ( modulator_buffer );
        cerr << "Error: The modulator has " << modulator_channels << " channels." << endl << "The maximum allowed number of channels is 1." << endl << "This restriction may be lifted in a future version." << endl;
        return 1;
    }

    if ( modulator_sample_rate != carrier_sample_rate )
    {
        free ( carrier_buffer );
        free ( modulator_buffer );
        cerr << "Error: The carrier has a sample rate of " << carrier_sample_rate << " and the modulator has " << modulator_sample_rate << "." << endl << "The carrier and modulator currently need to have the same sample rate." << endl << "This restriction may be lifted in a future version." << endl;
        return 1;
    }

    if ( modulator_frames > carrier_frames )
    {
        cout << "Info: The modulator is longer than the carrier." << endl << "The output will only be as long as the carrier." << endl;
        modulator_frames = carrier_frames;
    }
    else if ( carrier_frames > modulator_frames )
    {
        cerr << "Info: The carrier is longer than the modulator." << endl << "The output will only be as long as the modulator." << endl;
        carrier_frames = modulator_frames;
    }

    assert ( modulator_frames == carrier_frames );
    assert ( modulator_channels == carrier_channels );
    assert ( modulator_sample_rate == carrier_sample_rate );

    Vocoder vocoder(( unsigned char ) bands, ( unsigned char ) filters_per_band, carrier_sample_rate);
    vocoder.SetReactionTime(( float ) reaction_time);
    vocoder.SetFormantShift(( float ) formant_shift);

    auto output_buffer = vocoder.Process(carrier_buffer, modulator_buffer, carrier_frames);

    free ( carrier_buffer );
    free ( modulator_buffer );

    if ( !write_wave_file ( output_filename, carrier_sample_rate, carrier_channels, output_buffer.data(), carrier_frames, 12.0f ) )
    {
        output_buffer.clear();
        cerr << "Error: Could not write output file \"" << output_filename << "\"." << endl;
        return 1;
    }

    output_buffer .clear();
    cout << "Success." << endl;
    return 0;
}
