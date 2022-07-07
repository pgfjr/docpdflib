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

struct point_data
{
	real_t x{ 0.0f };
	real_t y{ 0.0f };
	byte_t type{ pt_moveto };
	point_data() = default;
	point_data(const point_data& ci) = default;
	~point_data() = default;
};

class path_data
{
	std::vector<point_data> m_data;
	bool push(real_t x, real_t y, byte_t type)
	{
		try
		{
			point_data t{ x, y, type };

			m_data.push_back(t);

			return true;
		}
		catch (...)
		{
			return false;
		}
	}

public:
	path_data() : m_data()
	{
		point_data tmp;

		m_data.reserve(16);
		// implicit moveto
		m_data.push_back(tmp);
	}
	path_data(const path_data& ci) : m_data()
	{
		m_data = ci.m_data;
	}
	~path_data()
	{
		m_data.clear();
	}
	path_data& operator=(const path_data& src)
	{
		m_data.clear();

		m_data = src.m_data;

		return *this;
	}
	std::vector<point_data>& get_data()
	{
		return m_data;
	}
	size_t size() const
	{
		return m_data.size();
	}
	// use clear only when DESTROYING the path; otherwise, use NEWPATH()
	void clear()
	{
		m_data.clear();
	}
	bool moveto(real_t x, real_t y)
	{
		if (size() != 1)
		{
			return push(x, y, pt_moveto);
		}
		else
		{
			// replace the current moveto
			m_data[0].x = x;
			m_data[0].y = y;

			return true;
		}
	}
	bool moveto(const pointf& pt)
	{
		return moveto(pt.x, pt.y);
	}
	bool lineto(real_t x, real_t y)
	{
		return push(x, y, pt_lineto);
	}
	bool lineto(const pointf& pt)
	{
		return push(pt.x, pt.y, pt_lineto);
	}
	bool curveto(real_t x1, real_t y1, real_t x2, real_t y2, real_t x3, real_t y3)
	{
		if (!push(x1, y1, pt_curveto))
		{
			return false;
		}
		else if (!push(x2, y2, pt_curveto))
		{
			// remove the last push
			m_data.pop_back();

			return false;
		}
		else if (!push(x3, y3, pt_curveto))
		{
			// remove the last 2 pushes
			m_data.pop_back();
			m_data.pop_back();

			return false;
		}
		return true;
	}
	bool curveto(const pointf &pt1, const pointf& pt2, const pointf& pt3 )
	{
		return curveto(pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y);
	}
	bool rect(real_t x1, real_t y1, real_t width, real_t height)
	{
		if (!push(x1, y1, pt_rect))
		{
			return false;
		}
		else if (!push(width, height, pt_rect))
		{
			// remove the last push
			m_data.pop_back();

			return false;
		}
		return true;
	}

	void newpath()
	{
		// leave the first moveto
		m_data.resize(1);
		m_data[0].x = m_data[0].y = 0;
	}

	void closepath()
	{
		size_t count = size();

		if (count > 1)
		{
			byte_t type = m_data[count - 1].type;

			//enable the high bit
			if (pt_lineto == type)
			{
				m_data[count - 1].type ^= pt_closepath;
			}
			else if (pt_curveto == type)
			{
				m_data[count - 1].type ^= pt_closepath;
				m_data[count - 2].type ^= pt_closepath;
				m_data[count - 3].type ^= pt_closepath;
			}
		}
	}
	pointf first_point()
	{
		pointf pt;

		pt.x = m_data[0].x;
		pt.y = m_data[0].y;

		return pt;
	}
	pointf last_point() 
	{
		pointf pt;
		size_t count = size();

		if (count > 0)
		{
			pt.x = m_data[count - 1].x;
			pt.y = m_data[count - 1].y;
		}
		return pt;
	}
	void to_cartesian(real_t page_height)
	{
		for (point_data& d : m_data)
		{
			d.y = -d.y + page_height;
		}
	}
	void to_screen(real_t page_height)
	{
		for (point_data& d : m_data)
		{
			d.y = -d.y + page_height;
		}
	}
	void append(const path_data& src)
	{
		m_data.insert(m_data.end(), src.m_data.begin(), src.m_data.end());
	}
	void transform(matrix& mtx)
	{
		for (auto& v : m_data)
		{
			mtx.transform_point(v.x, v.y);
		}
	}
	void rescale(real_t x, real_t y, point_data& d)
	{
		if (0 == x)
		{
			d.x = 0;
		}
		else
		{
			d.x /= x;
		}
		
		if (0 == y)
		{
			d.y = 0;
		}
		else
		{
			d.y /= y;
		}
	}
	void rescale(real_t x, real_t y, size_t count, point_data* data)
	{
		if (x != 0 && y != 0)
		{
			for (size_t i = 0; i < count; ++i)
			{
				point_data& d = data[i];

				d.x /= x;
				d.y /= y;
			}
		}
		else if (0 == x)
		{
			for (size_t i = 0; i < count; ++i)
			{
				point_data& d = data[i];

				d.x = 0;
				d.y /= y;
			}
		}
		else
		{
			for (size_t i = 0; i < count; ++i)
			{
				point_data& d = data[i];

				d.x /= x;
				d.y = 0;
			}
		}
	}
	void write(std::ostringstream& stream, std::string command, matrix &ctm)
	{
		if (ctm.sx == 0 && ctm.sy == 0)
		{
			return;
		}
		else
		{
			real_t scale = (ctm.sx + ctm.sy) / 2.0f;
			
			size_t count = m_data.size();

			point_data* data = m_data.data();

			ctm.sx /= scale;
			ctm.sy /= scale;
			//ctm.sx = ctm.sy = 1.0f;
			ctm.rx = ctm.ry = ctm.tx = ctm.ty = 0;


			stream << ctm << " cm\n";
				
			stream << std::fixed;

			rescale(ctm.sx, ctm.sy, count, data);

			for (size_t i = 0; i < count; ++i)
			{
				point_data& d = data[i];

				switch (d.type)
				{
				case pt_moveto:
					//exclude any moveto at the end
					if ((i + 1) < count)
					{
						stream << d.x << ' ' << d.y << " m\n";
					}
					break;
				case pt_lineto:
					stream << d.x << ' ' << d.y << " l\n";
					break;
				case pt_lineto | pt_closepath:
					stream << d.x << ' ' << d.y << " l h\n";
					break;
				case pt_curveto:
				case pt_curveto | pt_closepath:
					stream << d.x << ' ' << d.y << ' ';

					d = data[i + 1];
					stream << d.x << ' ' << d.y << ' ';

					d = data[i + 2];
					stream << d.x << ' ' << d.y << ' ';

					if (data[i].type & pt_closepath)
					{
						stream << "c h\n";
					}
					else
					{
						stream << "c\n";
					}
					i += 2;
					break;
				case pt_rect:
					stream << d.x << ' ' << d.y << ' ';

					d = data[i + 1];
					stream << d.x << ' ' << d.y << " re\n";
					i++;
					break;
				}
			}

			stream << command << '\n';
		}
	}
	void write_clip(std::ostringstream& stream, std::string command)
	{
			size_t count = m_data.size();
			point_data* data = m_data.data();

	
			stream << std::fixed;

			for (size_t i = 0; i < count; ++i)
			{
				point_data& d = data[i];

				switch (d.type)
				{
				case pt_moveto:
					stream << d.x << ' ' << d.y << " m\n";
					break;
				case pt_lineto:
					stream << d.x << ' ' << d.y << " l\n";
					break;
				case pt_lineto | pt_closepath:
					stream << d.x << ' ' << d.y << " l h\n";
					break;
				case pt_curveto:
				case pt_curveto | pt_closepath:
					stream << d.x << ' ' << d.y << ' ';

					d = data[i + 1];
				
					stream << d.x << ' ' << d.y << ' ';

					d = data[i + 2];
					
					stream << d.x << ' ' << d.y << ' ';

					if (data[i].type & pt_closepath)
					{
						stream << "c h\n";
					}
					else
					{
						stream << "c\n";
					}
					i += 2;
					break;
				case pt_rect:
					stream << d.x << ' ' << d.y << ' ';

					d = data[i + 1];
					
					stream << d.x << ' ' << d.y << " re\n";
					i++;
					break;
				}
			}

			stream << command << '\n';
	}
	void flatten(bool rescale_back)
	{
			size_t count = m_data.size();

			if (count < 4)
			{
				return;
			}
			else
			{
				matrix mtx;
				point_data* data;
				HDC hdc;
				int mapmode;
				POINT pts[3];
				// convert to TWIPS
				mtx.sx = mtx.sy = 20.0f;

				transform(mtx);

				hdc = GetDC(nullptr);

				mapmode = SetMapMode(hdc, MM_TWIPS);

				data = m_data.data();

				BeginPath(hdc);

				for (size_t i = 0; i < count; ++i)
				{
					point_data& d = data[i];

					switch (d.type)
					{
					case pt_moveto:
						//exclude any moveto at the end
						if ((i + 1) < count)
						{
							MoveToEx(hdc, (int)d.x, (int)d.y, nullptr);
						}
						break;
					case pt_lineto:
					case pt_lineto | pt_closepath:
						LineTo(hdc, (int)d.x, (int)d.y);
						if (d.type & pt_closepath)
							CloseFigure(hdc);
						break;
					case pt_curveto:
					case pt_curveto | pt_closepath:
						pts[0].x = (int)d.x;
						pts[0].y = (int)d.y;
						pts[1].x = (int)data[i + 1].x;
						pts[1].y = (int)data[i + 1].y;
						pts[2].x = (int)data[i + 2].x;
						pts[2].y = (int)data[i + 2].y;
						PolyBezierTo(hdc, pts, 3);
						if (d.type & pt_closepath)
							CloseFigure(hdc);
						i += 2;
						break;
					case pt_rect:
						pts[0].x = (int)d.x;
						pts[0].y = (int)d.y;
						pts[1].x = (int)data[i + 1].x;
						pts[1].y = (int)data[i + 1].y;
						Rectangle(hdc, pts[0].x, pts[0].y + pts[1].y, pts[0].x + pts[1].x, pts[0].y);
						i++;
						break;
					}
				}
				EndPath(hdc);
				FlattenPath(hdc);
				get_flattened_path(hdc, rescale_back);
				SetMapMode(hdc, mapmode);
				ReleaseDC(nullptr, hdc);
			}
	}
	private:
	void get_flattened_path(HDC hdc, bool rescale_back)
	{
		std::vector<POINT> points;
		std::vector<byte_t> point_types;
		UINT point_count;

		point_count = GetPath(hdc, nullptr, nullptr, 0);

		if (point_count > 0)
		{
			UINT size_read;
	
			points.resize(point_count);
			point_types.resize(point_count);

			size_read = GetPath(hdc, points.data(), point_types.data(), point_count);

			newpath();

			if (size_read)
			{
				const byte_t* types = point_types.data();
				const POINT* pts = points.data();				
				pointf pt1;
								
				for (UINT i = 0; i < size_read; ++i)
				{
					switch (types[i])
					{
					case PT_MOVETO:
						pt1.x = (real_t)pts[i].x;
						pt1.y = (real_t)pts[i].y;
						moveto(pt1);
						break;
					case PT_LINETO:
					case PT_LINETO | PT_CLOSEFIGURE:
						pt1.x = (real_t)pts[i].x;
						pt1.y = (real_t)pts[i].y;
						
						lineto(pt1);

						if (types[i] & PT_CLOSEFIGURE)
						{
							closepath();
						}
						break;
					}
				}
				
				if (rescale_back)
				{
					matrix mtx(1.0f / 20.0f, 0, 0, 1.0f / 20.0f, 0, 0);

					transform(mtx);
				}
			}
		}
	}
};

