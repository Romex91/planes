#pragma once
#include "stdafx.h"

namespace rplanes
{

	namespace configurationvalues//���� odb. (odb �� ������������ ��������� �������)
	{
		//������������ ��� �������� ����(��� ��������)  plane::shoot
#pragma db object no_id
		class Shooting
		{
		public:
			float shootRateFactor,
				accuracyFactor,	
				damageFactor,
				speedFactor,			//������������ ��� �� � ������� gun
				accelerationFactor,		//������������ ��� �� � ������� gun
				ttlFactor,				// ���������� �������� ����, ��� ������� ���� ������������ �� [0.f, 1.f] ������������ � bullet::isSpent � gun::getMaxDistance
				penetFactor,
				impactFactor,
				angleImpactFactor,
				impactRandomnesFactor,
				maxDistance,			//������������ � plane::updateInterim
				minDistance,			//������������ � plane::updateInterim � plane::shoot
				gravity;	// ���������� ��������� ���������� ������� ���� ������������ � ������������ bullet
			Shooting();
		};

#pragma db object no_id
		class Turrets
		{
		public:
			float shootRateFactor,
				accuracyFactor,
				impactFactor,
				angleImpactFactor,
				impactRandomnesFactor,
				gunLength,
				aimTimerMean,
				aimTimerSigma,
				aimErrorSigma,
				aimIntense,
				aimExp,
				shootTimeMean,
				shootTimeSigma,
				cooldownTimeMean,
				cooldownTimeSigma;
			Turrets();
		};

		//������������ ��� �������� ����� (��� ��������) * �� �����������
#pragma db object no_id
		class Missile
		{
		public:
			float 
				radiusFactor,
				speedFactor,
				damageFactor,
				accelerationFactor,
				speedAccuracyFactor,
				accuracyFactor,
				ttlFactor,
				gravity;	// ���������� ��������� ���������� ������� ������ ������������ � ������������ launchedMissile
		};
#pragma db object no_id
		class Defect
		{
		public:
			float 
				chanceFactor, //������������ � module::damage
				
				//////////////////////////////////////////////////////////////////////////
				// ������������ � �������� ������� � ��������� defect
				shootRateFactor,
				surfaceFactor,
				mobilityFactor,
				vMaxFactor,
				tMaxFactor,
				//////////////////////////////////////////////////////////////////////////
				pilotSkillsFactor; //������������ � �������� ������
			Defect();
		};

		//������������ ��� �������� ������ �������
#pragma db object
		class Profile
		{
		public:
			int 
				startExp,
				startMoney,
				//���� ���������� �� ��������� �� ����������. ���������� �� ������ ������ � ��������
				hitExperience,
				//������� �� ��������� ����� ����������
				damageReward,
				//����� �� ����������� ��������
				damagePenalty,
				maxBanlistSize;
			std::vector<std::string> startMaps;
			std::vector<std::string> startPlanes;
			Profile();
		private:
			#pragma db id
			size_t id;
			friend odb::access;
		};
		//������������ � �������� ������ plane
#pragma db object no_id
		class Flight
		{
		public:
			float
				forceFactor,					//������� �������/����������
				speedFactor,					//����������� �������������� ��������
				angleVelocityFactor,			//����������� ������������ ��������
				maneuverFrictionFactor,			//����������� ������ ��� ��������

				maxAngularVelocityFactor,		//����������� ������������� ���������. ����������� ������������� ��������� �� ������ ���������, ��� �� ����������� ����������
				angleAccelerationFactor,		//����������� �������������
				reverseAngleAccelerationFactor, //����������� �������� �������� � ������ �����
				turningExp;						//�������� ���������� ������� �������� ��� ������ ��������
			Flight();
		};
#pragma db object no_id
		class Engine
		{
		public:
			float
				//����������� ����������� ������ �� ����� �� 1 ������� ��� ������ ������ �����������.
				overheatDefectChanceFactor,
				//���������� ��������� ���������, ��������� � ������� ��� ���������
				overheatDamageFactor,			
				//���������� ������� ������� ������� ������ �� ������/����������
				additionalPowerFactor;			
			Engine();
		};
#pragma db object no_id
		class Pilot
		{
		public:
			float
				pilotFaintTime,					//�����, ���������� � ��������
				pilotMaxG;						//������������ ����������, ������������� ������� ��������
			Pilot();
		};
#pragma db object no_id
		class Collisions
		{
		public:
			float
				bulletDeflectionSigma,
				randomRicochetChance,
				rammingDistance,
				visibilityDistance;

			Collisions();
		};
#pragma db object no_id
		class Server
		{
		public:

			float spawnCooldownTime;

			unsigned short port;
			size_t numThreads;

			size_t roomMessagesPerFrame;
			float roomFrameTime;

			size_t hangarMessagesPerFrame;
			float hangarFrameTime;

			float unloginedDisconnectTime;
			size_t maxClientsNumber;

			Server();
		};

#pragma db object no_id
		class Bots
		{
		public:
			size_t hardStartExpiriance;
			
			float
				rammingWarningDistance;

			float
				easyFleeTimeMean,
				easyFleeTimeSigma,
				easyPatrolScanSpeed,

				easyTargerSelectionTimeSigma,
				easyTargetSelectionTimeMean,
				easyAttackSector,
				easyShootingDistanceAgility,
				easyMaxAttakersCount,

				easyPanicChance,
				easyPanicTimeMean,
				easyPanicTimeSigma;
			Bots();
		};

	}
	class Configuration
	{
	public:
		Configuration()
		{};
		Configuration( std::string fileName );

		configurationvalues::Shooting shooting;
		configurationvalues::Defect defect;
		configurationvalues::Missile missile;
		configurationvalues::Profile profile;
		configurationvalues::Flight flight;
		configurationvalues::Pilot pilot;
		configurationvalues::Engine engine;
		configurationvalues::Server server;
		configurationvalues::Collisions collisions;
		configurationvalues::Bots bots;
		configurationvalues::Turrets turrets;
	};
	Configuration & configuration();
}
