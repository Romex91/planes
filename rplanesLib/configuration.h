#pragma once
#include "stdafx.h"

namespace rplanes
{

	namespace configurationvalues//дань odb. (odb не поддерживает вложенных классов)
	{
		//используется при создании пуль(при стрельбе)  plane::shoot
#pragma db object no_id
		class Shooting
		{
		public:
			float shootRateFactor,
				accuracyFactor,	
				damageFactor,
				speedFactor,			//используется так же в методах gun
				accelerationFactor,		//используется так же в методах gun
				ttlFactor,				// определяет скорость пули, при которой пуля уничтожается от [0.f, 1.f] используется в bullet::isSpent и gun::getMaxDistance
				penetFactor,
				impactFactor,
				angleImpactFactor,
				impactRandomnesFactor,
				maxDistance,			//используется в plane::updateInterim
				minDistance,			//используется в plane::updateInterim и plane::shoot
				gravity;	// определяет ускорение свободного падения пули используется в конструкторе bullet
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

		//используется при создании ракет (при стрельбе) * не реализовано
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
				gravity;	// определяет ускорение свободного падения ракеты используется в конструкторе launchedMissile
		};
#pragma db object no_id
		class Defect
		{
		public:
			float 
				chanceFactor, //используется в module::damage
				
				//////////////////////////////////////////////////////////////////////////
				// используются в геттерах модулей с префиксом defect
				shootRateFactor,
				surfaceFactor,
				mobilityFactor,
				vMaxFactor,
				tMaxFactor,
				//////////////////////////////////////////////////////////////////////////
				pilotSkillsFactor; //используется в геттерах пилота
			Defect();
		};

		//используется при создании нового профиля
#pragma db object
		class Profile
		{
		public:
			int 
				startExp,
				startMoney,
				//опыт получаемый за попадание по противнику. Умножается на калибр орудия в квадрате
				hitExperience,
				//награда за нанесение урона противнику
				damageReward,
				//штраф за повреждение союзника
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
		//импользуется в функциях класса plane
#pragma db object no_id
		class Flight
		{
		public:
			float
				forceFactor,					//степень разгона/торможения
				speedFactor,					//коэффициент поступательной скорости
				angleVelocityFactor,			//коэффициент вращательной скорости
				maneuverFrictionFactor,			//коэффициент трения при повороте

				maxAngularVelocityFactor,		//коэффициент маневренности самолетов. Увеличивает маневренность самолетов на низких скоростях, где не учитываются перегрузки
				angleAccelerationFactor,		//коэффициент управляемости
				reverseAngleAccelerationFactor, //коэффициент скорости возврата в прямой полет
				turningExp;						//точность следования угловой скорости при мягком повороте
			Flight();
		};
#pragma db object no_id
		class Engine
		{
		public:
			float
				//коэффициент вероятности выхода из строя за 1 секунду при резком наборе температуры.
				overheatDefectChanceFactor,
				//количество процентов прочности, снимаемое в секунду при перегреве
				overheatDamageFactor,			
				//определяет степень влияния навыков пилота на разгон/торможение
				additionalPowerFactor;			
			Engine();
		};
#pragma db object no_id
		class Pilot
		{
		public:
			float
				pilotFaintTime,					//время, проводимое в обмороке
				pilotMaxG;						//максимальная перегрузка, выдерживаемая пилотом новичком
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
