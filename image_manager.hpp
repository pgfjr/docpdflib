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
#include "compressor.hpp"

class image_manager
{
	std::map<std::string, int32_t> m_table;
private:
	void copy_image(byte_vector& dest_buffer, unsigned width, unsigned height, unsigned stride, Gdiplus::PixelFormat format, const byte_t* source_data)
	{
		byte_vector tmp;
		stream_compressor compressor;
		unsigned row_width;
		byte_t* src, * dest;


		src = (byte_t*)source_data;

		row_width = width * 3;

		tmp.resize(row_width * height);

		dest = tmp.data();

		for (unsigned i = 0; i < height; ++i)
		{
			// convert from bgr to rgb
			for (unsigned j = 0; j < row_width; j += 3)
			{
				const byte_t* p = src + j;

				dest[0] = p[2];
				dest[1] = p[1];
				dest[2] = p[0];
				dest += 3;
			}

			src += stride;
		}

		if (!compressor.compress(dest_buffer, tmp.data(), tmp.size(), 9))
		{
			// copy as-is if compression failed
			dest_buffer = tmp;
		}
	}
public:
	image_manager() : m_table()
	{
	}
	~image_manager()
	{
		clear();
	}
	void clear()
	{
		m_table.clear();
	}
	int32_t find_image(const char* filename)
	{
		auto it = m_table.find(std::string(filename));

		if (it == m_table.end())
		{
			return NOTFOUND;
		}

		return it->second;
	}
	bool add_image(const char* filename, int32_t object_number, FILE *fp)
	{
		size_t len = strlen(filename);
		std::vector<wchar_t> wfilename(len*2+1, 0);
		

		if (MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, filename, (int)len, wfilename.data(), (int)wfilename.size()) == 0)
		{
			return false;
		}
		else
		{
			Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(wfilename.data());

			if (!bmp)
			{
				return false;
			}
			else
			{
				bool result = true;

				Gdiplus::BitmapData data;
				Gdiplus::PixelFormat format;
				unsigned width, height, stride;
				Gdiplus::Rect rect;
				byte_vector dest_buffer;
				short bits_per_component;

				format = bmp->GetPixelFormat();			
				width = bmp->GetWidth();
				height = bmp->GetHeight();
				
				rect.Width = (int)width;
				rect.Height = (int)height;


				bmp->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &data);

				stride = abs(data.Stride);

				bits_per_component = 8;

				copy_image(dest_buffer, width, height, stride, PixelFormat24bppRGB, (byte_t*)data.Scan0);

				result = write_image_data(dest_buffer, fp, width, height, bits_per_component);

				bmp->UnlockBits(&data);

				delete bmp;

				if (result)
				{
					m_table[std::string(filename)] = object_number;
				}
				return result;
			}
		}
	}	
	
	bool write_image_data(byte_vector dest_buffer, FILE* fp, unsigned width, unsigned height, short bits_per_component)
	{
		unsigned length = (unsigned)dest_buffer.size();

		std::fprintf(fp, "<</Type /XObject\n/Subtype /Image\n/Width %u\n/Height %u\n", width, height);
			
		std::fprintf(fp, "/ColorSpace /DeviceRGB\n/BitsPerComponent %d\n/Filter /FlateDecode\n/Length %u\n>>\nstream\n", bits_per_component, length);

		std::fwrite(dest_buffer.data(), 1, length, fp);

		std::fputs("\nendstream\nendobj\n", fp);

		return true;
	}
};