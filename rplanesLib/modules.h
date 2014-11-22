#pragma  once
//данный файл содержит классы модулей(конструктивных элементов самолета), а также классы пуля и запущенная ракета
#include "stdafx.h"
#include "hitzone.h"
namespace rplanes
{
	enum GunType {GUN1, GUN2, GUN3, GUN4, CANNON1, CANNON2, CANNON3, CANNON4}; //тип именно подвески. Чем выше число справа, тем круче пушку можно запихать. Собственно параметр для магазина(shop, not magazine).
	enum EngineType {SCREW, JET};
	enum ModuleType {GUN, MISSILE, WING, TAIL, CABINE, FRAMEWORK, TANK, ENGINE, AMMUNITION, TURRET};

	namespace planedata
	{
		// наличие дефекта модуля, может устраниться со временем
		class Defect 
		{
		public:
			// при timer >= permanentTimerValue деффект неустраним
			Defect(  float timer = -1.f ):  timer_(timer), updateRequired_(true) 
			{}
			//поврежден ли модуль
			operator bool()const;
			//является ли повреждение перманентным
			bool isPermanent()const;
			//уменьшить значение таймера. Возвращает true если дефект был устранен
			bool decrementTimer( float offset );
			//увеличить значение таймера. при timer >= permanentTimerValue деффект неустраним
			void incrementTimer( float offset ); 
			//сбросить значение таймера
			void reset( float permanentTimerValue );
			//проверить был ли модуль поврежден/починен со времени последнего вызова  checkUpdateNeed
			bool checkUpdateNeed();
		private:
			// необходимость обновления статических параметров самолета
			bool updateRequired_; 
			float timer_, permanentTimerValue_;
		};

		class ModuleHP
		{
		public:
			friend class Module;
			//причина, по которой произошло последнее изменение модуля
			enum ChangeReason
			{
				HIT,
				FIRE,
				REPAIR
			};

			//сломан ли модуль
			bool isDefected()const;

			//проверить было ли изменено состояние(прочность, статус поломки) cо времени последнего вызова checkConditionChange
			bool checkConditionChange();
			//проверить был ли модуль сломан/починен со времени последнего вызова checkDefectUpdate
			bool checkDefectUpdate();
			//сбросить таймер поломки. Модуль может быть починен
			void decrementDefectTimer(float frameTime);
			//причина последнего изменения состояния
			ChangeReason getReason()const;
			//игрок нанесший последний урон
			size_t getLastHitClient()const;
			//возвращаяет хп
			operator int()const;
		private:
			void reset(size_t clientID, int hpMax, float defectChance, float permanentTimerValue, float timerValueSigma, float timerValueMean);

			//сломать модуль
			void breakDown(ChangeReason reason, float timerVal);

			//повредить модуль. Модуль может сломаться
			void damage(float damageVal, size_t clientID, ChangeReason reason);

			//вероятность поломки при получении повреждения на половину прочности
			float defectChance_;

			float
				timerValueSigma_,
				timerValueMean_;


			//максимальное значение прочности
			int hpMax_;
			//текущая проочность
			int hp_;
			//определяет сломан ли модуль
			Defect defect_;
			//игрок нанесший последий урон
			size_t lastHitClient_;
			//было ли изменено состояние модуля
			bool hpUpdated_;
			//причина изменения состояния
			ChangeReason changeReason_;
		};

#pragma db object polymorphic
		class Module
		{
		public:
#pragma db id
			std::string name;

			int price;
			//максимальная прочность
			int hpMax;
			//вес в килограммах
			int weigth;
			//в миллиметрах толщина брони
			float armor; 

			float 
				permanentDefectTimerValue,
				defectTimerValueSigma,
				defectTimerValueMean;

			//вероятность выхода из строя при получении повреждения на половину прочности
			float defectChance; 

			//текущая прочность
#pragma db transient
			ModuleHP hp;

			//зона повреждения
#pragma db transient
			serverdata::HitZone hitZone;

			//номер в самолете
#pragma db transient
			size_t pos; 


			void reload( size_t clientID );


			virtual ModuleType getType() = 0;


			virtual ~Module();;


			//сломать модуль
			void breakDown(ModuleHP::ChangeReason reason, float timerVal)
			{
				hp.breakDown(reason, timerVal);
			}

			//повредить модуль. Модуль может сломаться
			void damage(float damageVal, size_t clientID, ModuleHP::ChangeReason reason)
			{
				hp.damage(damageVal, clientID, reason);
				if ( hp.isDefected() )
				{
					if (getType() == AMMUNITION || getType() == TANK)
					{
						hp.damage( hpMax , clientID, ModuleHP::FIRE);
					}
				}
			}
		protected:
			virtual void derv_reload();

		};

#pragma db object
		class Gun : public Module
		{
		public:
			Gun();
			ModuleType getType();

			//таймер для реализации стрельбы
#pragma db transient
			float timer;

			size_t caliber;
			int damage;
			//тип подвески
			GunType type;		
			float penetration,	
				shootRate,		
				speed,			
				acceleration,
				accuracy,
				impact;


			//темп стрельбы при повреждении
			float defectShootRate();

			//подходит ли пушка к подвеске gt;
			bool isSuitable( GunType gunType );

			float getMaxDistance( float planeSpeed );

			//получить время, через которое выпущенный снаряд достигнет цели
			float getHitTime( float shootingDistance, float planeSpeed);

			//получить начальную вертикальную скорость, необходимую для стрельбы на указанную дистанцию
			float getSpeedZ(float shootingDistance, float planeSpeed, float gunHeight = 0.f);

		protected:
			void derv_reload();
		private:
			void init();
		};

#pragma db object
		class Missile : public Module
		{
		public:
			ModuleType getType();
			bool isEmpty;
			// урон в радиусе при взрыве
			int damage; 
			float radius,
				speed,
				acceleration,
				ttl,
				accuracy;

			float getMaxDistance( float planeSpeed );
			//получить время, через которое выпущенный снаряд достигнет цели
			float getHitTime( float shootingDistance, float planeSpeed );
			//получить начальную вертикальную скорость, необходимую для стрельбы на указанную дистанцию
			float getSpeedZ( float shootingDistance, float planeSpeed );
		protected:
			void derv_reload();
		};

#pragma db object
		class Wing : public Module
		{
		public:
			ModuleType getType();
			float surface;
			float rollSpeed;
			float defectSurfase();
		};
#pragma db object
		class Tail : public Module
		{
		public:
			ModuleType getType();
			float mobilityFactor;
			float defectMobilityFactor();
		};
#pragma db object
		class Cabine : public Module
		{
		public:
			ModuleType getType();
		};
#pragma db object
		class Framework : public Module
		{
		public:
			ModuleType getType();
			float Vmax, Vmin, Gmax;
			float defectVmax();
		};
#pragma db object
		class Tank : public Module
		{
		public:
			ModuleType getType();
			//емкость
			float capacity;
			//запас
#pragma db transient
			float fuel;		
		protected:
			void derv_reload()
			{
				fuel = capacity;
			}
		};
#pragma db object
		class Engine : public Module
		{
		public:
			ModuleType getType();
			Engine()
			{
				init();
			}
			EngineType type;
			float maxPower;
			float fuelIntake;

			//температурные параметры
			float
				maxTemperature,					//температура при минимальной скорости и максимальной тяге
				minTemperature,					//температура при максимальной скорости и минимальной тяге  
				declaredMaxDt,					//максимально допустимое наращивание температуры
				criticalTemperature,			//температура, при которой двигатель повреждается
				heatingFactor,					//степень нагревания двигателя
				cooldownFactor;					//степень охлаждения двигателя


#pragma db transient
			float temperature;
#pragma db transient
			float stabTemperature;
#pragma db transient
			float dt;
#pragma db transient
			float maxDt;
#pragma db transient
			float temperatureFactor;
			float defectTMax();
		protected:
			void derv_reload();

		private:
			void init();
		};
#pragma db object
		class Ammunition : public Module
		{
		public:
			ModuleType getType();
			size_t caliber;
			size_t maxCapacity;
#pragma db transient
			size_t capacity;
		protected:
			void derv_reload()
			{
				capacity = maxCapacity;
			}
		};
#pragma  db object
		class Turret : public Module
		{
		public:
			ModuleType getType();

			std::string gunName;
			size_t nGuns;

#pragma db transient
			Gun gun;


			//расстояние между пушками
#pragma db transient
			float gunShift;

#pragma db transient
			PointXYZ gunsPosition;

			//сектор обстрела
#pragma db transient
			float sector;
			//начальное направление турели
#pragma db transient 
			float startAngle;


			//вспомогательные данные

#pragma  db transient
			PointXY aimError;
#pragma db transient
			float aimTimer;
#pragma db transient
			float shootTimer;
#pragma db transient
			float cooldownTimer;
			//текущая дальность стрельбы
#pragma db transient
			float aimDistance;
			//текущий угол турели
#pragma db transient 
			float aimAngle;
#pragma db transient
			bool isShooting;

			std::vector< PointXYZ > getRotatedGunsPositions( float angle, float roll );

			Turret();

			virtual void derv_reload();

		private:
		};
	}

}

