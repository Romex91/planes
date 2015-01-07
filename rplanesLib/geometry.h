#pragma once
#include "stdafx.h"
namespace rplanes
{

	class PointXY
	{
	public:
		template <typename Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & x;
			ar & y;
		}
		PointXY( float X=0.f, float Y=0.f ):x(X), y(Y)
		{}
		float x,y;
		void print()
		{
			std::cout << "x = " << x << " y = " << y << std::endl;
		}

		inline PointXY operator-() const
		{ return PointXY(-x, -y); }

		inline PointXY operator+() const
		{ return PointXY(+x, +y); }

		inline PointXY operator+(const PointXY& v) const
		{ return PointXY(x+v.x, y+v.y); }

		inline PointXY operator-(const PointXY& v) const
		{ return PointXY(x-v.x, y-v.y); }

		inline PointXY operator*(const float& s) const
		{ return PointXY(x*s, y*s); }

		inline float operator*(const PointXY& v) const
		{ return x*v.x + y*v.y; }

	private:
	};

	class PointXYZ
	{
	public:
		PointXYZ(float X = 0.f, float Y = 0.f, float Z = 0) :x(X), y(Y), z(Z)
		{}

		float x, y, z;
	};
	class Interval
	{
	public:
		template <typename Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & a;
			ar & b;
		}

		Interval()
		{
			a = 0;
			b = 0;
		}
		float a,b;
	};

	template<class A, class B>
	float angleFromPoints(const A& a, const B& b)
	{
		static const float PI = std::atan(1.f) * 4.f;
		return atan2(b.y - a.y, b.x - a.x) / PI * 180;
	}


	template<typename A, typename B, typename C, typename D, typename E >
	bool getLineSegmentsIntersection( A start1, B end1, C start2, D end2, E &out_intersection)
	{
		E dir1;
		E dir2;

		dir1.x = end1.x - start1.x;
		dir1.y = end1.y - start1.y;

		dir2.x = end2.x - start2.x;
		dir2.y = end2.y - start2.y;

		//считаем уравнения прямых проходящих через отрезки
		auto a1 = -dir1.y;
		auto b1 = +dir1.x;
		auto d1 = -(a1*start1.x + b1*start1.y);

		auto a2 = -dir2.y;
		auto b2 = +dir2.x;
		auto d2 = -(a2*start2.x + b2*start2.y);

		//подставляем концы отрезков, для выяснения в каких полуплоскотях они
		auto seg1_line2_start = a2*start1.x + b2*start1.y + d2;
		auto seg1_line2_end = a2*end1.x + b2*end1.y + d2;

		auto seg2_line1_start = a1*start2.x + b1*start2.y + d1;
		auto seg2_line1_end = a1*end2.x + b1*end2.y + d1;

		//если концы одного отрезка имеют один знак, значит он в одной полуплоскости и пересечения нет.
		if (seg1_line2_start * seg1_line2_end >= 0 || seg2_line1_start * seg2_line1_end >= 0) 
			return false;

		auto u = seg1_line2_start / (seg1_line2_start - seg1_line2_end);
		out_intersection.x =  start1.x + u*dir1.x;
		out_intersection.y =  start1.y + u*dir1.y;

		return true;
	}

	template< typename A, typename B >
	float distance( A pointA, B pointB )
	{
		return std::sqrt( (pointA.x - pointB.x)*(pointA.x - pointB.x) + (pointA.y-pointB.y)*(pointA.y-pointB.y) );
	}
}