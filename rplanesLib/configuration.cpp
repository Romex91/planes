#include "configuration.h"
#include "odb/configuration-odb.hxx"
#include "database.h"
#include "helpers.h"

namespace rplanes
{


	Configuration::Configuration( std::string fileName )
	{
		try
		{
			auto confDB = loadDatabase(fileName);
			odb::transaction t( confDB->begin() );

#define LOAD_VALUE(x, t)\
			{\
			odb::result<configurationvalues::t> all( confDB->query<configurationvalues::t>() );\
			for ( auto& Res: all )\
			{\
				x = Res;\
			}\
			}
			LOAD_VALUE(collisions, Collisions);
			LOAD_VALUE(defect, Defect);
			LOAD_VALUE(engine, Engine);
			LOAD_VALUE(flight, Flight);
			LOAD_VALUE(missile, Missile);
			LOAD_VALUE(pilot, Pilot);
			LOAD_VALUE(profile, Profile);
			LOAD_VALUE(server, Server);
			LOAD_VALUE(shooting, Shooting);
			LOAD_VALUE(bots, Bots);
#undef LOAD_VALUE
			t.commit();
			std::cout << "Loaded configuration" << std::endl;
		}
		catch (...)
		{
			std::cout << "Configuration not found. Default values will be used." << std::endl;
		}
	}

	Configuration & configuration()
	{
		static Configuration conf("config.db");
		return conf;
	}


	configurationvalues::Shooting::Shooting()
	{
		shootRateFactor=1;
		accuracyFactor=1;	
		damageFactor=1;
		speedFactor=0.4f;				//используется так же в методах gun
		accelerationFactor=0.8f;		//используется так же в методах gun
		ttlFactor=0.5f;				
		penetFactor=1;
		impactFactor=300.f;
		impactRandomnesFactor = 10.f;
		angleImpactFactor = 300.f;
		maxDistance=500.f;			//используется в plane::updateInterim
		minDistance=10;			//используется в plane::updateInterim и plane::shoot
		gravity=1.f;
	}


	configurationvalues::Defect::Defect()
	{
		chanceFactor=1; //используется в module::damage
		//////////////////////////////////////////////////////////////////////////
		// используются в геттерах модулей с префиксом defect
		shootRateFactor=0.5;
		surfaceFactor=0.5;
		mobilityFactor=0.5;
		vMaxFactor=2.f;
		tMaxFactor=0.5;
		//////////////////////////////////////////////////////////////////////////
		pilotSkillsFactor=0.5;
	}


	configurationvalues::Profile::Profile()
	{
		startExp = 100;
		startMoney = 10000000;
		hitExperience = 1;
		damageReward = 1;
		damagePenalty = 10;
		maxBanlistSize = 500;

		startPlanes.push_back("me262");
		startPlanes.push_back("mig9");
		startMaps.push_back("attack.map");
		startMaps.push_back("bot.map");
	}


	configurationvalues::Flight::Flight()
	{
		forceFactor=1;					//степень разгона/торможения
		speedFactor=0.3f;					//коэффициент поступательной скорости
		angleVelocityFactor=1;			//коэффициент вращательной скорости
		maneuverFrictionFactor=1;			//коэффициент трения при повороте

		maxAngularVelocityFactor=1;		//коэффициент маневренности самолетов. Увеличивает маневренность самолетов на низких скоростях, где не учитываются перегрузки
		angleAccelerationFactor=1;		//коэффициент управляемости
		reverseAngleAccelerationFactor=1; //коэффициент скорости возврата в прямой полет
		turningExp = 0.5f;						//точность следования угловой скорости при мягком повороте
	}


	configurationvalues::Engine::Engine()
	{
		overheatDefectChanceFactor=1;		//коэффициент вероятности выхода из строя за 1 секунду при резком наборе температуры
		overheatDamageFactor=20;			//количество процентов прочности, снимаемое в секунду при перегреве

		additionalPowerFactor = 1;			//определяет степень влияния навыков пилота на разгон/торможение
	}


	configurationvalues::Pilot::Pilot()
	{
		pilotFaintTime = 10;
		pilotMaxG = 5;
	}


	configurationvalues::Collisions::Collisions()
	{
		randomRicochetChance = 0.1f;
		bulletDeflectionSigma = 10.f;
		visibilityDistance = 2000;
		rammingDistance = 50;
	}


	configurationvalues::Server::Server()
	{

		spawnCooldownTime = 1.5f;

		port = 40000;
		numThreads = 4;
		roomMessagesPerFrame = 16;
		roomFrameTime = 0.03f;

		hangarMessagesPerFrame = 2;
		hangarFrameTime = 0.1f;


		unloginedDisconnectTime = 2;
		maxClientsNumber = 1000;

	}


	configurationvalues::Bots::Bots()
	{
		hardStartExpiriance = 4000000;

		rammingWarningDistance = 150.f;

		easyFleeTimeMean = 2.0f;
		easyFleeTimeSigma = 1.3f;
		easyPatrolScanSpeed = 20.f;

		easyTargerSelectionTimeSigma = 4.f;
		easyTargetSelectionTimeMean = 5.f;

		easyAttackSector = 4.f;
		easyShootingDistanceAgility = 10.f;
		easyMaxAttakersCount = 2;

		easyPanicChance = 0.1;
		easyPanicTimeMean = 20.f;
		easyPanicTimeSigma = 15.f;
	}


	configurationvalues::Turrets::Turrets()
	{
		accuracyFactor = 1.f;
		angleImpactFactor = 1.f;
		impactFactor = 1.f;
		impactRandomnesFactor = 10.f;
		shootRateFactor = 1.f;
		gunLength = 2.f;
		
		aimTimerMean = 1.f;
		aimTimerSigma = 0.5f;
		
		aimErrorSigma = 0.5f;
		
		aimIntense = 5.f;
		aimExp = 0.3f;

		shootTimeMean = 2.5f;
		shootTimeSigma = 2.f;
		cooldownTimeMean = 4.0f;
		cooldownTimeSigma = 2.0f;

	}

}
