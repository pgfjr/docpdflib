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


struct matrix
{
	real_t sx{ 1.0f };
	real_t rx{ 0.0f };
	real_t ry{ 0.0f };
	real_t sy{ 1.0f };
	real_t tx{ 0.0f };
	real_t ty{ 0.0f };
	matrix() = default;
	matrix(const matrix&) = default;
	matrix(real_t _sx, real_t _rx, real_t _ry, real_t _sy, real_t _tx, real_t _ty)
	{
		sx = _sx;
		rx = _rx;
		ry = _ry;
		sy = _sy;
		tx = _tx;
		ty = _ty;
	}
	void getmatrix(real_t& _sx, real_t& _rx, real_t& _ry, real_t& _sy, real_t& _tx, real_t& _ty)
	{
		_sx = sx;
		_rx = rx;
		_ry = ry;
		_sy = sy;
		_tx = tx;
		_ty = ty;
	}
	void getmatrix(real_t* values)
	{
		values[0] = sx;
		values[1] = rx;
		values[2] = ry;
		values[3] = sy;
		values[4] = tx;
		values[5] = ty;
	}
	void getmatrix(matrix &mtx)
	{
		mtx.sx = sx;
		mtx.rx = rx;
		mtx.ry = ry;
		mtx.sy = sy;
		mtx.tx = tx;
		mtx.ty = ty;
	}
	void setmatrix(real_t _sx, real_t _rx, real_t _ry, real_t _sy, real_t _tx, real_t _ty)
	{
		sx = _sx;
		rx = _rx;
		ry = _ry;
		sy = _sy;
		tx = _tx;
		ty = _ty;
	}
	void setmatrix(const real_t* values)
	{
		sx = (real_t)values[0];
		rx = (real_t)values[1];
		ry = (real_t)values[2];
		sy = (real_t)values[3];
		tx = (real_t)values[4];
		ty = (real_t)values[5];
	}
	void setmatrix(const matrix &mtx)
	{
		sx = mtx.sx;
		rx = mtx.rx;
		ry = mtx.ry;
		sy = mtx.sy;
		tx = mtx.tx;
		ty = mtx.ty;
	}
	bool is_identity() const
	{
		if (1.0f == sx && 0 == rx && 0 == ry && 1.0f == sy && 0 == tx && 0 == ty)
		{
			return true;
		}
		return false;
	}
	void multiply(const matrix& left)
	{
		const matrix right(*this);

		sx = real_t(left.sx * right.sx + left.rx * right.ry);
		rx = real_t(left.sx * right.rx + left.rx * right.sy);

		ry = real_t(left.ry * right.sx + left.sy * right.ry);
		sy = real_t(left.ry * right.rx + left.sy * right.sy);

		tx = real_t(left.tx * right.sx + left.ty * right.ry + right.tx);
		ty = real_t(left.tx * right.rx + left.ty * right.sy + right.ty);
	}
	void scale(real_t x, real_t y)
	{
		const matrix left(x, 0, 0, y, 0, 0);

		multiply(left);
	}
	void translate(real_t x, real_t y)
	{
		const matrix left(1.0f, 0, 0, 1.0f, x, y);

		multiply(left);
	}
	void rotate(real_t deg)
	{
		matrix left(0, 0, 0, 0, 0, 0);
		real_t s, c, radians;

		radians = (real_t)(deg * M_PI / 180.0f);

		s = (real_t)sin(radians);
		c = (real_t)cos(radians);

		left.sx = left.sy = c;
		left.rx = s;
		left.ry = -s;

		multiply(left);

	}
	void transform_distance(real_t& dx, real_t& dy)
	{
		real_t new_x = real_t(sx * dx + ry * dy);
		real_t new_y = real_t(rx * dx + sy * dy);

		dx = (real_t)new_x;
		dy = (real_t)new_y;
	}
	void transform_point(real_t& x, real_t& y)
	{
		transform_distance(x, y);

		x += tx;
		y += ty;
	}
	void transform_point(pointf &pt)
	{
		transform_point(pt.x, pt.y);
	}
	void transform_points(pointf* pts, size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			transform_point(pts[i].x, pts[i].y);
		}
	}
	bool invert_matrix()
	{
		Gdiplus::Matrix mtx(sx, rx, ry, sy, tx, ty);

		if (mtx.IsInvertible())
		{
			Gdiplus::REAL m[6]{ 0 };

			mtx.Invert();

			mtx.GetElements(m);

			sx = m[0];
			rx = m[1];
			ry = m[2];
			sy = m[3];
			tx = m[4];
			ty = m[5];

			return true;
		}

		return false;
	}
	double determinant_reciprocal() const
	{
		return 1.0 / (sx * sy - ry * rx);
	}	
	void itransform_point(pointf& pt)
	{
		if (!skewed())
		{
			inverse_transform(pt);
		}
		else
		{
			Gdiplus::Matrix mtx(sx, rx, ry, sy, tx, ty);

			if (mtx.IsInvertible())
			{
				mtx.Invert();

				mtx.TransformPoints((Gdiplus::PointF*)&pt, 1);
			}			
		}
	}
	void itransform_point(real_t &x, real_t &y)
	{
		pointf pt{ x, y };

		itransform_point(pt);

		x = pt.x;
		y = pt.y;
	}
	void reset()
	{
		sx = 1.0f;
		rx = 0;
		ry = 0;
		sy = 1.0f;
		tx = 0;
		ty = 0;
	}
	void write(std::ostringstream& stream, std::string command)
	{
		stream << std::fixed;
		stream << sx << ' ' << rx << ' ' << ry << ' ' << sy << ' ' << tx << ' ' << ty << ' ' << command << '\n';
	}
	friend std::ostream& operator<<(std::ostream& os, const matrix &ctm)
	{
		os << std::fixed;
		os << ctm.sx << ' ' << ctm.rx << ' ' << ctm.ry << ' ' << ctm.sy << ' ' << ctm.tx << ' ' << ctm.ty << ' ';

		return os;
	}
	protected:
		void inverse_transform(pointf& pt)
		{
			double d = determinant_reciprocal();
			double a = (pt.x - tx) * d;
			double b = (pt.y - ty) * d;
			pt.x = real_t(a * sy - b * rx);
			pt.y = real_t(b * sx - a * ry);
		}
		bool skewed() const
		{
			return !((rx == 0 && ry == 0) || (sx == 0 && sy == 0));
		}
};
