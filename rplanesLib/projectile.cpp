#include "projectile.h"
#include "configuration.h"
#define _USE_MATH_DEFINES
#include < math.h >
namespace rplanes
{
	namespace serverdata
	{
		void Projectile::move( float frameTime )
		{
			prevX = x;
			prevY = y;
			prevZ = z;
			speedZ -= 10.0 * frameTime * gravity_;
			z += speedZ * frameTime;
			speedXY += acceleration * frameTime;
			distance += speedXY*frameTime;
			x += speedXY * std::cos( angleXY / 180.0 * M_PI )*frameTime;
			y += speedXY * std::sin( angleXY / 180.0 * M_PI )*frameTime;
		}
		void LaunchedMissile::move( float frameTime )
		{
			Projectile::move(frameTime);
			TTL -= frameTime;
		}

		LaunchedMissile::LaunchedMissile()
		{
			gravity_ = configuration().missile.gravity;
		}

		int Bullet::getCurrentDamage()const
		{
			return damage * speedXY / startSpeed;
		}

		float Bullet::getCurrentPenetration()const
		{
			return penetration * speedXY / startSpeed;
		}

		bool Bullet::isSpent()const
		{
			return speedXY/startSpeed < configuration().shooting.ttlFactor;
		}

		Bullet::Bullet()
		{
			virgin = true;
			gravity_ = configuration().shooting.gravity;
		}

		bool Bullet::isVirgin() const
		{
			return virgin;
		}

	}
}
