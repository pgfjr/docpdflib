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


//NOTE: Include the header below and the files agg_bezier_arc.cpp and agg_trans_affine.cpp to compile this example

#include "docpdflib.hpp"

void rotated_text(pdf_page& page)
{
    real_t y;
    
    page.selectfont("Times-Roman", 18);
    y = page.height() - page.currentfontsize();
    page.moveto(0, y);
    page.show("Stroked Text (Selectable)");

    page.scalefont(32);
    page.translate(100, 200);
    page.rotate(45);
    page.scale(2, 1);
    page.moveto(5, 10);
    page.setstrokergb(0, 0, 1);
    page.setfillrgb(0, 1, 0);
    page.setrenderingmode(1);
    page.show("First Text");
    page.setrenderingmode(2);
    page.show(" Second Text");

    // call showpage to write the page to the document stream
    page.showpage();
}

void charpath(pdf_page& page)
{
    real_t y;

    page.selectfont("Times-Roman", 18);
    y = page.height() - page.currentfontsize();
    page.moveto(0, y);
    page.show("Text As Path (Not Selectable)");

    page.scalefont(24);
    page.translate(55, 200);
    page.rotate(45);
    page.scale(2, 1);
    page.moveto(0, 10);
    page.charpath("PostScript 'charpath' operator example");
    page.stroke();
    page.showpage();
}

void text_art(pdf_page& page)
{
    page.setfont("Times-Roman");
    page.scalefont(18);
    page.translate(100, 300);
    page.moveto(100, 100);
    page.show("Normal");
    page.moveto(100, 120);
    page.scale(2, 1);
    page.show("Stretched text");
    page.moveto(50, 140);
    page.rotate(40);
    page.show("Rotated text");
    page.showpage();
}


void draw_image(pdf_page& page)
{
    real_t y;

    page.selectfont("Times-Roman", 18);
    y = page.height() - page.currentfontsize();
    page.moveto(0, y);
    page.show("Scaled image example");

    page.gsave();
    page.scale(.15f, .15f);
    page.translate(20, 1500);
    page.image("Sagrada Familia Interior.jpg", 0, 0, 4032, 2268);
    page.grestore();
    page.showpage();
}



void draw_ellipse(pdf_page& page)
{
    real_t y;

    page.selectfont("Times-Roman", 18);
    y = page.height() - page.currentfontsize();
    page.moveto(0, y);
    page.show("Ellipse examples");

    page.setfillrgb(0, .5f, .75f);
    page.ellipse(100, 100, 50, 50);
    page.fill();

    page.setlinewidth(3.5f);
    page.setstrokergb(.6f, .75f, .4f);
    page.ellipse(250, 250, 50, 75);
    page.stroke();

    page.ellipse(450, 350, 75, 50);
    page.fill_and_stroke();
    page.showpage();
}

void draw_bezier(pdf_page &page)
{
    real_t y;

    page.selectfont("Times-Roman", 18);
    y = page.height() - page.currentfontsize();
    page.moveto(0, y);
    page.show("Bezier curve example");

    pointf pt1, pt2, pt3, pt4;
    real_t height = 11, internal_leading;

    page.translate(120, 400);
    page.selectfont("Times-Roman", height);
    internal_leading = page.font_internal_leading();

    page.setgray(.75f);
    page.moveto(0, 100);
    pt1 = page.currentpoint();
    pt1.y -= (height + internal_leading);
    page.lineto(150, 300);
    pt2 = page.currentpoint();
    pt2.y += height;

    page.lineto(350, 270);
    pt3 = page.currentpoint();
    pt3.y += height;

    page.lineto(400, 90);
    pt4 = page.currentpoint();

    pt4.y -= (height + internal_leading);

    page.stroke();

    page.setgray(0);
    page.moveto(pt1.x-4, pt1.y);
    page.show("P1");

    page.moveto(pt2.x - 4, pt2.y);
    page.show("P2");

    page.moveto(pt3.x - 4, pt3.y);
    page.show("P3");

    page.moveto(pt4.x - 4, pt4.y);
    page.show("P4");

    page.setstrokergb(0, 0, 1);

    page.moveto(0, 100);
    page.curveto(150, 300, 350, 275, 400, 90);
    page.stroke();
    page.setfillrgb(1, 0, 0);
    page.ellipse(0, 100, 3, 3);
    page.fill();
    page.ellipse(150, 300, 3, 3);
    page.fill();
    page.ellipse(350, 270, 3, 3);
    page.fill();
    page.ellipse(400, 90, 3, 3);
    page.fill();
    page.showpage();
}

void draw_pie(pdf_page& page)
{
    real_t y;

    page.selectfont("Times-Roman", 18);
    y = page.height() - page.currentfontsize();
    page.moveto(0, y);
    page.show("Pie chart example using arcs");

    page.translate(200, 400);
    page.scale(2, 2);
    page.setfillrgb(.11f, .51f, .50f);
    page.moveto(50, 50);
    page.arc(50, 50, 50, 90, 360);
    page.closepath();
    page.fill();
    page.moveto(55, 55);
    page.arcn(55, 55, 50, 90, 0);
    page.setfillrgb(.31f, .71f, .60f);
    page.closepath();
    page.fill();
    page.showpage();
}


void text_demo(pdf_page& page)
{
    const char* name[] = { "Times-Roman", "Times-Bold", "Times-Italic","Times-BoldItalic",
                            "Helvetica","Helvetica-Bold","Helvetica-Oblique","Helvetica-BoldOblique",
                            "Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique" };
    const real_t sizes[] = {14, 16, 18, 20, 14, 16, 18, 20 , 14, 16, 18, 20 , 14, 16, 18, 20 };
    const int count = sizeof(name) / sizeof(name[0]);
    real_t y = page.height();
    char buffer[128];

    real_t internal_leading;
   
    for (int i = 0; i < count; ++i)
    {
        real_t font_height = sizes[i];
        page.selectfont(name[i], font_height);

        internal_leading = page.font_internal_leading();

        y -= (font_height + internal_leading);
        page.moveto(0, y);
        sprintf_s(buffer, sizeof(buffer), "%s font at %.1f pts", name[i], font_height);
        page.show(buffer);
    }
    page.showpage();
}

int main()
{
    docpdflib doc;

    // create a document for writing
    // ENSURE that the 'fonts' folder is in the current directory
    if (doc.create("example.pdf"))
    {
        // create a page object
        // 612 = width; 792 = height; 0 = rotation (must be a multiple of 90)
        // each page can have different sizes

        pdf_page page(doc, 612, 792, 0);

        draw_bezier(page);
        draw_pie(page);
        draw_ellipse(page);

        rotated_text(page);
        
        charpath(page);

        text_art(page);

        draw_image(page);

        text_demo(page);

        doc.close(); // optional
    }    
}


