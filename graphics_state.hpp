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
#include "path_data.hpp"

enum class color_type
{
	rgb,
	cmyk,
	gray
};


struct rgb_color
{
	real_t r{ 0.0f };
	real_t g{ 0.0f };
	real_t b{ 0.0f };
	real_t unused{ 1.0f };
};

struct cmyk_color
{
	real_t c{ 0.0f };
	real_t m{ 0.0f };
	real_t y{ 0.0f };
	real_t k{ 1.0f };
};

struct solid_color
{
	union
	{
		rgb_color m_rgb{0.0f};
		cmyk_color m_cmyk;
	};
	color_type m_type{ color_type::rgb };
	solid_color() = default;
	solid_color(const solid_color &) = default;
	~solid_color() = default;
	void reset()
	{
		m_rgb.r = m_rgb.g = m_rgb.b = m_rgb.unused = 0;
		m_type = color_type::rgb;
	}
};

struct dash_pattern
{
	std::vector<real_t> m_array;
	real_t m_phase{0};
	dash_pattern() : m_array() {}
	dash_pattern(const dash_pattern& ci) : m_array(ci.m_array), m_phase(ci.m_phase){}
	~dash_pattern() = default;
	dash_pattern& operator=(const dash_pattern& obj)
	{
		m_array.clear();
		m_array = obj.m_array;
		m_phase = obj.m_phase;

		return *this;
	}
	void clear()
	{
		m_array.clear();
		m_phase = 0;
	}
	bool is_default()
	{
		return (0 == m_phase) && (m_array.size() == 0);
	}
	void set_value(std::vector<real_t>& _array, real_t phase)
	{
		m_array.clear();
		m_array = _array;
		m_phase = phase;
	}
	void get_value(std::vector<real_t>& _array, real_t &phase)
	{
		_array = m_array;
		phase = m_phase;
	}
	friend std::ostream& operator<<(std::ostream& os, const dash_pattern& dash)
	{
		size_t size = dash.m_array.size();
		size_t i = 0;

		os << std::fixed;

		os << '[';

		for (auto v : dash.m_array)
		{
			os << v;

			if ((i + 1) < size)
			{
				os << ' ';
			}
			++i;
		}
		os << "] " << dash.m_phase;

		return os;
	}
};

enum class clip_type
{
	none,
	nonzero,
	evenodd
};

struct graphics_state
{
	solid_color m_stroke_color;
	solid_color m_fill_color;
	real_t m_opacity{ 1.0f };
	real_t m_linewidth{ 1.0f };
	bool m_fill_non_zero_winding_rule{ true };
	pointf m_currentpoint{ 0,0 };
	bool m_has_currentpoint{ false };
	byte_t m_linejoin{ 0 };
	byte_t m_linecap{ 0 };
	font_record* m_font{ nullptr };

	real_t m_flatness{ 0.0f };
	matrix m_ctm;
	pointf m_last_moveto{ 0,0 };
	byte_t m_rendering_mode{ 0 };
	real_t m_miterlimit{ 10.0f };
	path_data m_clipping_path;
	dash_pattern m_dash_pattern;
	std::stack<path_data> m_clipping_path_stack;
	clip_type m_clip_type{ clip_type::none };
	graphics_state() : m_ctm(), m_stroke_color(), m_fill_color(), m_clipping_path(), m_dash_pattern(), m_clipping_path_stack()
	{}
	void reset()
	{
		m_stroke_color.reset();
		m_fill_color.reset();
		m_opacity = 1.0f;
		m_linewidth = 1.0f;
		m_fill_non_zero_winding_rule = true;
		m_currentpoint.x = m_currentpoint.y = 0;
		m_last_moveto = m_currentpoint;
		m_has_currentpoint = false;
		m_linejoin = 0;
		m_linecap = 0;
		m_flatness = 0.0f;
		m_ctm.reset();
		m_rendering_mode = 0;
		m_miterlimit = 10.0f;
		m_clipping_path.newpath();
		m_dash_pattern.clear();
		
		clear_clipping_path_stack();

		m_clip_type = clip_type::none;
	}
	void copy(const graphics_state& ci)
	{
		m_stroke_color = ci.m_stroke_color;
		m_fill_color = ci.m_fill_color;
		m_opacity = ci.m_opacity;
		m_linewidth = ci.m_linewidth;
		m_fill_non_zero_winding_rule = ci.m_fill_non_zero_winding_rule;
		m_currentpoint = ci.m_currentpoint;
		m_has_currentpoint = ci.m_has_currentpoint;
		m_linejoin = ci.m_linejoin;
		m_linecap = ci.m_linecap;
		m_font = (font_record*)ci.m_font;
		m_flatness = ci.m_flatness;
		m_ctm = ci.m_ctm;
		m_last_moveto = ci.m_last_moveto;
		m_rendering_mode = ci.m_rendering_mode;
		m_miterlimit = ci.m_miterlimit;
		m_clipping_path = ci.m_clipping_path;
		m_dash_pattern = ci.m_dash_pattern;
		m_clipping_path_stack = ci.m_clipping_path_stack;
		m_clip_type = ci.m_clip_type;
	}
	graphics_state(const graphics_state& ci)
	{
		copy(ci);
	}
	graphics_state& operator=(const graphics_state& ci)
	{
		copy(ci);

		return *this;
	}
	void clear_clipping_path_stack()
	{
		while (!m_clipping_path_stack.empty())
		{
			m_clipping_path_stack.pop();
		}
	}
	bool clipsave()
	{
		try
		{
			m_clipping_path_stack.push(m_clipping_path);

			return true;
		}
		catch (...)
		{
			return false;
		}
	}
	void cliprestore()
	{
		if (!m_clipping_path_stack.empty())
		{
			m_clipping_path = m_clipping_path_stack.top();
			m_clipping_path_stack.pop();
		}
	}
	void write_clip(std::ostringstream& stream)
	{
		if (m_clip_type != clip_type::none)
		{
			std::string command = (m_clip_type == clip_type::nonzero) ? "W n" : "W* n";

			m_clipping_path.write_clip(stream, command);
		}
	}
	void on_stroke(std::ostringstream& str, const matrix &ctm)
	{
		real_t scale = (ctm.sx + ctm.sy)/2.0f;

			

		// linecap J
		// linejoin j
		// flatness i

			if (m_linejoin != 0)
			{
				str << (short)m_linejoin << " j\n";
			}
			if (m_linecap != 0)
			{
				str << (short)m_linecap << " J\n";
			}
			if (!m_dash_pattern.is_default())
			{
				str << m_dash_pattern << " d\n";
			}
			switch (m_stroke_color.m_type)
			{
			case color_type::rgb:
				str << m_stroke_color.m_rgb.r << ' ' << m_stroke_color.m_rgb.g << ' ' << m_stroke_color.m_rgb.b << " RG\n";
				break;
			case color_type::cmyk:
				str << m_stroke_color.m_cmyk.c << ' ' << m_stroke_color.m_cmyk.m << ' ' << m_stroke_color.m_cmyk.y << ' ' << m_stroke_color.m_cmyk.k << " K\n";
				break;
			default:
				str << m_stroke_color.m_rgb.r << " G\n";
				break;
			}
			
			str << (m_linewidth * scale) << " w\n";

	}

	void on_fill(std::ostringstream& str)
	{

			switch (m_fill_color.m_type)
			{
			case color_type::rgb:
				str << m_fill_color.m_rgb.r << ' ' << m_fill_color.m_rgb.g << ' ' << m_fill_color.m_rgb.b << " rg\n";
				break;
			case color_type::cmyk:
				str << m_fill_color.m_cmyk.c << ' ' << m_fill_color.m_cmyk.m << ' ' << m_fill_color.m_cmyk.y << ' ' << m_fill_color.m_cmyk.k << " k\n";
				break;
			default:
				str << m_fill_color.m_rgb.r << " g\n";
				break;
			}
	}	
	font_record* font()
	{
		return m_font;
	}
	void font(font_record*fnt)
	{
		m_font = fnt;
	}
	void linewidth(real_t value)
	{
		m_linewidth = value;
	}
	real_t linewidth() const
	{
		return m_linewidth;
	}
	pointf currentpoint() 
	{
		pointf tmp = m_currentpoint;

		m_ctm.itransform_point(  tmp );

		return tmp;
	}
	void currentpoint(real_t &x, real_t &y)
	{
		pointf tmp = currentpoint();

		x = tmp.x;
		y = tmp.y;
	}
	void currentpoint(pointf &pt)
	{
		pt = currentpoint();
	}
	void set_currentpoint(const pointf& pt)
	{
		m_currentpoint.x = pt.x;
		m_currentpoint.y = pt.y;
	}
	void set_currentpoint(real_t x, real_t y)
	{
		m_currentpoint.x = x;
		m_currentpoint.y = y;
	}
	void set_last_moveto(const pointf& pt)
	{
		m_last_moveto.x = pt.x;
		m_last_moveto.y = pt.y;
	}
	pointf last_moveto() const
	{
		return m_last_moveto;
	}
	bool has_currentpoint() const
	{
		return m_has_currentpoint;
	}
	void has_currentpoint( bool value)
	{
		m_has_currentpoint = value;

		if (!value)
		{
			pointf pt{ 0, 0 };

			set_currentpoint(pt);
			set_last_moveto(pt);
		}
	}
	bool linecap(byte_t value)
	{
		if (value < 3)
		{
			m_linecap = value;

			return true;
		}

		return false;
	}
	byte_t linecap() const
	{
		return m_linecap;
	}
	bool linejoin(byte_t value)
	{
		if (value < 3)
		{
			m_linejoin = value;

			return true;
		}

		return false;
	}
	byte_t linejoin() const
	{
		return m_linejoin;
	}
	real_t miterlimit() const
	{
		return m_miterlimit;
	}
	void miterlimit(real_t value)
	{
		if (value >= 1.0f)
		{
			m_miterlimit = value;
		}
	}
	matrix currentmatrix() const
	{
		return m_ctm;
	}
	void currentmatrix(real_t &sx, real_t& rx, real_t& ry, real_t& sy, real_t& tx, real_t& ty)
	{
		m_ctm.getmatrix(sx, rx, ry, sy, tx, ty);
	}
	void setmatrix(real_t sx, real_t rx, real_t ry, real_t sy, real_t tx, real_t ty)
	{
		m_ctm.setmatrix(sx, rx, ry, sy, tx, ty);
	}
	void setmatrix(const matrix &mtx)
	{
		m_ctm.setmatrix(mtx);
	}	
	void transform(const matrix &mtx)
	{
		m_ctm.multiply(mtx);
	}
	void transform(real_t sx, real_t rx, real_t ry, real_t sy, real_t tx, real_t ty)
	{
		matrix mtx(sx, rx, ry, sy, tx, ty);

		m_ctm.multiply(mtx);
	}
	void stroke_cmyk(real_t c, real_t m, real_t y, real_t k)
	{
		set_cmyk_color(m_stroke_color, c, m, y, k);
	}
	void fill_cmyk(real_t c, real_t m, real_t y, real_t k)
	{
		set_cmyk_color(m_fill_color, c, m, y, k);
	}
	void setcmykcolor(real_t c, real_t m, real_t y, real_t k)
	{
		set_cmyk_color(m_stroke_color, c, m, y, k);

		m_fill_color = m_stroke_color;
	}
	void fill_rgb(real_t r, real_t g, real_t b)
	{
		set_rgb_color(m_fill_color, r, g, b);
	}
	void fill_rgb(const rgb_color &fill)
	{
		set_rgb_color(m_fill_color, fill.r, fill.g, fill.b);
	}
	
	void gray(real_t value)
	{
			rgb_color& rgb = m_stroke_color.m_rgb;

			color_range(value);

			rgb.r = value;
			rgb.g = value;
			rgb.b = value;
			rgb.unused = 1.0f;

			m_stroke_color.m_type = color_type::gray;

			m_fill_color = m_stroke_color;		
	}
	void stroke_rgb(real_t r, real_t g, real_t b)
	{
		set_rgb_color(m_stroke_color, r, g, b);
	}
	void stroke_rgb(const rgb_color& stroke)
	{
		set_rgb_color(m_stroke_color, stroke.r, stroke.g, stroke.b);
	}
	void setrgbcolor(real_t r, real_t g, real_t b)
	{
		stroke_rgb(r, g, b);
			
		m_fill_color = m_stroke_color;
	}
	real_t getflat() const
	{
		return m_flatness;
	}
	void setflat(real_t value)
	{
		if (value >= .2f && value <= 100.0f)
		{
			m_flatness = value;
		}
		else if (value < .2f)
		{
			m_flatness = .2f;
		}
		else //if (value > 100.0f)
		{
			m_flatness = 100.0f;
		}
	}
	void rotate(real_t degrees)
	{
		m_ctm.rotate(degrees);
	}
	void scale(real_t x, real_t y)
	{
		m_ctm.scale(x, y);
	}
	void translate(real_t x, real_t y)
	{
		m_ctm.translate(x, y);
	}
	void transform_distance(real_t& x, real_t& y)
	{
		m_ctm.transform_distance(x, y);
	}
	void transform_distance(pointf& pt)
	{
		m_ctm.transform_distance(pt.x, pt.y);
	}
	void transform_point(real_t& x, real_t& y)
	{
		m_ctm.transform_point(x, y);
	}
	void transform_point(pointf &pt)
	{
		m_ctm.transform_point(pt.x, pt.y);
	}
	void itransform_point(pointf& pt)
	{
		m_ctm.itransform_point(pt);
	}
	void itransform_point(real_t &x, real_t &y)
	{
		m_ctm.itransform_point(x, y);
	}
	byte_t rendering_mode() const
	{
		return m_rendering_mode;
	}
	bool rendering_mode(byte_t rendering_mode)
	{
		if (rendering_mode >= 0 && rendering_mode <= 7)
		{
			m_rendering_mode = rendering_mode;

			return true;
		}
		return false;
	}
	bool setdash(std::vector<real_t>& _array, real_t phase)
	{
		size_t size = _array.size();

		if (size > 0)
		{
			size_t zero = 0;

			for (auto v : _array)
			{
				if (0 == v)
					++zero;
				// cannot be negative
				else if (v < 0)
				{
					return false;
				}
			}
			// cannot be all zeros
			if (zero == size)
			{
				return false;
			}
		}

		m_dash_pattern.set_value(_array, phase);

		return true;
	}
	void currentdash(std::vector<real_t>& _array, real_t& phase)
	{
		m_dash_pattern.get_value(_array, phase);
	}
private:
	real_t color_range(real_t &v)
	{
		if (v < 0.0f)
		{
			v = 0.0f;
		}
		else if (v > 1.0f)
		{
			v = 1.0f;
		}
		return v;
	}
	void set_rgb_color(solid_color& color, real_t r, real_t g, real_t b)
	{
		rgb_color& rgb = color.m_rgb;

		color_range(r);
		color_range(g);
		color_range(b);
		
		rgb.r = r;
		rgb.g = g;
		rgb.b = b;
		rgb.unused = 1.0f;

		color.m_type = color_type::rgb;		
	}
	void set_cmyk_color(solid_color& color, real_t c, real_t m, real_t y, real_t k)
	{
			cmyk_color& cmyk = color.m_cmyk;

			color_range(c);
			color_range(m);
			color_range(y);
			color_range(k);

			cmyk.c = c;
			cmyk.m = m;
			cmyk.y = y;
			cmyk.k = k;

			color.m_type = color_type::cmyk;
	}	
};
