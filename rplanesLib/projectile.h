#pragma once
#include "stdafx.h"
namespace rplanes
{
	namespace serverdata
	{
		class Projectile
		{
		protected:
			float gravity_;
		public:
			Projectile():distance(0){}
			size_t ID;
			size_t planeID;
			float
				speedXY,	
				startSpeed,	
				speedZ,	
				acceleration,	
				x,				
				y,
				z,
				angleXY,
				prevX,
				prevY,
				prevZ,
				distance;		
			virtual void move( float frameTime );
		};
		class LaunchedMissile: public Projectile
		{
		public:
			int damage;
			float radius,
				TTL,
				startTTL;
			std::string model;
			void move( float frameTime );
			LaunchedMissile();
		};
		class Bullet: public Projectile
		{
		public:
			//I have no idea why I added this member. 
			//May be to provide a client ability to draw a shot from a specific gun when creating new bullet
			size_t gunNo;
			int damage;			
			float penetration;	
			unsigned short caliber;
			int getCurrentDamage()const;
			float getCurrentPenetration()const;
			bool isSpent()const;
			Bullet();
			
			//check if the bullet ever hitted any plane
			//a joke from the past. haha
			bool isVirgin()const;

			void rape()
			{
				virgin = false;
			}
		private:
			bool virgin;
		};
	}
}