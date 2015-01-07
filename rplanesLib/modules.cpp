#define _USE_MATH_DEFINES
#include < math.h >
#include <iostream>
#include <boost/random.hpp>

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

#include "modules.h"
#include "helpers.h"
#include "configuration.h"

namespace rplanes
{
	namespace planedata
	{
		ModuleType Gun::getType()
		{
			return GUN;
		}

		bool Gun::isSuitable( GunType gt ) /*подходит ли пушка к подвеске gt */
		{
			if ( static_cast <size_t> (gt) < static_cast <size_t> (type)) // если общий ранг подвески меньше ранга пушки - не подходит
				return false;
			if( static_cast<size_t> (gt) <= 3 )	//если подвеска пулеметная - точно подходит
			{
				return true;
			}else if( static_cast<size_t> (type) <= 3 )//если подвеска пушечная, а type - пулемет, не подходит
			{
				return false;
			}
			return true;
		}

		float Gun::defectShootRate()
		{
			if ( hp.isDefected() )
			{
				return shootRate * configuration().defect.shootRateFactor;
			}
			return shootRate;
		}

		float Gun::getMaxDistance( float planeSpeed)
		{
			//решение задачи о равноускоренном движении из школьной физики
			float startSpeed = planeSpeed * configuration().flight.speedFactor + speed * configuration().shooting.speedFactor;
			float bulletAcceleration = acceleration * configuration().shooting.accelerationFactor;

			float stopSpeed = startSpeed * configuration().shooting.ttlFactor;
			float stopTime = (stopSpeed - startSpeed)/ bulletAcceleration;
			return startSpeed * stopTime + bulletAcceleration * stopTime * stopTime / 2;
		}

		float Gun::getHitTime( float shootingDistance, float planeSpeed )
		{
			//решение задачи о равноускоренном движении из школьной физики
			float startSpeed = planeSpeed * configuration().flight.speedFactor + speed * configuration().shooting.speedFactor;
			float bulletAcceleration = acceleration * configuration().shooting.accelerationFactor;

			float desc = startSpeed * startSpeed + 2 * bulletAcceleration * shootingDistance;
			if ( desc < 0.f )
			{
				std::cout << "ballistic calculation : descreminant < 0" << std::endl;
				PRINT_VAR(shootingDistance);
				PRINT_VAR(planeSpeed);
				PRINT_VAR(startSpeed);
				PRINT_VAR(getMaxDistance(planeSpeed));
				PRINT_VAR(shootingDistance);

				return 0.f;
			}
			return (std::sqrt( desc ) - startSpeed) / bulletAcceleration;
		}

		float Gun::getSpeedZ(float shootingDistance, float planeSpeed, float gunHeight)
		{
			//решение задачи о равноускоренном движении из школьной физики

			float hitTime = getHitTime(shootingDistance, planeSpeed);
			return hitTime * 10.f * configuration().shooting.gravity / 2.f - gunHeight / hitTime;
		}

		void Gun::init()
		{
			damage = 0;
			penetration = 0;
			speed = 0;
			acceleration = 0;
			accuracy = 0;
			impact = 0;
			timer = 0;
		}

		void Gun::derv_reload()
		{
			timer = 0.f;
		}

		Gun::Gun() :Module()
		{
			init();
		}

		ModuleType Missile::getType()
		{
			return MISSILE;
		}

		float Missile::getMaxDistance( float planeSpeed)
		{
			float startSpeed = planeSpeed * configuration().flight.speedFactor + speed * configuration().missile.speedFactor;
			float missileAcceleration = acceleration * configuration().missile.accelerationFactor;

			return startSpeed * ttl + missileAcceleration * ttl * ttl / 2; 
		}

		float Missile::getHitTime( float shootingDistance, float planeSpeed )
		{
			float startSpeed = planeSpeed * configuration().flight.speedFactor + speed * configuration().missile.speedFactor;
			float missileAcceleration = acceleration * configuration().missile.accelerationFactor;

			float desc = startSpeed * startSpeed + 2 * missileAcceleration * shootingDistance;
			return (std::sqrt( desc ) - startSpeed) / missileAcceleration;
		}

		float Missile::getSpeedZ( float shootingDistance, float planeSpeed )
		{
			float hitTime = getHitTime(shootingDistance, planeSpeed);
			return hitTime * 10.f * configuration().missile.gravity / 2.f;
		}

		void Missile::derv_reload()
		{
			if ( damage!=0 )
			{
				isEmpty = false;
			}
		}

		ModuleType Wing::getType()
		{
			return WING;
		}

		float Wing::defectSurfase()
		{
			if ( hp.isDefected() )
			{
				return surface * configuration().defect.surfaceFactor;
			}
			return surface;
		}

		ModuleType Tail::getType()
		{
			return TAIL;
		}

		float Tail::defectMobilityFactor()
		{
			if ( hp.isDefected() )
			{
				return mobilityFactor * configuration().defect.mobilityFactor;
			}
			return mobilityFactor;
		}

		ModuleType Cabine::getType()
		{
			return CABINE;
		}

		ModuleType Framework::getType()
		{
			return FRAMEWORK;
		}

		float Framework::defectVmax()
		{
			if ( hp.isDefected() )
			{
				float retval = Vmax * configuration().defect.vMaxFactor;
				if ( retval < Vmin )
				{
					return Vmin;
				}
				return retval;
			}
			return Vmax;
		}

		ModuleType Tank::getType()
		{
			return TANK;
		}

		ModuleType Engine::getType()
		{
			return ENGINE;
		}

		void Engine::init()
		{
			temperature = 60;
			stabTemperature = 60;
			dt = 0;
			maxDt = 4;
			temperatureFactor = 0.01;
		}

		float Engine::defectTMax()
		{
			if ( hp.isDefected() )
			{
				return maxTemperature * configuration().defect.tMaxFactor;
			}
			return maxTemperature;
		}

		void Engine::derv_reload()
		{
			temperature = stabTemperature = minTemperature;
			dt = 0;
		}

		ModuleType Ammunition::getType()
		{
			return AMMUNITION;
		}

		bool Defect::isPermanent()const
		{
			if ( timer_ >= permanentTimerValue_ )
				return true;
			return false;
		}

		bool Defect::decrementTimer( float offset )
		{
			if( *this && timer_ < permanentTimerValue_ )
			{
				timer_ -= offset;
				if (!*this)
				{
					updateRequired_ = true;
					return true;
				}
			}
			return false;
		}

		void Defect::incrementTimer( float offset ) /* при timer >= permanentTimerValue деффект неустраним */
		{
			updateRequired_ = true;
			if ( timer_ < permanentTimerValue_ )
				timer_ += offset;
		}

		bool Defect::checkUpdateNeed()
		{
			if( updateRequired_ )
			{
				updateRequired_ = false;
				return true;
			}
			return false;
		}

		void Defect::reset(float permanentTimerValue)
		{
			permanentTimerValue_ = permanentTimerValue;
			timer_ = -1.f;
			updateRequired_ = true;
		}

		Defect::operator bool() const
		{
			return timer_ > 0;
		}

		static boost::mt19937 gen;

		void ModuleHP::damage( float damageVal, size_t clientID, ChangeReason reason ) /*повредить модуль пулей или огнем */
		{
			boost::random::normal_distribution<float> dist(timerValueMean_, timerValueSigma_);
			lastHitClient_ = clientID;
			hp_-=damageVal;
			hpUpdated_ = true;
			changeReason_ = reason;
			float chance = configuration().defect.chanceFactor * defectChance_ * damageVal / hpMax_*2.f;
			if( rand()/static_cast<float>(RAND_MAX) < chance )
			{
				defect_.incrementTimer(dist(gen));
			}
		}

		ModuleHP::operator int() const
		{
			return hp_;
		}

		size_t ModuleHP::getLastHitClient() const
		{
			return lastHitClient_;
		}

		rplanes::planedata::ModuleHP::ChangeReason ModuleHP::getReason() const
		{
			return changeReason_;
		}

		bool ModuleHP::checkDefectUpdate()
		{
			return defect_.checkUpdateNeed();
		}

		bool ModuleHP::checkConditionChange()
		{
			if ( hpUpdated_ )
			{
				hpUpdated_ =false;
				return true;
			}
			return false;
		}

		void ModuleHP::reset(
			size_t clientID, 
			int hpMax,
			float defectChance,
			float permanentTimerValue,
			float timerValueSigma,
			float timerValueMean)
		{
			hpMax_ = hpMax;
			defectChance_ = defectChance;
			changeReason_ = REPAIR;
			hp_  = hpMax_;
			hpUpdated_ = true;
			lastHitClient_ = clientID;

			defect_.reset(permanentTimerValue);
			timerValueMean_ = timerValueMean;
			timerValueSigma_ = timerValueSigma;

		}

		bool ModuleHP::isDefected() const
		{
			return defect_;
		}

		void ModuleHP::breakDown( ChangeReason reason, float timerVal )
		{
			changeReason_ = reason;
			hpUpdated_ = true;
			defect_.incrementTimer(timerVal);
		}

		void ModuleHP::decrementDefectTimer( float frameTime )
		{
			if ( defect_.decrementTimer(frameTime) )
			{
				changeReason_ = REPAIR;
				hpUpdated_ = true;
			}
		}


		void Module::derv_reload()
		{

		}

		Module::~Module()
		{

		}

		void Module::reload( size_t clientID )
		{
			hp.reset(clientID, hpMax, defectChance, permanentDefectTimerValue, defectTimerValueSigma, defectTimerValueMean);
			derv_reload();
		}



		std::vector< PointXYZ > Turret::getRotatedGunsPositions(float angle, float roll)
		{
			while (roll > 360.f)
				roll -= 360.f;
			while (roll < 0)
				roll += 360.f;
			if (roll > 90.f && roll < 270)
				roll -= 180.f;
			std::vector<PointXYZ> retval(nGuns, gunsPosition);
			for (int i = 0; i < nGuns; i++)
			{
				//сдвигаем орудие
				retval[i].x += (i - static_cast<int>(nGuns) / 2 + 0.5f * ( (nGuns + 1)%2 ) ) * gunShift;

				//вычисляем угол вращения турели в xy
				float beta = aimAngle / 180 * M_PI;
				float theta = roll / 180 * M_PI;
				float alpha = atan(tan(beta) * cos(theta)) / M_PI * 180;
				//вращаем турель в xy
				sf::Vector2f xyPos(retval[i].x, retval[i].y);
				xyPos = sf::Transform().rotate(alpha, sf::Vector2f(gunsPosition.x, gunsPosition.y)).transformPoint(xyPos);
				//вращаем самолет в xz
				sf::Vector2f xzPos(xyPos.x, gunsPosition.z);
				xzPos = sf::Transform().rotate(roll, sf::Vector2f(0.f, 0.f)).transformPoint(xzPos);
				//вращаем самолет в xy
				xyPos.x = xzPos.x;
				xyPos = sf::Transform().rotate(angle - 90, sf::Vector2f(0.f, 0.f)).transformPoint(xyPos);
				retval[i].x = xyPos.x;
				retval[i].y = xyPos.y;
				retval[i].z = xzPos.y;
			}
			return retval;
		}

		Turret::Turret()
		{
			cooldownTimer = shootTimer = aimTimer = -1.f;
		}

		rplanes::ModuleType Turret::getType()
		{
			return TURRET;
		}

		void Turret::derv_reload()
		{
			gun.reload(0);
			aimAngle = startAngle;
			aimDistance = gun.getMaxDistance(0.f);
		}

	}
}
