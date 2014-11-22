#include <hitzone.h>
#define _USE_MATH_DEFINES
#include < math.h >
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
namespace rplanes
{
	namespace serverdata
	{
		PointXY HitZone::getCenter()
		{
			if ( shape.points.size() == 0 )
			{
				return PointXY();
			}
			PointXY center;
			for( auto & i: shape.points )
			{
				center.x += i.x;
				center.y += i.y;
			};
			center.y /= shape.points.size();
			center.x /= shape.points.size();
			return center;
		}
		PointXY  HitZone::getSize()
		{
			if ( shape.points.size() == 0 )
			{
				return PointXY();
			}
			PointXY max( shape.points.back() ), min( shape.points.back() );
			for( auto i : shape.points )
			{
				if ( i.x > max.x )
				{
					max.x = i.x;
				}
				if ( i.y > max.y )
				{
					max.y = i.y;
				}
				if ( i.x < min.x )
				{
					min.x = i.x;
				}
				if ( i.y < min.y )
				{
					min.y = i.y;
				}
			}
			return PointXY( max.x - min.x, max.y - min.y );
		}

		void HitZone::spin( float angle, float Roll )
		{
			while( Roll > 360.f  )
				Roll -= 360.f;
			while( Roll < 0 )
				Roll += 360.f;
			if( Roll > 90.f && Roll < 270 )
				Roll -=180.f;
			shape = base_;
			sf::Vector2f center(0.f, 0.f); //  в плоскости xz
			sf::Vector2f newCenter( 0.f, 0.f);
			//определение ширины модуля
			PointXY widthRange( std::numeric_limits<float>::max() , -std::numeric_limits<float>::max());
			for( auto & i: shape.points )
			{
				center.x += i.x;
				if( i.x > widthRange.y)
					widthRange.y = i.x;
				if( i.x < widthRange.x )
					widthRange.x = i.x;
			};
			center.x /= shape.points.size();
			center.y = ( shape.heightRange.a + shape.heightRange.b ) /2.f;

			newCenter = sf::Transform().rotate( Roll, sf::Vector2f(0.f, 0.f) ).transformPoint( center );
			float Width_ = ( widthRange.y - widthRange.x) ;
			float Height_ = (shape.heightRange.b - shape.heightRange.a);
			for (auto i = shape.points.begin(); i!=shape.points.end(); i++)
			{
				i->x -= center.x;
				if ( std::abs(i->x * std::cos(Roll / 180 * M_PI) )  < Height_ / 2 && std::cos( Roll / 180 * M_PI ) < 0.9 )
				{
					i->x *= Height_/Width_;
				} 
				else
				{
					i->x *= std::cos( Roll / 180 * M_PI );
				}
				i->x += newCenter.x;			
			}

			widthRange = PointXY( std::numeric_limits<float>::max() , - std::numeric_limits<float>::max());
			for( auto & i: shape.points )
			{
				if( i.x > widthRange.y)
					widthRange.y = i.x;
				if( i.x < widthRange.x )
					widthRange.x = i.x;
			};
			float newWidth_ = ( widthRange.y - widthRange.x);
			shape.heightRange.a -= center.y;
			shape.heightRange.a *= Width_/newWidth_;
			shape.heightRange.a += newCenter.y;

			shape.heightRange.b -= center.y;
			shape.heightRange.b *= Width_/newWidth_;
			shape.heightRange.b += newCenter.y;
			Height_ = shape.heightRange.a - shape.heightRange.b;
			for( auto i = shape.points.begin(); i != shape.points.end(); i++ )
			{
				sf::Vector2f p(i->x, i->y);
				p = sf::Transform().rotate( angle - 90 , sf::Vector2f(0.f, 0.f) ).transformPoint( p );
				i->x = p.x;
				i->y = p.y;
			}
		}
	}
}
