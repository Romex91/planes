#pragma  once
#include "stdafx.h"
#include "hitzone.h"
namespace rplanes
{
	//a type of the gun pod. 
	//bigger the number at right greater gun can be mounted
	// i.e. a gun of type GUN2 can be mounted to a pod of type GUN3, but not vice versa
	enum GunType {GUN1, GUN2, GUN3, GUN4, CANNON1, CANNON2, CANNON3, CANNON4}; 
	enum EngineType {SCREW, JET};
	enum ModuleType {GUN, MISSILE, WING, TAIL, CABINE, FRAMEWORK, TANK, ENGINE, AMMUNITION, TURRET};

	namespace planedata
	{
		//a deffect can decrease some module parameters
		//if a deffect is not serious it can be fixed by the time
		class Defect 
		{
		public:
			// if timer >= permanentTimerValue deffect is unfixable
			Defect(  float timer = -1.f ):  timer_(timer), updateRequired_(true) 
			{}
			//true if defected
			operator bool()const;

			bool isPermanent()const;
			//returns true if a defect was fixed
			bool decrementTimer( float offset );
			// if timer >= permanentTimerValue deffect is unfixable
			void incrementTimer( float offset ); 
			//set timer to -1
			void reset( float permanentTimerValue );
			//check if the defect status was changed after the last call of checkUpdateNeed
			bool checkUpdateNeed();
		private:
			bool updateRequired_; 
			float timer_, permanentTimerValue_;
		};

		class ModuleHP
		{
		public:
			friend class Module;
			//the reason of the last module state change
			enum ChangeReason
			{
				HIT,
				FIRE,
				REPAIR
			};

			bool isDefected()const;

			//check if the defect status or hp were changed after the last call of checkConditionChange
			bool checkConditionChange();
			//check if the defect status was changed after the last call of checkDefectUpdate
			bool checkDefectUpdate();
			//check if the module is defected and the defect is fixable it can be repaired in this method
			void decrementDefectTimer(float frameTime);
			//the reason of the last condition change
			ChangeReason getReason()const;
			//the client id of the player who did the last damage
			size_t getLastHitClient()const;
			//returns integer hp value
			operator int()const;
		private:
			void reset(size_t clientID, int hpMax, float defectChance, float permanentTimerValue, float timerValueSigma, float timerValueMean);

			void breakDown(ChangeReason reason, float timerVal);

			//this method can apply a defect to the module
			void damage(float damageVal, size_t clientID, ChangeReason reason);

			//I am not good in the probability theory so I use this formula to check defection:
			//float chance = configuration().defect.chanceFactor * defectChance_ * damageVal / hpMax_*2.f;
			//if (rand() / static_cast<float>(RAND_MAX) < chance)
			float defectChance_;

			float
				timerValueSigma_,
				timerValueMean_;


			int hpMax_;
			int hp_;
			Defect defect_;
			//the client id of the player who did the last damage
			size_t lastHitClient_;
			//check if the defect status or hp were changed after the last call of checkConditionChange
			bool hpUpdated_;
			ChangeReason changeReason_;
		};

#pragma db object polymorphic
		class Module
		{
		public:
#pragma db id
			std::string name;

			int price;
			int hpMax;
			//weight in kg
			int weigth;
			//mm
			float armor; 

			float 
				permanentDefectTimerValue,
				defectTimerValueSigma,
				defectTimerValueMean;

			//I am not good in the probability theory so I use this formula to check defection:
			//float chance = configuration().defect.chanceFactor * defectChance_ * damageVal / hpMax_*2.f;
			//if (rand() / static_cast<float>(RAND_MAX) < chance)
			float defectChance;

#pragma db transient
			ModuleHP hp;

			//used in collision model
#pragma db transient
			serverdata::HitZone hitZone;

			//the slot number in the plane where the module is set
#pragma db transient
			size_t pos; 

			//reload hp ammunition fuel etc.
			void reload( size_t clientID );


			virtual ModuleType getType() = 0;


			virtual ~Module();;


			void breakDown(ModuleHP::ChangeReason reason, float timerVal);

			//the module can be defected in this module
			void damage(float damageVal, size_t clientID, ModuleHP::ChangeReason reason);
		protected:
			virtual void derv_reload();

		};

#pragma db object
		class Gun : public Module
		{
		public:
			Gun();
			ModuleType getType();

#pragma db transient
			float timer;

			size_t caliber;
			int damage;
			GunType type;		
			float penetration,	
				shootRate,		
				speed,			
				acceleration,
				accuracy,
				impact;


			//shoot rate considering if the module is defected
			float defectShootRate();

			//check if the gun is suitable to a pod of the type podType
			bool isSuitable( GunType podType );

			float getMaxDistance( float planeSpeed );

			//get a time for a bullet to reach the target
			float getHitTime( float shootingDistance, float planeSpeed);

			//get the begining rate of climb for a bullet to fly to the shooting distance
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
			int damage; 
			float radius,
				speed,
				acceleration,
				ttl,
				accuracy;

			float getMaxDistance( float planeSpeed );
			float getHitTime( float shootingDistance, float planeSpeed );
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

		//TODO: rename to cockpit
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
			float capacity;
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

			float
				//engine temperature stabilizing when speed is minimal and power is maximal
				maxTemperature,
				//engine temperature stabilizing when speed is maximal and power is minimal
				minTemperature,
				declaredMaxDt,
				criticalTemperature,
				//default value is 1
				heatingFactor,
				//default value is 1
				cooldownFactor;


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


			//distance between guns. reading when loading hitzones
#pragma db transient
			float gunShift;

#pragma db transient
			PointXYZ gunsPosition;

			//horizontal turret sector 
#pragma db transient
			float sector;
			//gun orientation. doesn't matter when sector >=180
#pragma db transient 
			float startAngle;


#pragma  db transient
			PointXY aimError;
#pragma db transient
			float aimTimer;
#pragma db transient
			float shootTimer;
#pragma db transient
			float cooldownTimer;
#pragma db transient
			float aimDistance;
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

