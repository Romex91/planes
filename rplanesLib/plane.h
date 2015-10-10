#pragma once
#include "modules.h"
#include "pilot.h"
#include "projectile.h"

namespace rplanes
{
	enum Nation { ALLIES, AXIS };

	namespace serverdata
	{
		//in-room plane
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
			//parameters that are changing rarely (when some module brakes down).
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
			//parameters changing by the player
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
				//[0,100]
				short power;
				//distance between guns and the aim mark center in meters
				short shootingDistanceOffset;
				//[-100,100]
				short turningVal; 
				bool isShooting;
				bool launchMissile;
				//missiles Aim
				bool missileAim;		
			};
			//parameters that depend on statical parameters and pilot skills and nothing else
			//so we update them only when statical parameters changing
			class DependentParameters 
			{
			public:
				float frictionFactor,
					minPower;
			};

			// parameters needed to calculate target parameters
			// updating once per frame
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
			//parameters needed to move plane on the map and to display on the player monitor 
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
				float V,					
					acceleration,
					angularVelocity,
					angularAcceleration,
					//level of g-force vision blackout
					faintVal,
					faintTimer,
					clientShootingDistance,
					aimSize;
			};


			std::vector<  planedata::Module * > modules;
			//using when calculating aimsize. initializing in the reload method
			planedata:: Gun * bestGun;
			planedata::Missile * currentMissile;

			std::string name;

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

			//these parameters are changing rarely

			StaticalParameters statical;
			DependentParameters dependent;

			//parameters changing each frame

			PlanePosition position;
			ControllableParameters controllable;
			InterimParameters interim;
			TargetParameters target;


			bool isFilled()const;
			//reset fuel/ammunition/hp/lastHitClient. Change the best gun and current missile
			void reload( size_t clientID );
			//load hitzones. fill the modules array
			void initModules(); 
			void showParams();
			//run this method when modules parameters are changed
			void updateStatical();
			//run this method after updateStatical
			void updateDependent();


			void updateInterim();
			void move( float frameTime );

			//run after missile fire
			void updateCurrentMissile();

			//guns and turrets fire
			//new bullets ids shold be initialized after this method
			std::vector<Bullet> shoot( float frameTime, size_t clientID, float serverTime);

			std::vector< rplanes::serverdata::LaunchedMissile > launchMissile(size_t clientID);
		};
	}
}