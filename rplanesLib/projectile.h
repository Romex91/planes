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
			//id �������
			size_t planeID;
			float
				//������� �������� � �������������� ���������
				speedXY,	
				//��������� �������� ����
				startSpeed,	
				//������� �������� �� ���������
				speedZ,	
				//���������
				acceleration,	
				x,				
				y,
				z,
				angleXY,
				//���������� ������� ���� �� x
				prevX,
				//���������� ������� ���� �� y
				prevY,
				//���������� ������� ���� �� z
				prevZ,
				//����������, ���������� ��������
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
			//����� ������
			size_t gunNo;
			//��������� ����
			int damage;			
			//��������� �������������
			float penetration;	
			unsigned short caliber;
			int getCurrentDamage()const;
			float getCurrentPenetration()const;
			bool isSpent()const;
			Bullet();
			//�������� �� ���� � �������. ��������� �����
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