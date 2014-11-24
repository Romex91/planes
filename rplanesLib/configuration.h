#pragma once
#include "stdafx.h"

namespace rplanes
{

	namespace configurationvalues
	{
		//используется при создании пуль(при стрельбе)  plane::shoot
		class Shooting
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(shootRateFactor);
				ar & BOOST_SERIALIZATION_NVP(accuracyFactor);
				ar & BOOST_SERIALIZATION_NVP(damageFactor);
				ar & BOOST_SERIALIZATION_NVP(speedFactor);
				ar & BOOST_SERIALIZATION_NVP(accelerationFactor);
				ar & BOOST_SERIALIZATION_NVP(ttlFactor);
				ar & BOOST_SERIALIZATION_NVP(penetFactor);
				ar & BOOST_SERIALIZATION_NVP(impactFactor);
				ar & BOOST_SERIALIZATION_NVP(impactRandomnesFactor);
				ar & BOOST_SERIALIZATION_NVP(angleImpactFactor);
				ar & BOOST_SERIALIZATION_NVP(maxDistance);
				ar & BOOST_SERIALIZATION_NVP(minDistance);
				ar & BOOST_SERIALIZATION_NVP(gravity);
			}

			float shootRateFactor = 1.f,
				accuracyFactor = 1.f,
				damageFactor = 1.f,
				speedFactor = 0.4f,			//используется так же в методах gun
				accelerationFactor = 0.8f,		//используется так же в методах gun
				ttlFactor = 0.5f,				// определяет скорость пули, при которой пуля уничтожается от [0.f, 1.f] используется в bullet::isSpent и gun::getMaxDistance
				penetFactor = 1.f,
				impactFactor = 300.f,
				impactRandomnesFactor = 10.f,
				angleImpactFactor = 300.f,
				maxDistance = 500.f,			//используется в plane::updateInterim
				minDistance = 10.f,			//используется в plane::updateInterim и plane::shoot
				gravity = 1.f;	// определяет ускорение свободного падения пули используется в конструкторе bullet
		};

		class Turrets
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(shootRateFactor);
				ar & BOOST_SERIALIZATION_NVP(accuracyFactor);
				ar & BOOST_SERIALIZATION_NVP(impactFactor);
				ar & BOOST_SERIALIZATION_NVP(angleImpactFactor);
				ar & BOOST_SERIALIZATION_NVP(impactRandomnesFactor);
				ar & BOOST_SERIALIZATION_NVP(gunLength);
				ar & BOOST_SERIALIZATION_NVP(aimTimerMean);
				ar & BOOST_SERIALIZATION_NVP(aimTimerSigma);
				ar & BOOST_SERIALIZATION_NVP(aimErrorSigma);
				ar & BOOST_SERIALIZATION_NVP(aimIntense);
				ar & BOOST_SERIALIZATION_NVP(aimExp);
				ar & BOOST_SERIALIZATION_NVP(shootTimeMean);
				ar & BOOST_SERIALIZATION_NVP(shootTimeSigma);
				ar & BOOST_SERIALIZATION_NVP(cooldownTimeMean);
				ar & BOOST_SERIALIZATION_NVP(cooldownTimeSigma);
			}
			float shootRateFactor = 1.f,
				accuracyFactor = 1.f,
				impactFactor = 1.f,
				angleImpactFactor = 1.f,
				impactRandomnesFactor = 10.f,
				gunLength = 2.f,
				aimTimerMean = 1.f,
				aimTimerSigma = 0.5f,
				aimErrorSigma = 0.5f,
				aimIntense = 5.f,
				aimExp = 0.3f,
				shootTimeMean = 2.5f,
				shootTimeSigma = 2.f,
				cooldownTimeMean = 4.0f,
				cooldownTimeSigma = 2.0f;
		};

		//используется при создании ракет (при стрельбе) * не реализовано
		class Missile
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(radiusFactor);
				ar & BOOST_SERIALIZATION_NVP(speedFactor);
				ar & BOOST_SERIALIZATION_NVP(damageFactor);
				ar & BOOST_SERIALIZATION_NVP(accelerationFactor);
				ar & BOOST_SERIALIZATION_NVP(speedAccuracyFactor);
				ar & BOOST_SERIALIZATION_NVP(accuracyFactor);
				ar & BOOST_SERIALIZATION_NVP(ttlFactor);
				ar & BOOST_SERIALIZATION_NVP(gravity);
			}
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
		class Defect
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(chanceFactor);
				ar & BOOST_SERIALIZATION_NVP(shootRateFactor);
				ar & BOOST_SERIALIZATION_NVP(surfaceFactor);
				ar & BOOST_SERIALIZATION_NVP(mobilityFactor);
				ar & BOOST_SERIALIZATION_NVP(vMaxFactor);
				ar & BOOST_SERIALIZATION_NVP(tMaxFactor);
				ar & BOOST_SERIALIZATION_NVP(pilotSkillsFactor);
			}
			float
				chanceFactor = 1.f, //используется в module::damage
				shootRateFactor = 0.5f,
				surfaceFactor = 0.5f,
				mobilityFactor = 0.5f,
				vMaxFactor = 0.7f,
				tMaxFactor = 2.f,
				pilotSkillsFactor = 0.5f; //используется в геттерах пилота
		};


		//используется при создании нового профиля
		class Profile
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(startExp);
				ar & BOOST_SERIALIZATION_NVP(startMoney);
				ar & BOOST_SERIALIZATION_NVP(hitExperience);
				ar & BOOST_SERIALIZATION_NVP(damageReward);
				ar & BOOST_SERIALIZATION_NVP(damagePenalty);
				ar & BOOST_SERIALIZATION_NVP(maxBanlistSize);
				ar & BOOST_SERIALIZATION_NVP(startMaps);
				ar & BOOST_SERIALIZATION_NVP(startPlanes);
			}
			int
				startExp = 100,
				startMoney = 10000000,
				//опыт получаемый за попадание по противнику. Умножается на калибр орудия в квадрате
				hitExperience = 1,
				//награда за нанесение урона противнику
				damageReward = 1,
				//штраф за повреждение союзника
				damagePenalty = 10,
				maxBanlistSize = 500;
			std::vector<std::string> startMaps;
			std::vector<std::string> startPlanes;
			Profile();
		};
		//импользуется в функциях класса plane

		class Flight
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(forceFactor);
				ar & BOOST_SERIALIZATION_NVP(speedFactor);
				ar & BOOST_SERIALIZATION_NVP(angleVelocityFactor);
				ar & BOOST_SERIALIZATION_NVP(maneuverFrictionFactor);
				ar & BOOST_SERIALIZATION_NVP(maxAngularVelocityFactor);
				ar & BOOST_SERIALIZATION_NVP(angleAccelerationFactor);
				ar & BOOST_SERIALIZATION_NVP(reverseAngleAccelerationFactor);
				ar & BOOST_SERIALIZATION_NVP(turningExp);
			}

			float
				forceFactor = 1.f,					//степень разгона/торможения
				speedFactor = 0.3f,					//коэффициент поступательной скорости
				angleVelocityFactor = 1.f,			//коэффициент вращательной скорости
				maneuverFrictionFactor = 1.f,			//коэффициент трения при повороте

				maxAngularVelocityFactor = 1.f,		//коэффициент маневренности самолетов. Увеличивает маневренность самолетов на низких скоростях, где не учитываются перегрузки
				angleAccelerationFactor = 1.f,		//коэффициент управляемости
				reverseAngleAccelerationFactor = 1.f, //коэффициент скорости возврата в прямой полет
				turningExp = 0.5f;						//точность следования угловой скорости при мягком повороте
		};


		class Engine
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(overheatDefectChanceFactor);
				ar & BOOST_SERIALIZATION_NVP(overheatDamageFactor);
				ar & BOOST_SERIALIZATION_NVP(additionalPowerFactor);
			}

			float
				//коэффициент вероятности выхода из строя за 1 секунду при резком наборе температуры.
				overheatDefectChanceFactor = 1.f,
				//количество процентов прочности, снимаемое в секунду при перегреве
				overheatDamageFactor = 20.f,
				//определяет степень влияния навыков пилота на разгон/торможение
				additionalPowerFactor = 1.f;
		};

		class Pilot
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(pilotFaintTime);
				ar & BOOST_SERIALIZATION_NVP(pilotMaxG);
			}

			float
				pilotFaintTime = 10.f,					//время, проводимое в обмороке
				pilotMaxG = 5.f;						//максимальная перегрузка, выдерживаемая пилотом новичком
		};

		class Collisions
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(bulletDeflectionSigma);
				ar & BOOST_SERIALIZATION_NVP(randomRicochetChance);
				ar & BOOST_SERIALIZATION_NVP(rammingDistance);
				ar & BOOST_SERIALIZATION_NVP(visibilityDistance);
			}

			float
				bulletDeflectionSigma = 10.f,
				randomRicochetChance = 0.1f,
				rammingDistance = 50.f,
				visibilityDistance = 2000.f;
		};


		class Server
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(spawnCooldownTime);
				ar & BOOST_SERIALIZATION_NVP(port);
				ar & BOOST_SERIALIZATION_NVP(numThreads);
				ar & BOOST_SERIALIZATION_NVP(roomMessagesPerFrame);
				ar & BOOST_SERIALIZATION_NVP(roomFrameTime);
				ar & BOOST_SERIALIZATION_NVP(hangarMessagesPerFrame);
				ar & BOOST_SERIALIZATION_NVP(hangarFrameTime);
				ar & BOOST_SERIALIZATION_NVP(unloginedDisconnectTime);
				ar & BOOST_SERIALIZATION_NVP(maxClientsNumber);
			}

			float spawnCooldownTime = 1.5f;

			unsigned short port = 40000;
			size_t numThreads = 4;

			size_t roomMessagesPerFrame = 16;
			float roomFrameTime = 0.03f;

			size_t hangarMessagesPerFrame = 2;
			float hangarFrameTime = 0.1f;

			float unloginedDisconnectTime = 2.f;
			size_t maxClientsNumber = 1000;
		};



		class Bots
		{
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int){
				ar & BOOST_SERIALIZATION_NVP(hardStartExpiriance);
				ar & BOOST_SERIALIZATION_NVP(rammingWarningDistance);
				ar & BOOST_SERIALIZATION_NVP(easyFleeTimeMean);
				ar & BOOST_SERIALIZATION_NVP(easyFleeTimeSigma);
				ar & BOOST_SERIALIZATION_NVP(easyPatrolScanSpeed);
				ar & BOOST_SERIALIZATION_NVP(easyTargerSelectionTimeSigma);
				ar & BOOST_SERIALIZATION_NVP(easyTargetSelectionTimeMean);
				ar & BOOST_SERIALIZATION_NVP(easyAttackSector);
				ar & BOOST_SERIALIZATION_NVP(easyShootingDistanceAgility);
				ar & BOOST_SERIALIZATION_NVP(easyMaxAttakersCount);
				ar & BOOST_SERIALIZATION_NVP(easyPanicChance);
				ar & BOOST_SERIALIZATION_NVP(easyPanicTimeMean);
				ar & BOOST_SERIALIZATION_NVP(easyPanicTimeSigma);
			}

			int hardStartExpiriance = 4000000;

			float
				rammingWarningDistance = 150.f;

			float
				easyFleeTimeMean = 2.0f,
				easyFleeTimeSigma = 1.3f,
				easyPatrolScanSpeed = 20.f,

				easyTargerSelectionTimeSigma = 4.f,
				easyTargetSelectionTimeMean = 5.f,
				easyAttackSector = 4.f,
				easyShootingDistanceAgility = 10.f,
				easyMaxAttakersCount = 2.f,

				easyPanicChance = 0.1f,
				easyPanicTimeMean = 20.f,
				easyPanicTimeSigma = 15.f;
		};

	}


	class Configuration
	{
	public:
		template<class Archive>
		void serialize(Archive & ar, const unsigned int)
		{
			ar & BOOST_SERIALIZATION_NVP(shooting);
			ar & BOOST_SERIALIZATION_NVP(defect);
			ar & BOOST_SERIALIZATION_NVP(missile);
			ar & BOOST_SERIALIZATION_NVP(profile);
			ar & BOOST_SERIALIZATION_NVP(flight);
			ar & BOOST_SERIALIZATION_NVP(pilot);
			ar & BOOST_SERIALIZATION_NVP(engine);
			ar & BOOST_SERIALIZATION_NVP(server);
			ar & BOOST_SERIALIZATION_NVP(collisions);
			ar & BOOST_SERIALIZATION_NVP(bots);
			ar & BOOST_SERIALIZATION_NVP(turrets);
		}

		void load(const std::string & path);
		void save(const std::string & path);

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
