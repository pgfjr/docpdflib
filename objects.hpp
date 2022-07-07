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

// holds the object numbers and the object offsets
struct object_record
{
    int32_t m_number{ 0 };
    int32_t m_offset{ 0 };
    void write(FILE* fp)
    {
        m_offset = ftell(fp);
        std::fprintf(fp, "%d 0 obj\n", m_number);
    }
};


// holds the list of objects
class object_list
{
    int m_counter{ 0 };
    std::vector<object_record*> m_list;
    object_record* m_page_tree{ nullptr };
    object_record* m_catalog{ nullptr };
    int_vector m_page_list;

    void write_xref(FILE* fp)
    {
        long xref = std::ftell(fp);
        int object_count = (int)(m_list.size() + 1);
        int generation_number = object_count;

        std::fprintf(fp, "xref\n0 %u\n", object_count);

        std::fputs("0000000000 65535 f\r\n", fp);

        for (object_record* obj : m_list)
        {
            if (obj->m_offset != 0)
            {
                std::fprintf(fp, "%010d 00000 n\r\n", obj->m_offset);
            }
            else
            {
                std::fprintf(fp, "%010d %05d f\r\n", obj->m_offset, ++generation_number);
            }
        }

        std::fprintf(fp, "trailer\n<</Size %u\n/Root 2 0 R\n>>\n", object_count);

        std::fprintf(fp, "startxref\n%ld\n%%%%EOF", xref);
    }

    void write_page_tree(FILE* fp)
    {
        m_page_tree->write(fp);

        std::fprintf(fp, "<</Type /Pages\n/Count %u\n/Kids [\n", (unsigned)m_page_list.size());

        for (int i : m_page_list)
        {
            std::fprintf(fp, "\t%d 0 R\n", i);
        }

        std::fputs("\t]\n>>\nendobj\n", fp);
    }

    void write_catalog(FILE* fp)
    {
        m_catalog->write(fp);

        std::fputs("<</Type /Catalog\n/Pages 1 0 R\n>>\nendobj\n", fp);
    }

public:
    object_list() : m_list(), m_page_list()
    {
        m_list.reserve(32);
        m_page_list.reserve(32);

        m_page_tree = next_object();
        m_catalog = next_object();
    }
    ~object_list()
    {
        m_list.clear();
    }
    void clear()
    {
        m_list.clear();
        m_page_list.clear();    }

    object_record* next_object()  // create a new object
    {
        object_record* tmp = new object_record;

        if (tmp)
        {
            try
            {
                tmp->m_number = ++m_counter;

                m_list.push_back(tmp);

                return tmp;
            }
            catch (...)
            {
            }
        }

        throw std::runtime_error("Out of memory");
    }

    object_record* new_page_object() // create a new object for page use
    {
        object_record* tmp = next_object();

        try
        {
            m_page_list.push_back(tmp->m_number); // add the page number for use in the root node

            return tmp;
        }
        catch (...)
        {
            if (tmp)
            {
                delete tmp;
            }
            throw std::runtime_error("Out of memory");
        }
    }


    void write_ender(FILE* fp)
    {
        write_page_tree(fp);
        write_catalog(fp);
        write_xref(fp);
    }

};



