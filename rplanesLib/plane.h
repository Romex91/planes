#pragma once
//данный файл содержит класс "готового" самолета
#include "modules.h"
#include "pilot.h"
#include "projectile.h"

namespace rplanes
{
	enum Nation { ALLIES, AXIS };

	namespace serverdata
	{
		//"готовый" самолет
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
			// параметры, которые меняются относительно редко.
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
			// параметры, манипулируемые игроком
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
				//в диапазоне от 0 до 100
				short power;
				//в метрах
				short shootingDistanceOffset;
				//в диапазоне от -100 до 100
				short turningVal; 
				bool isShooting;
				bool launchMissile;
				//ракетный прицел
				bool missileAim;		
			};
			// параметры, расчитываемые только из статических параметров и навыков пилота, в передаче туда-сюда не нуждаются.
			class DependentParameters 
			{
			public:
				float frictionFactor,
					minPower;
			};
			// промежуточные параметры. Являются сугубо серверной собственностью. Помимо статических и депендиальных параметров зависят от целевых параметров.
			// Расчитывается каждый кадр.
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
			//собственно цель расчетов. Характер движения ,состояние двигателя и пилота. Расчитывается каждый кадр.
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
				float V,					// скорость
					acceleration,			// прирост скорости
					angularVelocity,		// угловая скорость
					angularAcceleration,	// прирост угловой скорости
					faintVal,				// уровень затемнения экрана от перегрузок при значении > 1 - обморок
					faintTimer,				// оставшееся время нахождения в обмороке
					clientShootingDistance,	// дальность стрельбы, отображаемая у клиента
					aimSize;				// радиус прицельной рамки
			};

			//data
			std::vector<  planedata::Module * > modules;
			planedata:: Gun * bestGun;
			planedata::Missile * currentMissile;

			std::string name;
			//Элементы
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

			//Параметры
			//редко меняемые
			StaticalParameters statical;
			DependentParameters dependent;
			//Часто меняемые
			PlanePosition position;
			ControllableParameters controllable;
			InterimParameters interim;
			TargetParameters target;

			//functions


			bool isFilled()const;
			//устанавливает топливо/боезапас/прочность модулей/lastHtiClient в начальное значение. Выбор лучшей пушки и текущей ракеты
			void reload( size_t clientID );
			//загрузка зон повреждения, заполнение массива указателей на модули. 
			void initModules(); 
			void showParams();
			//запускается при изменении  параметров модулей
			void updateStatical();
			//запускается при изменении параметров модулей
			void updateDependent();

			//запускается перед move каждый кадр
			void updateInterim();
			//запускается каждый кадр
			void move( float frameTime );

			//запускается при стрельбе ракетами
			void updateCurrentMissile();

			//провести стрельбу из курсовых орудий и турелей в течении frameTime если пилот не в обмороке. ID пуль не устанавливаются
			std::vector<Bullet> shoot( float frameTime, size_t clientID, float serverTime);

			std::vector< rplanes::serverdata::LaunchedMissile > launchMissile(size_t clientID);
		};
	}
}