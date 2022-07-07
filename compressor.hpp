/*
//  Copyright (c) 2020 Peter Frane Jr. All Rights Reserved.
//
//  Use of this source code is governed by the BSD 3-Clause License that can be
//  found in the LICENSE file.
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
//  OF ANY KIND, either express or implied.
//
//  For inquiries, email the author at pfranejr AT hotmail.com
*/

#pragma once
#include "types.h"
#include <zlib.h>

#pragma comment(lib, "zdll.lib")

class stream_compressor
{
public:
    stream_compressor() = default;
    ~stream_compressor() = default;
    
	bool compress(byte_vector& dest_buffer, const byte_t* source_data, size_t source_length, int compression_level)
	{
        uLongf dest_length = 0;
        uint8_t* compressed_data;
        int ret;

        try
        {
            // allocate the buffer; make it at least equal to source length
            dest_buffer.resize(source_length);

            //dest_length = 0;
            dest_length = (uLongf)dest_buffer.size();

            compressed_data = dest_buffer.data();
        }
        catch (...)
        {
            return false;
        }

        // compress
        ret = compress2(compressed_data, &dest_length, source_data, (uLong)source_length, compression_level);

        if (Z_OK == ret)
        {
            // resize to the size of the contents
            dest_buffer.resize(dest_length);

            return true;
        }
        else
        {
            // most likely the source_data are too small to compress, and the resulting compressed data will be bigger than the source_data
            return false;
        }
	}
    
    
};