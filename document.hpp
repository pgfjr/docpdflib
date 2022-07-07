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
#include "objects.hpp"
#include "fonts.hpp"
#include "compressor.hpp"
#include "image_manager.hpp"


class page_resources
{
    std::set<int32_t> m_font_obj_number_list;
    std::set<int32_t> m_image_obj_number_list;
public:
    page_resources() : m_font_obj_number_list(), m_image_obj_number_list()
    {
    }
    ~page_resources()
    {
        clear();
    }
    void clear()
    {
        m_font_obj_number_list.clear();
        m_image_obj_number_list.clear();
    }
    bool empty() const
    {
        return m_font_obj_number_list.empty() && m_image_obj_number_list.empty();
    }
    void add_font_obj_number(int32_t m_number)
    {
        m_font_obj_number_list.insert(m_number);
    }
    void add_image_obj_number(int32_t m_number)
    {
        m_image_obj_number_list.insert(m_number);
    }
    void write(FILE* fp)
    {
        std::fputs("\n\t<<\n", fp);

        if (!m_font_obj_number_list.empty())
        {
            std::fputs("\t/Font <<\n", fp);

            for (auto i : m_font_obj_number_list)
            {
                // the font m_number is also the object m_number
                std::fprintf(fp, "\t\t/F%d %d 0 R\n", i, i);
            }
            std::fputs("\t\t>>\n", fp);
        }
        if (!m_image_obj_number_list.empty())
        {
            std::fputs("\t/XObject <<\n", fp);

            for (auto i : m_image_obj_number_list)
            {
                // the image m_number is also the object m_number
                std::fprintf(fp, "\t\t/Im%d %d 0 R\n", i, i);
            }
            std::fputs("\t\t>>\n", fp);
        }
        std::fputs("\t>>\n", fp);

        clear();
    }
};


class docpdf
{
    object_list m_obj_list;
    page_resources m_resources;
    font_manager m_font_mgr;
    image_manager m_image_mgr;
    FILE* m_output_file{ stdout };
    ULONG_PTR gdiplusToken{ 0 };

    error_type m_last_error{ error_type::none };

    bool close_file{ false };

private:
    void write_header()
    {
        std::fputs("%PDF-1.4\n%\x84\x85\x86\x87\n", m_output_file);
    }
    int32_t write_content_stream(std::ostringstream &stream)
    {
        const std::string str = stream.rdbuf()->str();
        const byte_t* source_data = (byte_t*)str.data();
        long source_length = (long)str.length();
        object_record* content = m_obj_list.next_object();
        stream_compressor compressor;
        byte_vector dest_data;

        stream.rdbuf()->str(std::string(""));
        stream.clear();

        content->write(m_output_file);

        //compress_stream(source_data, source_length);

        if (compressor.compress(dest_data, source_data, source_length, 9))
        {
            long dest_length = (long)dest_data.size();

            std::fprintf(m_output_file, "<</Length %ld/Filter /FlateDecode>>\nstream\n", dest_length);

            std::fwrite(dest_data.data(), 1, dest_length, m_output_file);
        }
        else
        {
            std::fprintf(m_output_file, "<</Length %d>>\nstream\n", source_length);

            std::fwrite(source_data, 1, source_length, m_output_file);
        }

        std::fputs("\nendstream\nendobj\n", m_output_file);

        return content->m_number;
    }

    void write_page_info(int32_t page_content_obj_number, real_t page_width, real_t page_height, int32_t page_rotation)
    {
        object_record* page = m_obj_list.new_page_object();


        page->write(m_output_file);

        std::fputs("<<\n/Type /Page\n/Parent 1 0 R\n", m_output_file);

        std::fprintf(m_output_file, "/MediaBox [0 0 %f %f]\n/Contents [%d 0 R]\n", page_width, page_height, page_content_obj_number);

        if (page_rotation != 0)
        {
            std::fprintf(m_output_file, "/Rotation %d\n", page_rotation);
        }

        std::fputs("/Resources ", m_output_file);

        if (m_resources.empty())
        {
            std::fputs("<<>>\n", m_output_file);
        }
        else
        {
            m_resources.write(m_output_file);
        }

        std::fputs(">>\nendobj\n", m_output_file);
    }


public:
    docpdf() : m_obj_list(), m_resources(), m_font_mgr(), m_image_mgr()
    {

    }

    ~docpdf()
    {
        close();
    }


    void close()
    {
        if (!close_file)
        {
            m_font_mgr.write_font(m_output_file);

            m_obj_list.write_ender(m_output_file);

            if (m_output_file && m_output_file != stdout)
            {
                std::fclose(m_output_file);

                m_output_file = stdout;
            }

            m_obj_list.clear();
            m_resources.clear();
            m_font_mgr.clear();
            m_image_mgr.clear();

            Gdiplus::GdiplusShutdown(gdiplusToken);
        
            close_file = true;
        }
    }

    bool create(const char* filename)
    {
        if (!filename)
        {
            m_last_error = error_type::missing_filename;
        }
        
        else
        {
            FILE* tmp = nullptr;

            fopen_s(&tmp, filename, "wb");

            if (!tmp)
            {
                m_last_error = error_type::file_create_error;
            }
            else
            {
                Gdiplus::GdiplusStartupInput gdiplusStartupInput;

                m_output_file = tmp;

                write_header();

                m_last_error = error_type::none;

                GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

                return true;
            }
        }

        return false;
    }
    void write_page(std::ostringstream &stream, real_t page_width, real_t page_height, int32_t page_rotation)
    {
        int32_t page_content_obj_number;

        page_content_obj_number = write_content_stream(stream);


        write_page_info(page_content_obj_number, page_width, page_height, page_rotation);
    }
    font_record* find_font(const char* m_basefont)
    {
        font_record* font = m_font_mgr.find_font(m_basefont);

        if (font)
        {
            if (0 == font->m_number)
            {
                font->m_obj_number = m_obj_list.next_object();                

                font->m_number = font->m_obj_number->m_number;

                if (!font->m_is_base_font)
                {
                    font->m_font_descriptor_number = m_obj_list.next_object();

                    font->m_font_file_number = m_obj_list.next_object();
                }

            }

            // add this to the page resource m_list
            m_resources.add_font_obj_number(font->m_number);

            return font;
        }

        return nullptr;
    }
    int32_t find_image(const char* filename)
    {
        int object_number = m_image_mgr.find_image(filename);

        if (NOTFOUND == object_number)
        {
            object_record* obj = m_obj_list.next_object();

            obj->write(m_output_file);

            if (m_image_mgr.add_image(filename, obj->m_number, m_output_file))
            {
                m_resources.add_image_obj_number(obj->m_number);

                return obj->m_number;
            }
            // close the object in case of failure to avoid a corrupt file
            std::fputs("endob\n", m_output_file);
        }
        else
        {
            m_resources.add_image_obj_number(object_number);
        }
        return object_number;
    }
};



