#pragma once
//������ ���� �������� ����� "��������" ��������
#include "modules.h"
#include "pilot.h"
#include "projectile.h"

namespace rplanes
{
	enum Nation { ALLIES, AXIS };

	namespace serverdata
	{
		//"�������" �������
		class Plane
		{
		public:
			//types
			class PlanePosition
			{
			public:
				PlanePosition()
				{
					x=0;
					y=0;
					angle = 0;
					roll = 0;
				}
				float x, y, angle,roll;
			};
			// ���������, ������� �������� ������������ �����.
			class StaticalParameters 
			{
			public:
				float maxPower,
					Vmax,
					Vmin,
					Gmax,
					mobilityFactor,
					surface,
					rollSpeed,
					mass;
			};
			// ���������, �������������� �������
			class ControllableParameters 
			{
			public:
				ControllableParameters()
				{
					power = 0;
					shootingDistanceOffset = 0.f;
					turningVal = 0;
					isShooting = false;
					launchMissile = false;
					missileAim = false;
				}
				//� ��������� �� 0 �� 100
				short power;
				//� ������
				short shootingDistanceOffset;
				//� ��������� �� -100 �� 100
				short turningVal; 
				bool isShooting;
				bool launchMissile;
				//�������� ������
				bool missileAim;		
			};
			// ���������, ������������� ������ �� ����������� ���������� � ������� ������, � �������� ����-���� �� ���������.
			class DependentParameters 
			{
			public:
				float frictionFactor,
					minPower;
			};
			// ������������� ���������. �������� ������ ��������� ��������������. ������ ����������� � ������������� ���������� ������� �� ������� ����������.
			// ������������� ������ ����.
			class InterimParameters 
			{				
			public:
				float additionalPower,
					summaryPower,
					frictionForce,
					engineForce,
					maneuverFrictionForce,
					force,
					maxAngularVelocity,
					maxShootingDistane,
					maxMissileDistance,
					shootingDistance,
					ACS;
				InterimParameters()
				{
					additionalPower = 0.f;
					summaryPower = 0.f;
					frictionForce = 0.f;
					engineForce = 0.f;
					maneuverFrictionForce = 0.f;
					force = 0.f;
					maxAngularVelocity = 0.f;
					maxShootingDistane = 0.f;
					maxMissileDistance = 0.f;
					shootingDistance = 0.f;
					ACS = 0.f;
				}
			};
			//���������� ���� ��������. �������� �������� ,��������� ��������� � ������. ������������� ������ ����.
			class TargetParameters 
			{
			public:
				TargetParameters()
				{
					V = 500;
					acceleration = 0;
					angularAcceleration = 0;
					angularVelocity = 0;
					faintVal = 0;
					faintTimer = -1;
				}
				float V,					// ��������
					acceleration,			// ������� ��������
					angularVelocity,		// ������� ��������
					angularAcceleration,	// ������� ������� ��������
					faintVal,				// ������� ���������� ������ �� ���������� ��� �������� > 1 - �������
					faintTimer,				// ���������� ����� ���������� � ��������
					clientShootingDistance,	// ��������� ��������, ������������ � �������
					aimSize;				// ������ ���������� �����
			};

			//data
			std::vector<  planedata::Module * > modules;
			planedata:: Gun * bestGun;
			planedata::Missile * currentMissile;

			std::string name;
			//��������
			playerdata::Pilot pilot;
			Nation nation;
			planedata:: Cabine cabine;
			planedata:: Framework framework;
			planedata:: Tail tail;
			std::vector< planedata::Gun> guns;
			std::vector< planedata::Missile> missiles;
			std::vector< planedata::Ammunition> ammunitions;
			std::vector< planedata::Engine> engines;
			std::vector< planedata::Tank> tanks;
			std::vector< planedata::Wing> wings;
			std::vector< planedata::Turret> turrets;

			//���������
			//����� ��������
			StaticalParameters statical;
			DependentParameters dependent;
			//����� ��������
			PlanePosition position;
			ControllableParameters controllable;
			InterimParameters interim;
			TargetParameters target;

			//functions


			bool isFilled()const;
			//������������� �������/��������/��������� �������/lastHtiClient � ��������� ��������. ����� ������ ����� � ������� ������
			void reload( size_t clientID );
			//�������� ��� �����������, ���������� ������� ���������� �� ������. 
			void initModules(); 
			void showParams();
			//����������� ��� ���������  ���������� �������
			void updateStatical();
			//����������� ��� ��������� ���������� �������
			void updateDependent();

			//����������� ����� move ������ ����
			void updateInterim();
			//����������� ������ ����
			void move( float frameTime );

			//����������� ��� �������� ��������
			void updateCurrentMissile();

			//�������� �������� �� �������� ������ � ������� � ������� frameTime ���� ����� �� � ��������. ID ���� �� ���������������
			std::vector<Bullet> shoot( float frameTime, size_t clientID, float serverTime);

			std::vector< rplanes::serverdata::LaunchedMissile > launchMissile(size_t clientID);
		};
	}
}