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
#include "document.hpp"
#include "graphics_state.hpp"
#include "path_data.hpp"
#include "agg_bezier_arc.h"

using point_array = std::vector<pointf>;
class pdf_page
{
	docpdf& m_doc;
	real_t m_page_width{ 0 };
	real_t m_page_height{ 0 };
	int32_t m_page_rotation{ 0 };
	error_type m_error_type{ error_type::none };

	graphics_state m_gstate;

	std::ostringstream m_stream;

	path_data m_path_data;
	std::stack<graphics_state> m_graphics_stack;
	std::stack<path_data> m_path_stack;
	std::string m_error_message;
private:
	void save_path()
	{
		try
		{
			m_path_stack.push(m_path_data);
		}
		catch (...)
		{
			throw;
		}
	}
	void restore_path()
	{
		if (!m_path_stack.empty())
		{
			m_path_data = m_path_stack.top();

			m_path_stack.pop();
		}
	}
	void _stringwidth(const byte_t* char_codes, size_t count, real_t& width, real_t& height)
	{
		if (char_codes)
		{
			font_record* font = m_gstate.font();

			width = 0;

			for (size_t i = 0; i < count; ++i)
			{
				real_t w = font->scaled_width(char_codes[i]);

				width += w;
			}

			height = font->height();
		}
	}	
	void prepare_graphics()
	{
		bool apply_stroke = false;
		bool apply_fill = false;

		switch (currentrenderingmode())
		{
		case 0:
			apply_fill = true;
			break;
		case 1:
			apply_stroke = true;
			break;
		case 2:
			apply_stroke = apply_fill = true;
			break;
		case 4:
			apply_fill = true;
			break;
		case 5:
			apply_stroke = true;
			break;
		case 6:
			apply_stroke = apply_fill = true;
			break;
		case 3:
		case 7:
		default:
			return;
		}

		if (apply_stroke)
		{
			m_gstate.on_stroke(m_stream, m_gstate.currentmatrix());
		}
		if (apply_fill)
		{
			m_gstate.on_fill(m_stream);
		}
	}
	bool write_text(real_t x, real_t y, const byte_t* char_codes, size_t count)
	{
		pointf current_point{ 0, y };
		font_record* font = m_gstate.font();
		matrix font_ctm = font->transform();
		real_t font_size = font_ctm.sy;
		real_t total_width = 0;
		matrix ctm = m_gstate.currentmatrix();


		m_stream << "q\n";

		m_gstate.write_clip(m_stream);

		if (!ctm.is_identity())
		{
			m_stream << ctm << " cm\n";
		}

		prepare_graphics();

		m_stream << "BT\n";

		m_stream << (int)currentrenderingmode() << " Tr\n";

		font_ctm.tx += x;
		font_ctm.ty += y;

		m_stream << font_ctm << " Tm\n";

		m_stream << "/F" << font->number() << " 1.0 Tf\n";

		font->in_use(true);

		m_stream << '(';

		for (size_t i = 0; i < count; ++i)
		{
			byte_t ch = char_codes[i];

			real_t w = font->scaled_width(ch);

			total_width += w;

			if (isprint(ch))
			{
				if (ch != '(' && ch != ')')
				{
					m_stream << ch;
				}
				else
				{
					m_stream << '\\' << ch;
				}
			}
			else
			{
				m_stream << '\\' << std::setfill('0') << std::setw(3) << std::oct << (short)ch;
			}
		}

		m_stream << ") Tj\n";

		m_stream << "ET\nQ\n";

		current_point.x += total_width;

		moveto(current_point.x, current_point.y);

		m_error_type = error_type::none;

		return true;
	}
public:
	pdf_page(docpdf& doc, real_t width, real_t height, int32_t rotation) : m_doc(doc), m_stream(), m_gstate(), 
							m_path_data(), m_graphics_stack(), m_path_stack(), m_error_message()
	{
		if (width <= 0)
		{
			throw std::runtime_error("Invalid width");
		}
		else if (height <= 0)
		{
			throw std::runtime_error("Invalid height");
		}
		else if ( rotation < 0 || (rotation % 90) != 0)
		{
			throw std::runtime_error("Invalid rotation");
		}
		else
		{
			font_record* font = m_doc.find_font("Times-Roman");

			if (!font)
			{
				throw std::runtime_error("Default font not found");
			}
			else
			{
				font->scale(11.0f);
				m_gstate.font( font );
			}
			m_page_width = width;
			m_page_height = height;
			m_page_rotation = rotation;

			m_stream << std::fixed;
			m_stream << std::setprecision(2);
		}
	}
	~pdf_page()
	{
		if (m_stream.tellp() != std::streampos(0))
		{
			showpage();
		}
	}
	real_t height() const
	{
		return m_page_height;
	}
	real_t width() const
	{
		return m_page_width;
	}
	int rotation() const
	{
		return m_page_rotation;
	}
	void erasepage()
	{
		gsave();
		setfillrgb(1, 1, 1);
		setmatrix(1, 0, 0, 1, 0, 0);
		moveto(0, 0);
		lineto(0, m_page_height);
		lineto(m_page_width, m_page_height);
		lineto(m_page_width, 0);
		closepath();
		fill();
		grestore();
	}
	void showpage()
	{
		m_doc.write_page(m_stream, m_page_width, m_page_height, m_page_rotation);

		m_stream.str(std::string(""));
		m_stream.clear();

		m_path_data.newpath();

		m_gstate.reset();

		// clear the stacks
		while (!m_graphics_stack.empty())
		{
			m_graphics_stack.pop();
		}
		while (!m_path_stack.empty())
		{
			m_path_stack.pop();
		}		

		m_error_type = error_type::none;
	}
	bool setfont(const char* name)
	{
		font_record* font = m_doc.find_font(name);

		if (font)
		{
			m_gstate.font( font );

			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::invalid_font;

		return false;
	}
	bool scalefont(real_t size)
	{
		if (size >= 0)
		{
			m_gstate.font()->scale(size);

			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::range_check;

		return false;
	}
	bool selectfont(const char* name, real_t size)
	{
		if (setfont(name))
		{
			return scalefont(size);
		}

		m_error_type = error_type::invalid_font;

		return false;
	}
	real_t currentlinewidth()
	{
		m_error_type = error_type::none;

		return m_gstate.linewidth();
	}
	void setlinewidth(real_t value)
	{
		m_gstate.linewidth( value );

		m_error_type = error_type::none;
	}
	byte_t currentlinecap()
	{
		m_error_type = error_type::none;

		return m_gstate.linecap();
	}
	bool setlinecap(byte_t value)
	{
		if (m_gstate.linecap(value))
		{
			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::range_check;

		return false;
	}
	byte_t currentlinejoin() 
	{
		m_error_type = error_type::none;

		return m_gstate.linejoin();
	}
	bool setlinejoin(byte_t value)
	{
		if (m_gstate.linejoin(value))
		{
			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::range_check;

		return false;
	}
	void setgray(real_t value)
	{
		m_gstate.gray(value);

		m_error_type = error_type::none;
	}
	void setstrokergb(real_t r, real_t g, real_t b)
	{
		m_gstate.stroke_rgb(r, g, b);

		m_error_type = error_type::none;
	}
	void setfillrgb(real_t r, real_t g, real_t b)
	{
		m_gstate.fill_rgb(r, g, b);

		m_error_type = error_type::none;
	}
	// sets the fill and stroke to the same color, like in PS
	void setrgbcolor(real_t r, real_t g, real_t b)
	{
		m_gstate.setrgbcolor(r, g, b);

		m_error_type = error_type::none;

	}
	void setstrokecmyk(real_t c, real_t m, real_t y, real_t k)
	{
		m_gstate.stroke_cmyk(c, m, y, k);

		m_error_type = error_type::none;
		
	}
	void setfillcmyk(real_t c, real_t m, real_t y, real_t k)
	{
		m_gstate.fill_cmyk(c, m, y, k);

		m_error_type = error_type::none;
	}
	void setcmykcolor(real_t c, real_t m, real_t y, real_t k)
	{
		m_gstate.setcmykcolor(c, m, y, k);

		m_error_type = error_type::none;
	}
	void setflat(real_t value)
	{
		m_gstate.setflat(value);

		m_error_type = error_type::none;
	}
	real_t currentflat()
	{
		m_error_type = error_type::none;

		return m_gstate.m_flatness;
	}
	matrix currentmatrix() 
	{
		m_error_type = error_type::none;

		return m_gstate.currentmatrix();
	}
	void currentmatrix(real_t& a, real_t& b, real_t& c, real_t& d, real_t& e, real_t& f)
	{
		matrix mtx = m_gstate.currentmatrix();

		mtx.getmatrix(a, b, c, d, e, f);

		m_error_type = error_type::none;
	}
	void invertmatrix()
	{
		m_gstate.currentmatrix().invert_matrix();

		m_error_type = error_type::none;
	}
	void setmatrix(real_t a, real_t b, real_t c, real_t d, real_t e, real_t f)
	{
		m_gstate.setmatrix(a, b, c, d, e, f);

		m_error_type = error_type::none;
	}
	void concatmatrix(real_t a, real_t b, real_t c, real_t d, real_t e, real_t f)
	{
		m_gstate.transform(a, b, c, d, e, f);

		m_error_type = error_type::none;
	}
	void rotate(real_t degrees)
	{
		m_gstate.rotate(degrees);

		m_error_type = error_type::none;
	}
	void scale(real_t x, real_t y)
	{
		m_gstate.scale(x, y);

		m_error_type = error_type::none;
	}
	void translate(real_t x, real_t y)
	{
		m_gstate.translate(x, y);

		m_error_type = error_type::none;
	}
	void transform(real_t a, real_t b, real_t c, real_t d, real_t e, real_t f)
	{
		m_gstate.transform(a, b, c, d, e, f);

		m_error_type = error_type::none;
	}
	bool has_currentpoint() 
	{
		m_error_type = error_type::none;

		return m_gstate.has_currentpoint();
	}
	// the method below is preferred
	// this one always returns whatever currentpoint contains even if it was not explicitly set
	pointf currentpoint()
	{
		return m_gstate.currentpoint();
	}
	bool currentpoint(real_t& x, real_t& y)
	{
		if (m_gstate.has_currentpoint())
		{
			m_gstate.currentpoint(x, y);

			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::no_current_point;

		return false;
	}
	real_t currentfontsize() 
	{
		m_error_type = error_type::none;

		return m_gstate.font()->size();
	}
	real_t font_ascent() 
	{
		m_error_type = error_type::none;

		return m_gstate.font()->ascent();
	}
	real_t font_descent()
	{
		m_error_type = error_type::none;

		return m_gstate.font()->descent();
	}
	real_t font_internal_leading()
	{
		m_error_type = error_type::none;

		return m_gstate.font()->internal_leading();
	}
	real_t font_external_leading()
	{
		m_error_type = error_type::none;

		return m_gstate.font()->external_leading();
	}
	pointf angle_to_point(real_t angle, real_t cx, real_t cy, real_t radius, bool is_radian)
	{
		pointf pt;

		if (!is_radian)
		{
			angle = (real_t)(angle * M_PI / 180.0f);
		}
		pt.x = cx + radius * cos(angle);
		pt.y = cy + radius * sin(angle);

		return pt;
	}
	real_t ccw_to_cw(real_t angle)
	{
		if (angle >= 0 && angle <= 360)
		{
			angle = 360 - angle;
		}
		else if (angle < 0 && angle >= -360)
		{
			angle = -angle;
		}

		return angle;
	}
private:
	void do_arc_clockwise(int count, const double* vertices)
	{
		const double* v = vertices;

		if (has_currentpoint())
		{
			lineto((real_t)v[0], (real_t)v[1]);
		}
		else
		{
			moveto((real_t)v[0], (real_t)v[1]);
		}
		if (4 == count)
		{
			lineto((real_t)v[2], (real_t)v[3]);
		}
		else //if (count > 4)
		{
			for (int i = 2; i < count; i += 6)
			{
				curveto((real_t)v[i], (real_t)v[i + 1], (real_t)v[i + 2], (real_t)v[i + 3], (real_t)v[i + 4], (real_t)v[i + 5]);
			}
		}
	}
	void do_arc_anticlockwise(int count, const double* vertices)
	{
		const double* v = vertices;
		
		if (4 == count)
		{
			if (has_currentpoint())
			{
				lineto((real_t)v[2], (real_t)v[3]);
			}
			else
			{
				moveto((real_t)v[2], (real_t)v[3]);
			}

			lineto((real_t)v[0], (real_t)v[1]);
		}
		else //if (count > 4)
		{
			int last = count - 1;

			if (has_currentpoint())
			{
				lineto((real_t)v[last-1], (real_t)v[last]);
			}
			else
			{
				moveto((real_t)v[last-1], (real_t)v[last]);
			}
			// draw the curves in reverse order so that the first point becomes the last point
			for (int i = last - 2; i >= 0; i -= 6)
			{
				curveto((real_t)v[i-1], (real_t)v[i], (real_t)v[i - 3], (real_t)v[i - 2], (real_t)v[i - 5], (real_t)v[i - 4]);
			}
		}
	}
	bool do_arc(real_t cx, real_t cy, real_t rx, real_t ry, real_t start_angle, real_t end_angle, bool anticlockwise)
	{
		bool has_current_point = has_currentpoint();
		agg::bezier_arc bz;
		real_t sweep_angle, tmp;


		if (anticlockwise)
		{
			tmp = ccw_to_cw(start_angle);
			start_angle = ccw_to_cw(end_angle);

			end_angle = tmp;
		}
		else
		{
			start_angle = ccw_to_cw(start_angle);
			end_angle = ccw_to_cw(end_angle);
		}
		if (start_angle < end_angle)
		{
			sweep_angle = end_angle - start_angle;
		}
		else
		{
			sweep_angle = (real_t)fabs((real_t)(360.0f - start_angle + end_angle));
		}

		start_angle = (real_t)(start_angle * M_PI / 180.0f);
		sweep_angle = (real_t)(sweep_angle * M_PI / 180.0f);

		cy = -cy + m_page_height;

		bz.init(cx, cy, rx, ry, start_angle, sweep_angle);

		int count = (int)bz.num_vertices();
		double* vertices = bz.vertices();

		if (count >= 4)
		{
			// flip the y axis
			for (int i = 0; i < count; i += 2)
			{
					vertices[i+1] = -vertices[i + 1] + m_page_height;
			}
			if (anticlockwise)
			{
				do_arc_anticlockwise(count, vertices);
			}
			else
			{
				do_arc_clockwise(count, vertices);
			}
		}
		else
		{
			m_error_type = error_type::invalid_parameter;

			return false;
		}

		m_error_type = error_type::none;

		return true;
	}
public:
	bool arc(real_t cx, real_t cy, real_t radius, real_t start_angle, real_t end_angle)
	{
		if (radius <= 0)
		{
			m_error_type = error_type::range_check;

			return false;
		}
		else
		{
			do_arc(cx, cy, radius, radius, start_angle, end_angle, true);

			m_error_type = error_type::none;

			return true;
		}
	}
	bool arcn(real_t cx, real_t cy, real_t radius, real_t start_angle, real_t end_angle)
	{
		if (radius <= 0)
		{
			m_error_type = error_type::range_check;

			return false;
		}
		else
		{	do_arc(cx, cy, radius, radius, start_angle, end_angle, false);

			m_error_type = error_type::none;
			
			return true;
		}
	}
	bool ellipse(real_t cx, real_t cy, real_t rx, real_t ry)
	{
		if (rx <= 0 || ry <= 0)
		{
			m_error_type = error_type::range_check;

			return false;
		}
		else
		{
			do_arc(cx, cy, rx, ry, 0, 360, true);

			m_error_type = error_type::none;

			return true;
		}
	}
	bool rmoveto(real_t x, real_t y)
	{
		const pointf pt = m_gstate.currentpoint();

		x = pt.x + x;
		y = pt.y + y;

		return moveto(x, y);
	}
	bool rlineto(real_t x, real_t y)
	{
		const pointf pt = m_gstate.currentpoint();

		x = pt.x + x;
		y = pt.y + y;

		return lineto(x, y);
	}
	bool rcurveto(real_t x1, real_t y1, real_t x2, real_t y2, real_t x3, real_t y3)
	{
		const pointf pt = m_gstate.currentpoint();

		x1 = pt.x + x1;
		y1 = pt.y + y1;

		x2 = pt.x + x2;
		y2 = pt.y + y2;

		x3 = pt.x + x3;
		y3 = pt.y + y3;

		return curveto(x1, y1, x2, y2, x3, y3);
	}
	bool moveto(real_t x, real_t y)
	{

		m_gstate.transform_point(x, y);

		if (m_path_data.moveto(x, y))
		{
			pointf pt{ x, y };

			m_gstate.set_currentpoint(pt);
			m_gstate.set_last_moveto(pt);
			m_gstate.has_currentpoint( true );

			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::out_of_memory;

		return false;
	}
	bool lineto(real_t x, real_t y)
	{
		
		m_gstate.transform_point(x, y);

		if (m_path_data.lineto(x, y))
		{
			pointf pt{ x, y };

			m_gstate.set_currentpoint( pt );

			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::out_of_memory;

		return false;
	}
	bool rectangle(real_t x, real_t y, real_t width, real_t height)
	{		
		pointf pt{ x, y };
		real_t x2 = x + width;
		real_t y2 = y + height;

		m_gstate.transform_point(x, y);
		m_gstate.transform_point(x2, y2);

		width = x2 - x;
		height = y2 - y;

		if (m_path_data.rect(x, y, width, height))
		{
			return moveto(pt.x, pt.y);
		}
		else
		{
			m_error_type = error_type::out_of_memory;

			return false;
		}
	}
private:
	bool _rectangle(real_t x, real_t y, real_t width, real_t height, bool do_stroke)
	{
		try
		{
			bool result = true;

			save_path();

			newpath();

			if (rectangle(x, y, width, height))
			{
				m_stream << "q\n";

				if (do_stroke)
				{
					stroke();
				}
				else
				{
					fill();
				}
				m_stream << "Q\n";
				m_error_type = error_type::none;
			}
			else
			{
				m_error_type = error_type::out_of_memory;

				result = false;
			}

			restore_path();

			return result;
		}
		catch (...)
		{
			m_error_type = error_type::out_of_memory;

			return false;
		}
	}
public:
	bool rectstroke(real_t x1, real_t y1, real_t width, real_t height)
	{
		return _rectangle(x1, y1, width, height, true);
	}
	bool rectfill(real_t x1, real_t y1, real_t width, real_t height)
	{
		return _rectangle(x1, y1, width, height, false);
	}
	bool curveto(real_t x1, real_t y1, real_t x2, real_t y2, real_t x3, real_t y3)
	{

		m_gstate.transform_point(x1, y1);
		m_gstate.transform_point(x2, y2);
		m_gstate.transform_point(x3, y3);

		if (m_path_data.curveto(x1, y1, x2, y2, x3, y3))
		{
			pointf pt{ x3, y3 };

			m_gstate.set_currentpoint( pt );

			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::out_of_memory;

		return false;
	}
	void newpath()
	{
		m_path_data.newpath();
		m_gstate.has_currentpoint( false );

		m_error_type = error_type::none;
	}
	void closepath()
	{
		pointf pt = m_gstate.last_moveto();

		m_path_data.closepath();

		m_gstate.set_currentpoint(pt);

		m_error_type = error_type::none;
	}
	void transform_distance(real_t& x, real_t& y)
	{
		m_gstate.transform_distance(x, y);

		m_error_type = error_type::none;
	}
	void transform_distance(pointf& pt)
	{
		m_gstate.transform_distance(pt);

		m_error_type = error_type::none;
	}
	void transform_point(real_t& x, real_t& y)
	{
		m_gstate.transform_point(x, y);

		m_error_type = error_type::none;
	}
	void transform_point(pointf& pt)
	{
		m_gstate.transform_point(pt);

		m_error_type = error_type::none;
	}
	void itransform_point(pointf& pt)
	{
		m_gstate.itransform_point(pt);

		m_error_type = error_type::none;
	}
	void itransform_point(real_t &x, real_t &y)
	{
		m_gstate.itransform_point(x, y);

		m_error_type = error_type::none;
	}
	void stroke()
	{
		matrix ctm = m_gstate.currentmatrix();

		if (ctm.sx != 0 || ctm.sy != 0)
		{
			m_stream << "q\n";
			m_gstate.write_clip(m_stream);
			m_gstate.on_stroke(m_stream, ctm);
			m_path_data.write(m_stream, "S", ctm);
			m_stream << "Q\n";
		}
		newpath();

		m_error_type = error_type::none;
	}
	void fill()
	{
		matrix ctm = m_gstate.currentmatrix();

		m_stream << "q\n";
		m_gstate.write_clip(m_stream);
		m_gstate.on_fill(m_stream);
		m_path_data.write(m_stream, "f", ctm);
		m_stream << "Q\n";
		newpath();

		m_error_type = error_type::none;
	}
	void eofill()
	{
		matrix ctm = m_gstate.currentmatrix();

		m_stream << "q\n";
		m_gstate.write_clip(m_stream);
		m_gstate.on_fill(m_stream);
		m_path_data.write(m_stream, "f*", ctm);
		m_stream << "Q\n";
		newpath();

		m_error_type = error_type::none;
	}
	void fill_and_stroke()
	{
		matrix ctm = m_gstate.currentmatrix();

		if (ctm.sx != 0 || ctm.sy != 0)
		{
			m_stream << "q\n";
			m_gstate.write_clip(m_stream);
			m_gstate.on_fill(m_stream);
			m_gstate.on_stroke(m_stream, ctm);
			m_path_data.write(m_stream, "B", ctm);
			m_stream << "Q\n";
		}
		newpath();

		m_error_type = error_type::none;
	}
	void eofill_and_stroke()
	{
		matrix ctm = m_gstate.currentmatrix();

		if (ctm.sx != 0 || ctm.sy != 0)
		{
			m_stream << "q\n";
			m_gstate.on_fill(m_stream);
			m_gstate.on_stroke(m_stream, ctm);
			m_path_data.write(m_stream, "B*", ctm);
			m_stream << "Q\n";
		}
		newpath();

		m_error_type = error_type::none;
	}
	void stringwidth(const byte_t* char_codes, size_t count, real_t& width, real_t& height)
	{
		if (char_codes && count > 0)
		{
			_stringwidth(char_codes, count, width, height);
		}
	}
	void stringwidth(const char* ansi_text, real_t& width, real_t& height)
	{
		if (ansi_text)
		{
			size_t len = strlen(ansi_text);

			_stringwidth((byte_t*)ansi_text, len, width, height);
		}
	}
	bool show(const byte_t* char_codes, size_t count)
	{
		if (!char_codes || 0 == count)
		{
			m_error_type = error_type::invalid_parameter;

			return false;
		}
		else
		{
			const pointf& pt = m_path_data.last_point();

			return write_text(pt.x, pt.y, char_codes, count);
		}
	}
	bool show(real_t x, real_t y, const byte_t* char_codes, size_t count)
	{
		if (!char_codes || 0 == count )
		{
			m_error_type = error_type::invalid_parameter;

			return false;
		}
		else
		{
			return write_text(x, y, char_codes, count);
		}
	}
	bool show(real_t x, real_t y, const char* ansi_text)
	{
		if (!ansi_text)
		{
			m_error_type = error_type::invalid_parameter;

			return false;
		}
		else
		{
			return write_text(x, y, (byte_t*)ansi_text, strlen(ansi_text));
		}
	}
	bool show(const char* ansi_text)
	{
		if (!ansi_text)
		{
			m_error_type = error_type::invalid_parameter;

			return false;
		}
		else
		{
			pointf pt = m_gstate.currentpoint();

			return write_text(pt.x, pt.y, (byte_t*)ansi_text, strlen(ansi_text));
		}
	}
	bool gsave()
	{
		try
		{
			m_graphics_stack.push(m_gstate);

			m_gstate.clear_clipping_path_stack();

			m_path_stack.push(m_path_data);

			m_error_type = error_type::none;

			return true;
		}
		catch (std::exception &ex)
		{
			m_error_message = ex.what();

			m_error_type = error_type::out_of_memory;

			return false;
		}
	}
	void grestore()
	{
		if (!m_graphics_stack.empty())
		{
			m_gstate = m_graphics_stack.top();

			m_graphics_stack.pop();

		}

		if (!m_path_stack.empty())
		{
			m_path_data = m_path_stack.top();

			m_path_stack.pop();
		}

		m_error_type = error_type::none;
	}
	void grestoreall()
	{
		if (!m_graphics_stack.empty())
		{
			// pop the stack and use the bottomnost item as the current item
			while (m_graphics_stack.size() > 1)
			{
				m_graphics_stack.pop();
			}

			m_gstate = m_graphics_stack.top();

			m_graphics_stack.pop();

		}

		if (!m_path_stack.empty())
		{
			while (m_path_stack.size() > 1)
			{
				m_path_stack.pop();
			}
			m_path_data = m_path_stack.top();

			m_path_stack.pop();
		}

		m_error_type = error_type::none;
	}
	bool charpath(const byte_t* char_codes, size_t len)
	{
		if (!char_codes || 0 == len)
		{
			m_error_type = error_type::invalid_parameter;

			return false;
		}
		else
		{
			HDC hdc = GetDC(nullptr);
			UINT point_count;
			byte_vector point_types;
			std::vector<POINT> points;
			SIZE sz;

			SaveDC(hdc);
			SetMapMode(hdc, MM_TWIPS);
			SelectObject(hdc, m_gstate.font()->m_hfont);

			SetBkMode(hdc, TRANSPARENT);
			SetTextAlign(hdc, TA_BASELINE);
			GetTextExtentPoint32A(hdc, (char*)char_codes, (int)len, & sz);
			BeginPath(hdc);
			TextOutA(hdc, 0, 0, (char *) char_codes,  (int)len);
			EndPath(hdc);

			point_count = GetPath(hdc, nullptr, nullptr, 0);

			if (point_count > 0)
			{
				UINT size_read;
				point_types.resize(point_count);
				points.resize(point_count);

				size_read = GetPath(hdc, points.data(), point_types.data(), point_count);

				if (size_read)
				{
					const byte_t* types = point_types.data();
					const POINT* pts = points.data();
					real_t emsquare = m_gstate.font()->m_em_square; //todo
					matrix font_mtx = m_gstate.font()->m_matrix;	//todo
					pointf curpoint = m_gstate.currentpoint();
					matrix ctm = m_gstate.m_ctm;
					pointf endpoint;

					real_t scale = (real_t)(1.0f / emsquare);


					font_mtx.tx += curpoint.x;
					font_mtx.ty += curpoint.y;

					font_mtx.scale(scale, scale);
					
					endpoint.x = (real_t)sz.cx;

					font_mtx.transform_point(endpoint);

					endpoint.x = curpoint.x + endpoint.x;
					endpoint.y = curpoint.y;

					pointf pt1, pt2, pt3;

					for (UINT i = 0; i < size_read; ++i)
					{
						switch (types[i])
						{
						case PT_MOVETO:
							pt1.x = (real_t)pts[i].x;
							pt1.y = (real_t)pts[i].y;
							font_mtx.transform_point(pt1.x, pt1.y);
							moveto(pt1.x, pt1.y);
							break;
						case PT_LINETO:
						case PT_LINETO | PT_CLOSEFIGURE:
							pt1.x = (real_t)pts[i].x;
							pt1.y = (real_t)pts[i].y;
							font_mtx.transform_point(pt1.x, pt1.y);
							lineto(pt1.x, pt1.y);
							if (types[i] & PT_CLOSEFIGURE)
							{
								closepath();
							}
							break;
						case PT_BEZIERTO:
						case PT_BEZIERTO | PT_CLOSEFIGURE:
							pt1.x = (real_t)pts[i].x;
							pt1.y = (real_t)pts[i].y;
							font_mtx.transform_point(pt1.x, pt1.y);
							pt2.x = (real_t)pts[i+1].x;
							pt2.y = (real_t)pts[i+1].y;
							font_mtx.transform_point(pt2.x, pt2.y);
							pt3.x = (real_t)pts[i+2].x;
							pt3.y = (real_t)pts[i+2].y;
							font_mtx.transform_point(pt3.x, pt3.y);
							curveto(pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y);

							i += 2;
							if (types[i] & PT_CLOSEFIGURE)
							{
								closepath();
							}
							break;
						}
					}

					moveto(endpoint.x, endpoint.y);
				}
			}
			RestoreDC(hdc, -1);

			m_error_type = error_type::none;
			
			return true;
		}
	}
	bool charpath(const char* ansi_text)
	{
		if (!ansi_text)
		{
			m_error_type = error_type::invalid_parameter;

			return false;
		}
		else
		{
			size_t len = strlen(ansi_text);

			return charpath((byte_t*)ansi_text, len);
		}
	}
	bool image(const char* filename, real_t x, real_t y, real_t width, real_t height)
	{
		int32_t obj_number = m_doc.find_image(filename);

		if (obj_number != NOTFOUND)
		{
			matrix mtx(width, 0, 0, height, x, y);
			matrix ctm = m_gstate.currentmatrix();

			m_stream << "q\n";

			ctm.write(m_stream, "cm");
			mtx.write(m_stream, "cm");

			m_stream << "/Im" << obj_number << " Do\n";

			m_stream << "Q\n";

			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::file_open_failed;

		return false;
	}
	byte_t currentrenderingmode()
	{
		m_error_type = error_type::none;

		return m_gstate.rendering_mode();
	}
	bool setrenderingmode(byte_t rendering_mode)
	{
		if (m_gstate.rendering_mode(rendering_mode))
		{
			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::range_check;

		return true;
	}
	real_t currentmiterlimit()
	{
		m_error_type = error_type::none;

		return m_gstate.miterlimit();
	}
	bool setmiterlimit(real_t value) 
	{
		if (value >= 1.0f)
		{
			m_gstate.miterlimit(value);

			m_error_type = error_type::none;

			return true;
		}
		else
		{
			m_error_type = error_type::range_check;

			return false;
		}
	}

private:
	void currentrgb(const solid_color& clr, real_t& r, real_t& g, real_t& b)
	{
		if (clr.m_type != color_type::cmyk)
		{
			r = clr.m_rgb.r;
			g = clr.m_rgb.g;
			b = clr.m_rgb.b;
		}
		else
		{
			r = 1.0f - min(1.0f, clr.m_cmyk.c + clr.m_cmyk.k);
			g = 1.0f - min(1.0f, clr.m_cmyk.m + clr.m_cmyk.k);
			b = 1.0f - min(1.0f, clr.m_cmyk.y + clr.m_cmyk.k);
		}

		m_error_type = error_type::none;
	}
public:
	void currentstrokergb(real_t& r, real_t& g, real_t& b)
	{
		const solid_color& clr = m_gstate.m_stroke_color;

		currentrgb(clr, r, g, b);

		m_error_type = error_type::none;
	}
	void currentfillrgb(real_t& r, real_t& g, real_t& b)
	{
		const solid_color& clr = m_gstate.m_fill_color;

		currentrgb(clr, r, g, b);

		m_error_type = error_type::none;
	}
	void currentcmyk(const solid_color& clr, real_t& c, real_t& m, real_t& y, real_t &k)
	{
		if (clr.m_type == color_type::cmyk)
		{
			c = clr.m_cmyk.c;
			m = clr.m_cmyk.m;
			y = clr.m_cmyk.y;
			k = clr.m_cmyk.k;
		}
		else if( clr.m_type == color_type::rgb)
		{
			c = (1.0f - clr.m_rgb.r);

			m = (1.0f - clr.m_rgb.g);

			y = (1.0f - clr.m_rgb.b);

			k = 0;
		}
		else
		{
			k = (1.0f - clr.m_rgb.r);
		}
		m_error_type = error_type::none;
	}
	void currentstrokecmyk(real_t& c, real_t& m, real_t& y, real_t& k)
	{
		const solid_color& clr = m_gstate.m_stroke_color;

		currentcmyk(clr, c, m, y, k);

		m_error_type = error_type::none;
	}
	void currentfillcmyk(real_t& c, real_t& m, real_t& y, real_t& k)
	{
		const solid_color& clr = m_gstate.m_fill_color;

		currentcmyk(clr, c, m, y, k);

		m_error_type = error_type::none;
	}
	real_t currentgray(const solid_color& clr)
	{
		if (clr.m_type == color_type::cmyk)
		{
			return 1.0f - min(1.0f, (0.3f * clr.m_cmyk.c) + (0.59f * clr.m_cmyk.m) + (0.11f * clr.m_cmyk.y) + clr.m_cmyk.k);
		}
		else if (clr.m_type == color_type::rgb)
		{
			return ((0.3f * clr.m_rgb.r) + (0.59f * clr.m_rgb.g) + (0.11f * clr.m_rgb.b));
		}
		else
		{
			return clr.m_rgb.r;
		}
		m_error_type = error_type::none;
	}
	real_t currentstrokegray()
	{
		const solid_color& clr = m_gstate.m_stroke_color;

		m_error_type = error_type::none;

		return currentgray(clr);
	}
	real_t currentfillgray()
	{
		const solid_color& clr = m_gstate.m_fill_color;

		m_error_type = error_type::none;

		return currentgray(clr);
	}
	void initclip()
	{
		
		path_data& path = m_gstate.m_clipping_path;

		m_gstate.m_clipping_path.rect(0, 0, m_page_width, m_page_height);
				
		m_error_type = error_type::none;
	}
	void clippath()
	{
		path_data& clip_path = m_gstate.m_clipping_path;

		if (clip_path.size() >= 3)
		{
			const point_data *data = clip_path.get_data().data();
			size_t size = clip_path.size();

			for (size_t i = 0; i < size; ++i)
			{
				const point_data& d = data[i];

				switch (d.type)
				{
				case pt_moveto:
					moveto(d.x, d.y);
					break;
				case pt_lineto:
					lineto(d.x, d.y);
					break;
				case pt_lineto | pt_closepath:
					lineto(d.x, d.y);
					closepath();
					break;
				case pt_curveto:
				case pt_curveto | pt_closepath:
					{
						const auto p2 = data[i + 1];
						const auto p3 = data[i + 2];

						curveto(d.x, d.y, p2.x, p2.y, p3.x, p3.y);

						if (d.type & pt_closepath)
						{
							closepath();
						}
						i += 2;
					}
					break;
				case pt_rect:
					{
						const auto p2 = data[i + 1];
						rectangle(d.x, d.y, p2.x, p2.y);
						i++;
					}
					break;
				}
			}
		}
		m_error_type = error_type::none;
	}
	bool clipsave()
	{
		if( m_gstate.clipsave())
		{
			m_error_type = error_type::none;

			return true;
		}
	
		m_error_type = error_type::out_of_memory;

		return false;
	}
	void cliprestore()
	{
		m_gstate.cliprestore();

		m_error_type = error_type::none;
	}
	bool setdash(std::vector<real_t>& _array, real_t phase)
	{
		if (m_gstate.setdash(_array, phase))
		{
			m_error_type = error_type::none;

			return true;
		}

		m_error_type = error_type::range_check;

		return false;
	}
	void currentdash(std::vector<real_t>& _array, real_t phase)
	{
		m_gstate.currentdash(_array, phase);
	
		m_error_type = error_type::none;
	}
	void flattenpath()
	{
		m_path_data.flatten(true);
	}
	void do_clip(bool even_odd)
	{
		if (m_path_data.size() > 2) // must be a polygon
		{			
			m_gstate.m_clipping_path.append(m_path_data);
		}
	}
	void clip()
	{
		do_clip(false);

		m_gstate.m_clip_type = clip_type::nonzero;
	}
	void eoclip()
	{
		do_clip(true);
		m_gstate.m_clip_type = clip_type::evenodd;
	}
	error_type get_error() const
	{
		return m_error_type;
	}
	std::string get_error_message() const
	{
		return m_error_message;
	}
};