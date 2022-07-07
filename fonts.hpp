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
#include "matrix.hpp"
#include "compressor.hpp"

struct font_record
{
    //todo: make private
    int32_t m_number{ 0 };
    uint32_t m_first_char{ 0 };
    uint32_t m_last_char{ 0 };
    std::string m_subtype;//todo change to sub_type
    std::string m_basefont;//todo change to base_font
    std::string m_font_path;
    bool m_is_base_font{ false };
    int32_t m_ascent{ 0 };
    int32_t m_descent{ 0 };
    int32_t m_cap_height{ 0 };
    int32_t m_x_height{ 0 };//todo optional
    int32_t m_internal_leading{ 0 };
    int32_t m_external_leading{ 0 };
    int32_t m_font_bbox[4]{ 0 };
    int_vector m_glyph_widths;
    int32_t* m_pwidths{ nullptr }; // points to the data of m_glyph_widths
    object_record* m_obj_number{ nullptr };
    object_record *m_font_descriptor_number{ nullptr };
    object_record* m_font_file_number{ nullptr };
    matrix m_matrix;
    real_t m_em_square{ 1000.0f };
    real_t m_italic_angle{ 0.0f };// todo
    real_t m_stemV{ 80.0f };//guessed
    bool m_font_in_use{ false };
    std::string m_type1_full_path; // *pfm and *pfb combined
#ifdef _WIN32
    HFONT m_hfont{ nullptr };
#endif
    font_record() : m_subtype(), m_basefont(), m_font_path(), m_matrix(), m_type1_full_path()
    {}
    virtual ~font_record()
    {
#ifdef _WIN32
        if (m_hfont)
        {
            DeleteObject(m_hfont);

            if (m_subtype == "Type1")
            {
                RemoveFontResourceA(m_type1_full_path.c_str());
            }
            else
            {
                RemoveFontResourceA(m_font_path.c_str());
            }
        }
#endif

    }
    virtual int32_t width(uint8_t c)
    {
        if (c >= m_first_char && c <= m_last_char)
        {
            return m_pwidths[c - (uint8_t)m_first_char];
        }
        return 0;
    }
    virtual int32_t width(uint32_t c)
    {
        if (c >= m_first_char && c <= m_last_char)
        {
            return m_pwidths[c - m_first_char];
        }
        return 0;
    }
    real_t scaled_width(uint8_t c)
    {
        return real_t(width((uint8_t)c)) * size() / em_square();
    }
    real_t scaled_width(uint32_t c)
    {
        return real_t(width((uint32_t)c)) * size() / em_square();
    }
    real_t size() const
    {
        return m_matrix.sy;
    }
    void scale(real_t new_size)
    {
        m_matrix.sx = m_matrix.sy = new_size;
    }
    void scale(real_t width, real_t height)
    {
        m_matrix.sx = width;
        m_matrix.sy = height;
    }
    real_t ascent(bool scaled = true) const
    {
        if (scaled)
            return real_t(m_ascent) * size() / m_em_square;
        else
            return real_t(m_ascent);
    }
    real_t descent(bool scaled = true) const
    {
        if (scaled)
            return real_t(m_descent) * size() / m_em_square;
        else
            return real_t(m_descent);
    }
    real_t em_square() const
    {
        return m_em_square;
    }
    real_t height(bool scaled = true)
    {
        if (scaled)
            return ascent() + fabs(descent());
        else
            return real_t(m_ascent) + fabs(real_t(m_descent));
    }
    real_t internal_leading(bool scaled = true) const
    {
        if (scaled)
            return real_t(m_internal_leading) * size() / m_em_square;
        else
            return real_t(m_internal_leading);
    }
    real_t external_leading(bool scaled = true) const
    {
        if (scaled)
            return real_t(m_external_leading) * size() / m_em_square;
        else
            return real_t(m_external_leading);
    }
    matrix transform() const
    {
        return m_matrix;
    }
    void in_use(bool value)
    {
        m_font_in_use = value;
    }
    bool in_use() const
    {
        return m_font_in_use;
    }
    int32_t number() const
    {
        return m_number;
    }
    void write_font_descriptor(FILE* fp)
    {
        m_font_descriptor_number->write(fp);

        fprintf(fp, "<</Type /FontDescriptor\n/FontName /%s\n", m_basefont.c_str());

        fprintf(fp, "/FontBBox [%d %d %d %d]\n", m_font_bbox[0], m_font_bbox[1], m_font_bbox[2], m_font_bbox[3]);

        fprintf(fp, "/Flags %d\n", 4);// font->m_flag);

        fprintf(fp, "/Ascent %d\n", m_ascent);// font->m_ascent);

        fprintf(fp, "/Descent %d\n", m_descent);// font->m_descent);

        fprintf(fp, "/ItalicAngle %f\n", m_italic_angle);

        fprintf(fp, "/StemV %f\n", m_stemV);

        fprintf(fp, "/CapHeight %d\n", m_cap_height);// font->m_capheight);

        if (m_subtype == "Type1")
        {
            fprintf(fp, "/FontFile %d 0 R\n", m_font_file_number->m_number);
        }
        else if (m_subtype == "TrueType")
        {
            fprintf(fp, "/FontFile2 %d 0 R\n", m_font_file_number->m_number);
        }
        else
        {
            fprintf(fp, "/FontFile3 %d 0 R\n", m_font_file_number->m_number);
        }
        fputs(">>\nendobj\n", fp);
    }
    void write_font_info(FILE* fp)
    {
        m_obj_number->write(fp);

        fprintf(fp, "<</Type /Font\n/Subtype /%s\n/BaseFont /%s\n", m_subtype.c_str(), m_basefont.c_str());

        if (!m_is_base_font)
        {
            fprintf(fp, "/FirstChar %d\n", m_first_char);

            fprintf(fp, "/LastChar %d\n", m_last_char);

            {
                int n = 0;

                fputs("/Widths [\n", fp);
                for (int w : m_glyph_widths)
                {
                    fprintf(fp, "%d ", w);

                    // only 20 per row
                    if (++n == 20)
                    {
                        fputc('\n', fp);
                        n = 0;
                    }
                }
                fputs("]\n", fp);
            }

            fprintf(fp, "/FontDescriptor %d 0 R\n", m_font_descriptor_number->m_number);
        }

        fputs(">>\nendobj\n", fp);
    }
    void write_type1_font(FILE* fp)
    {
        FILE* tfile = nullptr;

        fopen_s(&tfile, m_font_path.c_str(), "rb");

        if (!tfile)
        {
            return;
        }
        else
        {
            long length1 = 0, length2 = 0, total_length;
            uint16_t hdr[3]{ 0 };
            byte_vector source_buffer, dest_buffer;
            byte_t* source;
            stream_compressor compressor;

            // read the header
            std::fread(hdr, 1, sizeof(hdr), tfile);

            // get the offset of the binary data; this also indicates the length
            // of the text part
            length1 = hdr[1];

            // go to the second header; location is relative to current position
            std::fseek(tfile, length1, SEEK_CUR);

            // read the header
            std::fread(hdr, 1, sizeof(hdr), tfile);

            // get the length
            length2 = hdr[1];

            // return to the top after the first header
            std::fseek(tfile, 6, SEEK_SET);

            // allocate the buffer
            total_length = length1 + length2;

            source_buffer.resize(total_length);

            source = source_buffer.data();

            // read the text part
            fread(source, 1, length1, tfile);

            // skip the 2nd header
            std::fseek(tfile, 6, SEEK_CUR);
            
            // read the binary data
            fread(source+length1, 1, length2, tfile);

            // done with the file
            fclose(tfile);            

            if (compressor.compress(dest_buffer, source, total_length, 9))
            {
                // change to the size of the compressed data
                total_length = (long)dest_buffer.size();

                // length3 is the text portion after the binary data; it's optional
                fprintf(fp, "<</Filter /FlateDecode /Length %ld /Length1 %ld /Length2 %ld /Length3 0>>\nstream\n", total_length, length1, length2);

                fwrite(dest_buffer.data(), 1, total_length, fp);
            }
            else
            {
                fprintf(fp, "<<//Length %ld /Length1 %ld /Length2 %ld /Length3 0>>\nstream\n", total_length, length1, length2);

                // write the source data uncompressed

                fwrite(source, 1, total_length, fp);
            }
            fputs("\nendstream\n", fp);
        }
    }
    void write_font_file(FILE *fp)
    {
        //int length1 = 0, length2 = 0, length3 = 0;

        m_font_file_number->write(fp);

        if (m_subtype == "Type1")
        {
            //todo
            write_type1_font(fp);
        }
        fputs("endobj\n", fp);
    }
    void write(FILE* fp)
    {
        write_font_info(fp);

        if (m_font_descriptor_number)
        {
            write_font_descriptor(fp);
        }
        if (m_font_file_number)
        {
            write_font_file(fp);
        }
    }
};

static struct base_fonts
{
    const char* base_name;
    //const char* face_nam
    const char* font_name;
    const char* m_font_path;
} base_font_table[] = {
{"Times-Roman", "NimbusRomNo9L-Regu", "./fonts/times/utmr8a"},
{"Times-Bold", "NimbusRomNo9L-Medi", "./fonts/times/utmb8a"},
{"Times-Italic", "NimbusRomNo9L-ReguItal", "./fonts/times/utmri8a"},
{"Times-BoldItalic", "NimbusRomNo9L-MediItal", "./fonts/times/utmbi8a"},
{"Helvetica", "NimbusSanL-Regu", "./fonts/helvetica/uhvr8a"},
{"Helvetica-Bold", "NimbusSanL-Bold", "./fonts/helvetica/uhvb8a"},
{"Helvetica-Oblique", "NimbusSanL-ReguItal", "./fonts/helvetica/uhvro8a"},
{"Helvetica-BoldOblique", "NimbusSanL-BoldItal", "./fonts/helvetica/uhvbo8a"},
{"Courier", "NimbusMonL-Regu", "./fonts/courier/ucrr8a"},
{"Courier-Bold", "NimbusMonL-Bold", "./fonts/courier/ucrb8a"},
{"Courier-Oblique", "NimbusMonL-ReguObli", "./fonts/courier/ucrro8a"},
{"Courier-BoldOblique", "NimbusMonL-BoldObli", "./fonts/courier/ucrbo8a"},
{"Symbol", "StandardSymL", "./fonts/symbol/usyr"},
{"ZapfDing", "Dingbats", "./fonts/zapfding/uzdr"}
};


class font_manager
{
    std::map<std::string, font_record*> m_table;
private:
    font_record* search_table(const std::string& key)
    {
        auto it = m_table.find(key);

        if (it != m_table.end())
        {
            return it->second;
        }

        return nullptr;
    }
    font_record* load_base_font(const char* m_basefont)
    {
        const int table_size = sizeof(base_font_table) / sizeof(base_font_table[0]);

        for (int i = 0; i < table_size; ++i)
        {
            const char* base_name = base_font_table[i].base_name;

            if (_strcmpi(m_basefont, base_name) == 0)
            {
                // check if it's already in the table
                font_record* font = search_table(std::string(base_name));

                if (font)
                {
                    return font;
                }
                else
                {
                    std::string pfm_file(base_font_table[i].m_font_path);
                    std::string pfb_file(pfm_file);

                    pfm_file.append(".pfm");
                    pfb_file.append(".pfb");

                    // load the font
                    return load_type1_font(pfm_file.c_str(), pfb_file.c_str(), base_name, base_font_table[i].font_name, true);
                }
            }
        }

        return nullptr;
    }

    HFONT get_font_metrics(const char* font_path, const char *face_name, OUTLINETEXTMETRICA& otm, int_vector &m_glyph_widths)
    {
        // install the fonts and read the metrics
        if (AddFontResourceA(font_path) != 0)
        {
            TEXTMETRICA& tm = otm.otmTextMetrics;
            HFONT hfont;
            HGDIOBJ hprev;
            LOGFONTA lf{ 0 };
            HDC hdc;
            size_t glyph_count;

            lf.lfHeight = -1000; // 1000 for PostScript fonts; 2048 for TrueType fonts
            lf.lfCharSet = DEFAULT_CHARSET;

            lstrcpyA(lf.lfFaceName, face_name);// "Nimbus Roman No9 L");// ps_font_name);

            hfont = CreateFontIndirectA(&lf);

            hdc = GetDC(0);

            hprev = SelectObject(hdc, hfont);

            GetOutlineTextMetricsA(hdc, otm.otmSize, &otm);

            glyph_count = tm.tmLastChar - tm.tmFirstChar + 1;

            m_glyph_widths.resize(glyph_count);

            GetCharWidth32A(hdc, tm.tmFirstChar, tm.tmLastChar, m_glyph_widths.data());

            // restore the previous font
            SelectObject(hdc, hprev);

            ReleaseDC(0, hdc);

            //DeleteObject(hfont);

            //RemoveFontResourceA(font_path);

            return hfont;
        }

        return nullptr;
    }
    font_record* load_type1_font(const char* pfm_name, const char* pfb_name, const char *base_name, const char* face_name, bool m_is_base_font)
    {
        std::string m_font_path(pfm_name);

        m_font_path.push_back('|');
        m_font_path.append(pfb_name);
        OUTLINETEXTMETRICA otm{ 0 };
        TEXTMETRICA& tm = otm.otmTextMetrics;
        font_record* font;

        otm.otmSize = sizeof(otm);

        font = new font_record;

        if (font)
        {
            HFONT hfont = get_font_metrics(m_font_path.c_str(), face_name, otm, font->m_glyph_widths);

            if (hfont != nullptr)
            {
                font->m_is_base_font = m_is_base_font;
                font->m_first_char = tm.tmFirstChar;
                font->m_last_char = tm.tmLastChar;
                font->m_internal_leading = tm.tmInternalLeading;
                font->m_external_leading = tm.tmExternalLeading;
                font->m_ascent = otm.otmAscent;
                font->m_descent = otm.otmDescent;
                font->m_cap_height = otm.otmsCapEmHeight;
                font->m_x_height = otm.otmsXHeight;
                font->m_font_bbox[0] = otm.otmrcFontBox.left;
                font->m_font_bbox[1] = otm.otmrcFontBox.bottom;
                font->m_font_bbox[2] = otm.otmrcFontBox.right;
                font->m_font_bbox[3] = otm.otmrcFontBox.top;
                font->m_pwidths = font->m_glyph_widths.data();
                font->m_basefont = base_name;
                font->m_font_path = pfb_name;
                font->m_subtype = "Type1";
                font->m_em_square = (real_t)otm.otmEMSquare; // this should be 1000; otherwise, a different font was loaded by GDI, most likely a TrueType1 one
                font->m_hfont = hfont;
                font->m_type1_full_path = m_font_path;

                // install this font into the table
                m_table[font->m_basefont] = font;

                return font;
            }
            else
            {
                delete font;
            }
        }
        
        return nullptr;
    }


    bool get_font_name(const char* filename, std::string& font_name)
    {
        FILE* fp = nullptr;

        fopen_s(&fp, filename, "rb");

        if (!fp)
        {
            return false;
        }
        else
        {
            const int buf_size = 512;
            bool result = false;
            char buf[buf_size + 1];
            uint8_t header[6]{ 0 };

            // read the 6-byte header

            std::fread(header, 1, sizeof(header), fp);

            if (header[0] != 128 && header[1] != 1)
            {
                std::fclose(fp);

                return false;
            }

            // read 1 line at a time
            while (std::fgets(buf, buf_size, fp))
            {
                char* p = buf;
                char* pos;

                while (isspace((uint8_t)*p))
                    ++p;

                // skip the comments; several of them at the start of the font file
                if ('%' == *p)
                    continue;

                pos = strstr(p, "/FontName");

                if (pos)
                {
                    char ps_font_name[40]{ 0 }; // big enough; m_basefont is usually <= 32 bytes

                    pos += 9;// skip /FontName

                    // read the m_basefont; it should begin with '/'. 
                    // This also assumes the m_basefont is on the same line as the key '/FontName' and follows it 
                    if (sscanf_s(pos, "%s", ps_font_name, (unsigned)sizeof(font_name)) == 1)
                    {
                        if ('/' == *ps_font_name)
                        {
                            font_name = ps_font_name + 1;

                            result = true;
                        }
                    }
                    break;
                }

            }

            std::fclose(fp);

            return result;
        }
    }

public:
    font_manager() : m_table()
    {

    }

    ~font_manager()
    {
        clear();
    }
    void clear()
    {
        if (!m_table.empty())
        {
            for (auto i : m_table)
            {
                if (i.second)
                {
                    delete i.second;

                    i.second = nullptr;
                }
            }

            m_table.clear();
        }
    }
    font_record* find_font(const char* m_basefont)
    {
        // check if there's an extension
        const char* ext = strrchr(m_basefont, '.');

        if (ext)
        {
            if (_strcmpi(ext, ".t1") == 0 || _strcmpi(ext, ".pfb") == 0)
            {
                std::string pfm_file;
                std::string font_name;

                pfm_file.append(m_basefont, ext - m_basefont);
                pfm_file.append(".pfm");

                // open the Type1 font file and read its /FontName
                if (get_font_name(m_basefont, font_name))
                {
                    // check if it's in the table already
                    font_record* font = search_table(font_name);

                    if (font)
                    {
                        return font;
                    }
                    // load it instead
                    return load_type1_font(pfm_file.c_str(), m_basefont, font_name.c_str(), font_name.c_str(), false);
                }
            }

            //use the default font
            return load_base_font("Times-Roman");
        }
        else
        {
            // if no extension given, assume this is just a m_basefont; perhaps a one of the base fonts
            font_record *font = load_base_font(m_basefont);

            if (!font)
            {
                //use the default font
                return load_base_font("Times-Roman");
            }

            return font;
        }
    }
    void write_font(FILE *m_output_file)
    {
        // write the font object to the file
        for (auto it : m_table)
        {
            font_record* font = it.second;

            // write the font only if it was used
            if (font && font->m_font_in_use)
            {
                font->write(m_output_file);
            }
        }
    }
};


