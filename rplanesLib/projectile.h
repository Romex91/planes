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
			//id стрелка
			size_t planeID;
			float
				//текущая скорость в горизонтальной плоскости
				speedXY,	
				//начальная скорость пули
				startSpeed,	
				//текущая скорость по вертикали
				speedZ,	
				//ускорение
				acceleration,	
				x,				
				y,
				z,
				angleXY,
				//предыдущая позиция пули по x
				prevX,
				//предыдущая позиция пули по y
				prevY,
				//предыдущая позиция пули по z
				prevZ,
				//расстояние, пройденное снарядом
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
			//номер орудия
			size_t gunNo;
			//начальный урон
			int damage;			
			//начальное бронепробитие
			float penetration;	
			unsigned short caliber;
			int getCurrentDamage()const;
			float getCurrentPenetration()const;
			bool isSpent()const;
			Bullet();
			//попадала ли пуля в самолет. Серверный метод
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