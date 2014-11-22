#pragma once

#include "stdafx.h"

#include "geometry.h"

namespace rplanes
{
	namespace serverdata
	{
		class HitZone
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & base_;
			}
			class Shape
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & points;
					ar & heightRange;
				}
				std::vector < PointXY > points;
				Interval heightRange;
			};
			Shape shape;
			void spin( float angle, float Roll );
			PointXY getCenter();
			PointXY getSize();
		private:
			Shape base_;
			friend class Plane;
		};
	}
}