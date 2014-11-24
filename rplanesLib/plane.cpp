#include <iostream>
#define _USE_MATH_DEFINES
#include < math.h >
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <float.h>
#include < boost/random.hpp>

#include "plane.h"
#include "configuration.h"
#include "helpers.h"
#include "exceptions.h"

using std::cout;
using std::endl;


namespace rplanes
{
	namespace serverdata
	{
		static boost::random::mt19937 gen;

		void Plane::updateStatical() // Запускается при изменении параметров модулей.
		{
			//рассчет массы
			statical.mass = 0;
			for( auto & Module : modules )
			{
				statical.mass += Module->weigth;
			}

			//рассчет мощности
			statical.maxPower = 0;

			for( size_t i = 0; i < engines.size(); i++ )
			{
				statical.maxPower += engines[i].maxPower;
			}
			//рассчет показателей корпуса
			statical.Vmax = framework.defectVmax();

			statical.Vmin = framework.Vmin;
			statical.Gmax = framework.Gmax;

			//рассчет коэффициента маневренности
			statical.mobilityFactor = tail.defectMobilityFactor();
			//рассчет площади крыла и управляемости
			statical.surface = 0;
			statical.rollSpeed = 0;
			for( size_t i=0; i< wings.size(); i++ )
			{
				statical.surface += wings[i].defectSurfase();
				statical.rollSpeed += wings[i].rollSpeed;
			}
			statical.rollSpeed /= wings.size();
		}
		void Plane::updateDependent() // Запускается при изменении параметров модулей.
		{
			dependent.frictionFactor 
				= statical.maxPower 
				/ ( std::pow( statical.Vmax, 2 ) 
				+ std::pow( statical.Vmax - statical.Vmin, 2 )
				* statical.Vmax / 100 );
			
			dependent.minPower 
				= std::pow( statical.Vmin , 2) 
				* dependent.frictionFactor 
				/ ( 1 + pilot.get_skill_engine(cabine.hp.isDefected())/ 10 );
			
			for ( auto & engine: engines )
			{
				engine.temperatureFactor = statical.Vmin / statical.maxPower * ( engine.defectTMax() - engine.minTemperature );
				engine.maxDt = engine.declaredMaxDt * pilot.get_skill_engine(cabine.hp.isDefected());
			}
		}

		void Plane::updateInterim()
		{
			if ( controllable.power > 100 )
			{
				controllable.power = 100;
			}
			if ( controllable.power < 0 )
			{
				controllable.power = 0;
			}
			float currentPower = controllable.power / 100.f * ( statical.maxPower - dependent.minPower ) + dependent.minPower ;
			//в случае обморока выдается минимальная мощность двигателя
			if( target.faintTimer < 0 )
			{
				interim.additionalPower = pilot.get_skill_engine( cabine.hp.isDefected() ) 
					* currentPower
					* ( (statical.Vmin - target.V ) 
					/ ( statical.Vmax - statical.Vmin ) + 1  ) / 10 
					* configuration().engine.additionalPowerFactor;
				interim.summaryPower = interim.additionalPower + currentPower;
			}else
			{
				interim.additionalPower = pilot.get_skill_engine( cabine.hp.isDefected() ) 
					* dependent.minPower 
					* ( (statical.Vmin - target.V ) 
					/ ( statical.Vmax - statical.Vmin ) + 1  ) / 10 
					* configuration().engine.additionalPowerFactor;
				interim.summaryPower = dependent.minPower + interim.additionalPower;
			}

			interim.maxAngularVelocity = std::sqrt( target.V ) 
				* statical.surface 
				/ statical.mass 
				* pilot.get_skill_flight( cabine.hp.isDefected() ) 
				* statical.mobilityFactor 
				* 400
				* configuration().flight.maxAngularVelocityFactor;

			interim.ACS = std::abs( target.angularVelocity * target.V * M_PI / 180 );

			interim.frictionForce = target.V * dependent.frictionFactor + std::pow( target.V - statical.Vmin, 2 ) * dependent.frictionFactor / 100;
			interim.maneuverFrictionForce 
				= ( std::abs(target.angularVelocity) / interim.maxAngularVelocity ) 
				* interim.frictionForce 
				/ pilot.get_skill_flight( cabine.hp.isDefected() ) 
				* configuration().flight.maneuverFrictionFactor;

			for( auto & Engine : engines )
			{
				Engine.stabTemperature = ( currentPower - dependent.minPower) / target.V * Engine.temperatureFactor + Engine.minTemperature;
			}
			interim.engineForce = interim.summaryPower  / target.V;
			interim.force = ( interim.engineForce - interim.frictionForce - interim.maneuverFrictionForce ) * 10000 *
				configuration().flight.forceFactor;

			//дальность стрелбы
			//максимальные показатели
			interim.maxShootingDistane = 0.f;
			interim.maxMissileDistance = 0.f;
			if ( bestGun != NULL )
			{
				interim.maxShootingDistane = bestGun->getMaxDistance(target.V);
			}
			if ( currentMissile != NULL )
			{
				interim.maxMissileDistance = currentMissile->getMaxDistance(target.V);
			}

			//проверка максимальных показателей
			if ( interim.maxShootingDistane > configuration().shooting.maxDistance )
			{
				interim.maxShootingDistane = configuration().shooting.maxDistance;
			}

			interim.shootingDistance += controllable.shootingDistanceOffset;
			if ( controllable.missileAim )
			{
				if ( interim.shootingDistance > interim.maxMissileDistance )
				{
					interim.shootingDistance = interim.maxMissileDistance;
				}
			}else
			{
				if ( interim.shootingDistance > interim.maxShootingDistane )
				{
					interim.shootingDistance = interim.maxShootingDistane;
				}
			}
			if ( interim.shootingDistance < configuration().shooting.minDistance )
			{
				interim.shootingDistance = configuration().shooting.minDistance;
			}

		}
		void Plane::move( float frameTime )
		{
			while( position.angle > 360 )
				position.angle -= 360;
			while( position.angle < 0 )
				position.angle += 360;

			//перегрузки
			target.faintVal 
				= interim.ACS 
				/ pilot.get_skill_endurance(cabine.hp.isDefected()) 
				/ 10.f / configuration().pilot.pilotMaxG;

			if ( target.faintVal > 1.f  && target.faintTimer < 0 )
			{
				target.faintTimer += configuration().pilot.pilotFaintTime;
			}
			if ( target.faintTimer > 0 )
			{
				controllable.turningVal = 0;
				controllable.isShooting = false;
				controllable.launchMissile = false;
				target.faintTimer -= frameTime;
				target.faintVal =  1 -  
					std::pow ( 
					( configuration().pilot.pilotFaintTime - target.faintTimer) 
					/ configuration().pilot.pilotFaintTime
					, 16.f) ;
			}

			//поступательное движение
			target.acceleration = interim.force/statical.mass;
			target.V += target.acceleration * frameTime;

			if ( target.V < statical.Vmin )
			{
				target.V = statical.Vmin;
			}

			position.x += target.V * std::cos( position.angle / 180.f * M_PI) * frameTime * configuration().flight.speedFactor;
			position.y += target.V * std::sin( position.angle / 180.f * M_PI ) * frameTime * configuration().flight.speedFactor;
			//вращательное движение

			//расчет модуля приращения угловой скорости
			//набор угловой скорости происходит до того как перегрузка( пилота / ЛА) не возрастет до максимума 
			//или не будет достигнута максимальная угловая скорость
			float  angularVal = std::abs( statical.Gmax * 10.f - interim.ACS )
				* pilot.get_skill_flight( cabine.hp.isDefected() )
				* ( 1 - target.angularVelocity / interim.maxAngularVelocity )
				* configuration().flight.angleAccelerationFactor 
				* ( 1 - std::abs( target.faintVal) )
				/ 5.f
				* statical.rollSpeed;
			
			//сброс угловой скорости
			float reverseAngularVal = statical.Gmax * 10.f
				* pilot.get_skill_flight( cabine.hp.isDefected() )
				* configuration().flight.reverseAngleAccelerationFactor
				/ 5.f
				* statical.rollSpeed;

			if ( target.faintTimer > 0 )
			{
				reverseAngularVal *= 0.1f;
			}


			if ( controllable.turningVal > 100 )
			{
				controllable.turningVal = 100;
			}
			
			if (  controllable.turningVal < -100 )
			{
				controllable.turningVal = -100;
			}

			float sign;
			
			if ( controllable.turningVal > 0 )
			{
				sign = 1.f;
			}else
			{
				sign = -1.f;
			}

			//расчет требуемой угловой скорости из данных управления

			float maxAngularVelocityG = statical.Gmax * 10.f / std::fabs( target.V ) / M_PI * 180.f;

			float stabAngle = std::min( interim.maxAngularVelocity,
				maxAngularVelocityG)
				* controllable.turningVal / 100.f ;


			bool stable = false;
			//расчет углового ускорения
			if ( std::abs( target.angularVelocity - stabAngle ) 
				< configuration().flight.turningExp )
			{
				target.angularAcceleration = 0; //если угловая скорость соответствует требуемой, оставляем как есть
				stable = true;
			} else if( sign * target.angularVelocity > 0  ) 
			{
				//если угловая скорость одного знака с требуемой и меньше ее по модулю, наращиваем угловую скорость
				if ( std::abs(stabAngle) > std::abs(target.angularVelocity) )
				{
					target.angularAcceleration = sign * angularVal;
				}else
				{
					target.angularAcceleration = - sign * reverseAngularVal;
				}
			} else
			{
				target.angularAcceleration = sign * reverseAngularVal;
			}
			

			if ( !stable )
			{
				auto prevAngularVelocity = target.angularVelocity; 
				target.angularVelocity += target.angularAcceleration * frameTime ;
				if( (prevAngularVelocity - stabAngle)* (target.angularVelocity - stabAngle) < 0  )
				{
					target.angularVelocity = stabAngle;
				}
			}

			position.roll = 90 * interim.ACS / statical.Gmax / 10.f;
			if ( target.angularVelocity < 0 )
			{
				position.roll *= -1.f;
			}
			position.angle += target.angularVelocity 
				* frameTime * configuration().flight.angleVelocityFactor;


			//потребление двигателей
			for ( auto & Engine : engines )
			{
				float fuelIntake = Engine.fuelIntake 
					* frameTime 
					* (controllable.power / 100.f + dependent.minPower / statical.maxPower);
				bool tankIsEmpty = true;
				for ( auto & Tank : tanks  )
				{
					if ( Tank.fuel < 0 )
					{
						continue;
					}
					tankIsEmpty = false;
					Tank.fuel -= fuelIntake;
					break;
				}
			}

			//температура двигателей
			for (auto & engine : engines)
			{
				boost::random::normal_distribution<float> dist(engine.defectTimerValueMean, engine.defectTimerValueSigma);
				//расчет прироста температуры
				if( engine.stabTemperature > engine.temperature  )
					engine.dt = ( engine.stabTemperature - engine.temperature ) / 20 * engine.heatingFactor;
				else
					engine.dt = ( engine.stabTemperature - engine.temperature ) / 20 * engine.cooldownFactor;
				//проверка повреждений при резком наборе
				if ( engine.dt > engine.maxDt )
				{
					float chance = configuration().engine.overheatDefectChanceFactor * engine.defectChance * frameTime; //расчет вероятности повреждения
					if( rand()/static_cast<float>(RAND_MAX) < chance )
					{
						engine.breakDown( planedata::ModuleHP::FIRE, dist(gen));
					}
				}
				//приращение температуры
				engine.temperature += engine.dt * frameTime;
				//проверка перегрева
				if ( engine.temperature > engine.criticalTemperature )
				{
					engine.damage( configuration().engine.overheatDamageFactor * engine.hpMax / 100 * frameTime, 
						engine.hp.getLastHitClient(),
						planedata::ModuleHP::FIRE);
				}
			}
			//прицел
			target.aimSize = 0;
			if ( controllable.missileAim && currentMissile!=NULL )
			{
				target.aimSize = target.V * 
					configuration().flight.speedFactor * 
					bestGun->getHitTime( interim.shootingDistance, target.V );
			}else if ( bestGun != NULL )
			{
				target.aimSize = target.V * 
					configuration().flight.speedFactor * 
					bestGun->getHitTime( interim.shootingDistance, target.V );
			}
			target.clientShootingDistance = interim.shootingDistance;
		}
		void Plane::initModules()
		{
			std::fstream in;
			in.open( std::string("../Resources/hitZones/") + name + ".txt", std::ios_base::in );
			if ( !in.is_open() )
				throw( planesException( "Не удалось загрузить зоны повреждения: файл ../Resources/hitZones/" + name + " не найден. " ) );
			std::stringstream ss;
			std::string moduleName, empty, line;
			HitZone hitZone;
			std::getline(in, moduleName);
			while( in.good() )
			{
				hitZone = HitZone();
				std::getline(in, empty);
				while( in.good() )
				{
					std::getline( in, line );
					ss.clear();
					ss.str("");
					ss << line;
					if ( line.find( "height" )!=line.npos )
					{
						ss>> empty >> hitZone.base_.heightRange.a>> hitZone.base_.heightRange.b;
						break;
					} 
					else
					{
						float x, y;
						ss >> x >> y;
						hitZone.base_.points.push_back(PointXY( x , y));
					}
				}
				ss.clear();
				ss.str("");
				ss<< moduleName;
				size_t pos = 0;
				ss>> empty >> pos;

				if( moduleName.find("wing")!= moduleName.npos )
				{
					wings[pos].hitZone = hitZone;
				}else if( moduleName.find("framework")!= moduleName.npos )
				{
					framework.hitZone = hitZone;
				}else if( moduleName.find("engine")!= moduleName.npos )
				{
					engines[ pos ].hitZone = hitZone;
				}else if( moduleName.find("tail")!= moduleName.npos )
				{
					tail.hitZone = hitZone;
				}else if( moduleName.find("gun")!= moduleName.npos )
				{
					guns[pos].hitZone = hitZone;
				}else if( moduleName.find("tank")!= moduleName.npos )
				{
					tanks[pos].hitZone = hitZone;
				}else if( moduleName.find("ammunition")!= moduleName.npos )
				{
					ammunitions[pos].hitZone = hitZone;
				}else if( moduleName.find("cabine")!= moduleName.npos )
				{
					cabine.hitZone = hitZone;
				}else if( moduleName.find("missile")!= moduleName.npos )
				{
					missiles[pos].hitZone = hitZone;
				}else if (moduleName.find("turret") != moduleName.npos)
				{
					planedata::Turret & turret = turrets[pos];
					turret.hitZone = hitZone;
					ss >> turret.gunShift;
					ss >> turret.gunsPosition.x;
					ss >> turret.gunsPosition.y;
					ss >> turret.gunsPosition.z;
					ss >> turret.startAngle;
					ss >> turret.sector;

					turret.aimAngle = turret.startAngle;
					turret.aimDistance = 0.f;
				}
				else
				{
					throw( planesException( "Не удалось загрузить зоны повреждения: ошибка формата зон повреждения. " ) );
				}
				std::getline(in, moduleName);
			}
			modules.clear();
			modules.push_back( &cabine );
			modules.push_back( &framework );
			modules.push_back( &tail );

			int j = 0;
			for( auto i = guns.begin(); i != guns.end(); i++ , j++ )
			{
				i->pos = j;
				modules.push_back( &(*i) );
			}
			j = 0;
			for( auto i = missiles.begin(); i != missiles.end(); i++ , j++ )
			{
				i->pos = j;
				modules.push_back( &(*i) );
			}
			j = 0;
			for( auto i = ammunitions.begin(); i != ammunitions.end(); i++ , j++ )
			{
				i->pos = j;
				modules.push_back( &(*i) );
			}
			j = 0;
			for( auto i = engines.begin(); i != engines.end(); i++ , j++ )
			{
				i->pos = j;
				modules.push_back( &(*i) );
			}
			j = 0;
			for( auto i = tanks.begin(); i!=tanks.end(); i++ , j++ )
			{
				i->pos = j;
				modules.push_back( &(*i) );
			}
			j = 0;
			for( auto i = wings.begin(); i!=wings.end(); i++ , j++ )
			{
				i->pos = j;
				modules.push_back( &(*i) );
			}
			for (auto i = turrets.begin(); i != turrets.end(); i++, j++)
			{
				i->pos = j;
				modules.push_back(&(*i));
			}

		}

		void Plane::updateCurrentMissile()
		{
			currentMissile = NULL;
			for ( size_t i = 0; i < missiles.size() && i <= missiles.size() / 2; i++ )
			{
				if ( missiles[i].isEmpty == false )
				{
					currentMissile = &missiles[i];
					return;
				}
				if ( missiles[ missiles.size() - i ].isEmpty == false )
				{
					currentMissile = &missiles[ missiles.size() - i];
					return;
				}
			}
		}
		//провести стрельбу из курсовых орудий и турелей в течении frameTime если пилот не в обмороке
		std::vector<Bullet> Plane::shoot( float frameTime, size_t clientID, float serverTime)
		{
			//для отдачи
			serverTime *= configuration().shooting.impactRandomnesFactor;

			float shootingDistanceImpact = 0.f;
			float angleImpact = 0.f;

			std::vector<Bullet> retval;
			if ( target.faintTimer > 0 )
			{
				return retval;
			}
			for( size_t gunNo = 0; gunNo < guns.size() && controllable.isShooting; gunNo++ )
			{
				auto & gun = guns[gunNo];

				if (gun.hp < 0)
				{
					continue;
				}

				float speedZ = gun.getSpeedZ( interim.shootingDistance, target.V);

				gun.timer += frameTime; 

				for ( auto & Ammunition: ammunitions )
				{
					float sRate = gun.defectShootRate() * configuration().shooting.shootRateFactor;
					while ( Ammunition.caliber == gun.caliber && gun.timer > 60.0 / sRate && Ammunition.capacity > 0 )
					{
						gun.timer  -= 60.0 / sRate;
						Ammunition.capacity--;

						Bullet newBullet;

						newBullet.acceleration = gun.acceleration * configuration().shooting.accelerationFactor;

						newBullet.damage = gun.damage * configuration().shooting.damageFactor;
						newBullet.penetration = gun.penetration * configuration().shooting.penetFactor;

						newBullet.prevX = newBullet.x =  position.x + gun.hitZone.getCenter().x;
						newBullet.prevY = newBullet.y =  position.y + gun.hitZone.getCenter().y;
						
						newBullet.startSpeed = gun.speed
							* configuration().shooting.speedFactor;

						newBullet.speedXY = newBullet.startSpeed //пушечная составляющая скорости
							+ target.V * configuration().flight.speedFactor; //составляющая скорости самолета;

						boost::random::normal_distribution<float> speedDist( 0.f,
							std::tan(gun.accuracy / 180 * M_PI) * newBullet.speedXY);

						newBullet.speedZ 
							= gun.getSpeedZ(interim.shootingDistance, target.V)
							+ speedDist(gen) 
							/ configuration().shooting.accuracyFactor;

						boost::random::normal_distribution<float> angleDist(0.f,
							gun.accuracy);


						newBullet.angleXY 
							= position.angle 
							+ angleDist(gen) 
							/ configuration().shooting.accuracyFactor;


						newBullet.z = (gun.hitZone.shape.heightRange.a + gun.hitZone.shape.heightRange.b)/2;
						newBullet.planeID = clientID;
						newBullet.gunNo = gunNo;
						newBullet.ID = 0;
						newBullet.caliber = gun.caliber;
						retval.push_back(newBullet);
						//отдача

						float verticalTurn = std::sin(serverTime + clientID) + std::sin(serverTime*1.3) + std::sin(serverTime*1.7);

						float horizontalTurn = std::sin(serverTime * 1.1 + clientID) + std::sin( serverTime * 1.6 ) + std::sin(serverTime * 2 );

						angleImpact
							+= horizontalTurn
							* gun.impact
							* configuration().shooting.angleImpactFactor
							/ pilot.get_skill_shooting(cabine.hp.isDefected())
							/ statical.mass;

						shootingDistanceImpact
							+= verticalTurn
							* gun.impact
							* configuration().shooting.impactFactor
							/ pilot.get_skill_shooting(cabine.hp.isDefected())
							/ statical.mass;
					}
				}
			}

			interim.shootingDistance += shootingDistanceImpact;
			position.angle += angleImpact;

			if (interim.shootingDistance < configuration().shooting.minDistance)
			{
				interim.shootingDistance = configuration().shooting.minDistance;
			}
			if (interim.shootingDistance > interim.maxShootingDistane)
			{
				interim.shootingDistance = interim.maxShootingDistane;
			}

			//обработка турелей
			size_t gunNo = guns.size();
			for ( auto & turret : turrets )
			{
				if ( !turret.isShooting || turret.hp < 0 )
				{
					continue;
				}
				auto & gun = turret.gun;
				
				//подсчитываем количество выстрелов

				gun.timer += frameTime;
				size_t nShots = 0;

				float sRate = gun.defectShootRate() * configuration().turrets.shootRateFactor;
				for (auto & Ammunition : ammunitions)
				{
					while ( Ammunition.caliber == gun.caliber && Ammunition.capacity > 0 && gun.timer > 60.0 / sRate)
					{
						Ammunition.capacity -= turret.nGuns;
						gun.timer -= 60.0 / sRate;
						nShots++;
					}
				}

				//получаем позиции орудий
				auto gunPositions = turret.getRotatedGunsPositions(position.angle, position.roll);


				for ( auto & gunPosition : gunPositions )
				{
					for (size_t i = 0; i < nShots; i++)
					{
						Bullet newBullet;

						newBullet.acceleration = gun.acceleration * configuration().shooting.accelerationFactor;

						newBullet.damage = gun.damage * configuration().shooting.damageFactor;
						newBullet.penetration = gun.penetration * configuration().shooting.penetFactor;

						newBullet.prevX = newBullet.x = position.x + gunPosition.x;
						newBullet.prevY = newBullet.y = position.y + gunPosition.y;
						newBullet.z = newBullet.prevZ = gunPosition.z;

						newBullet.startSpeed = gun.speed
							* configuration().shooting.speedFactor;

						newBullet.speedXY = newBullet.startSpeed;

						boost::random::normal_distribution<float> speedDist(0.f,
							std::tan(gun.accuracy / 180 * M_PI) * newBullet.speedXY);

						newBullet.speedZ
							= gun.getSpeedZ( turret.aimDistance, 0.f, gunPositions.front().z)
							+ speedDist(gen)
							/ configuration().turrets.accuracyFactor;

						boost::random::normal_distribution<float> angleDist(0.f,
							gun.accuracy);

						newBullet.angleXY
							= position.angle
							+ turret.aimAngle
							+ angleDist(gen)
							/ configuration().turrets.accuracyFactor;

						newBullet.planeID = clientID;
						newBullet.gunNo = gunNo;
						newBullet.ID = 0;
						newBullet.caliber = gun.caliber;
						retval.push_back(newBullet);
						//отдача

						float verticalTurn = std::sin(serverTime + clientID) + std::sin(serverTime*1.3) + std::sin(serverTime*1.7);

						float horizontalTurn = std::sin(serverTime * 1.1 + clientID) + std::sin(serverTime * 1.6) + std::sin(serverTime * 2);

						turret.aimAngle
							+= horizontalTurn
							* gun.impact
							* configuration().turrets.angleImpactFactor
							/ pilot.get_skill_shooting(turret.hp.isDefected())
							/ 10.f;

						turret.aimDistance
							+= verticalTurn
							* gun.impact
							* configuration().turrets.impactFactor
							/ pilot.get_skill_shooting(turret.hp.isDefected())
							/ 10.f;

						if (turret.aimDistance < configuration().shooting.minDistance )
						{
							turret.aimDistance = configuration().shooting.minDistance;
						}
						if ( turret.aimDistance > gun.getMaxDistance(0) )
						{
							turret.aimDistance = gun.getMaxDistance(0);
						}
					}
					gunNo++;
				}
			}
			return retval;
		}

		std::vector< rplanes::serverdata::LaunchedMissile > Plane::launchMissile(size_t clientID)
		{
			throw std::exception("not implemented");
			std::vector< rplanes::serverdata::LaunchedMissile > retval;
			if (target.faintTimer > 0)
			{
				return retval;
			}
			boost::random::normal_distribution<float> dist;

		}

		void Plane::reload( size_t clientID )
		{
			for(auto & module : modules )
			{
				module->reload(clientID);
			}
			//выбор пушки
			float gunRating = 0.f;
			bestGun = NULL;
			for( auto & gun: guns )
			{
				float gr = gun.damage * gun.penetration * gun.shootRate;
				if ( gr > gunRating )
				{
					gunRating = gr;
					bestGun = & gun;
				}
			}
			updateCurrentMissile();
		}

		void Plane::showParams()
		{
			PRINT_VAR( name);
			PRINT_VAR( nation);

			PRINT_VAR( statical.Gmax);
			PRINT_VAR( statical.mass);
			PRINT_VAR( statical.maxPower);
			PRINT_VAR( statical.mobilityFactor);
			PRINT_VAR( statical.surface);
			PRINT_VAR( statical.Vmax);
			PRINT_VAR( statical.Vmin);

			PRINT_VAR( dependent.frictionFactor);
			PRINT_VAR( dependent.minPower);

			PRINT_VAR( controllable.isShooting);
			PRINT_VAR( controllable.launchMissile);
			PRINT_VAR( controllable.missileAim);
			PRINT_VAR( controllable.power);
			PRINT_VAR( controllable.shootingDistanceOffset);
			PRINT_VAR( controllable.turningVal);
			PRINT_VAR( interim.ACS);
			PRINT_VAR( interim.additionalPower);
			PRINT_VAR( interim.engineForce);
			PRINT_VAR( interim.force);
			PRINT_VAR( interim.frictionForce);
			PRINT_VAR( interim.maneuverFrictionForce);
			PRINT_VAR( interim.maxAngularVelocity);
			PRINT_VAR( interim.maxMissileDistance);
			PRINT_VAR( interim.maxShootingDistane);
			PRINT_VAR( interim.shootingDistance);
			PRINT_VAR( interim.summaryPower);

			PRINT_VAR( pilot.get_skill_endurance( cabine.hp.isDefected()));
			PRINT_VAR( pilot.get_skill_engine( cabine.hp.isDefected()));
			PRINT_VAR( pilot.get_skill_flight( cabine.hp.isDefected()));
			PRINT_VAR( pilot.get_skill_shooting( cabine.hp.isDefected()));

			PRINT_VAR( position.angle);
			PRINT_VAR( position.roll);
			PRINT_VAR( position.x);
			PRINT_VAR( position.y);

			PRINT_VAR( target.acceleration );
			PRINT_VAR( target.aimSize );
			PRINT_VAR( target.angularAcceleration );
			PRINT_VAR( target.angularVelocity );
			PRINT_VAR( target.clientShootingDistance );
			PRINT_VAR( target.faintTimer );
			PRINT_VAR( target.faintVal );
			PRINT_VAR( target.V );
		}

		bool Plane::isFilled() const
		{
			for (auto & tank : tanks)
			{
				if (tank.fuel > 0)
				{
					return true;
				}
			}
			return false;
		}


	}
}
#undef PRINT_VAR
