
#include "player.h"

Player::Player(const rplanes::serverdata::Plane & plane, std::string Name) : plane_(plane), name(Name)
{
	isJoined = true;
	name = Name;
}

void Player::setControllable(rplanes::serverdata::Plane::ControllableParameters controllable)
{
	if (plane_.target.faintTimer <= 0)
	{
		plane_.controllable = controllable;
	}
}

std::string Player::getPlaneName() const
{
	return plane_.name;
}

void Player::updateModulesMessage()
{
	auto & mess = messages.updateModules;
	mess.modules.clear();
	for (auto & player : visiblePlayers_)
	{
		mess.modules.insert(mess.modules.end(),
			player.second->messagesInfo_.updatedModules.begin(),
			player.second->messagesInfo_.updatedModules.end());
	}

}

void Player::updatePositionsMessage(float serverTime)
{
	auto & mess = messages.setPlanesPositions;
	mess.positions.clear();
	mess.time = serverTime;

	for (auto & player : visiblePlayers_)
	{
		MPlanesPositions::PlanePos p;
		p.extrapolationData = player.second->messagesInfo_.clientPlane.extrapolationData;
		p.planeID = player.first;
		p.pos = player.second->messagesInfo_.clientPlane.pos;
		mess.positions.push_back(p);
	}
}

void Player::updateProjectilesMessages(float serverTime)
{
	messages.createBullets.bullets.clear();
	messages.createMissiles.missiles.clear();
	messages.createRicochetes.bullets.clear();
	messages.destroyBullets.bullets.clear();
	messages.destroyMissiles.ids.clear();

	messages.createBullets.time = serverTime;
	messages.createMissiles.time = serverTime;
	messages.createRicochetes.time = serverTime;


	for (auto & Player : visiblePlayers_)
	{
		messages.createBullets.bullets.insert(
			messages.createBullets.bullets.end(),
			Player.second->messagesInfo_.newBullets.begin(),
			Player.second->messagesInfo_.newBullets.end());

		messages.createRicochetes.bullets.insert(
			messages.createRicochetes.bullets.end(),
			Player.second->messagesInfo_.newRicochetes.begin(),
			Player.second->messagesInfo_.newRicochetes.end());

		messages.createMissiles.missiles.insert(
			messages.createMissiles.missiles.end(),
			Player.second->messagesInfo_.newMissiles.begin(),
			Player.second->messagesInfo_.newMissiles.end());

		messages.destroyBullets.bullets.insert(
			messages.destroyBullets.bullets.end(),
			Player.second->messagesInfo_.destroyedBullets.begin(),
			Player.second->messagesInfo_.destroyedBullets.end());

		messages.destroyMissiles.ids.insert(
			messages.destroyMissiles.ids.end(),
			Player.second->messagesInfo_.destroyedMissiles.begin(),
			Player.second->messagesInfo_.destroyedMissiles.end());
	}
}


static boost::random::mt19937 gen;

class IntersectionInfo
{
public:
	size_t moduleNo;
	rplanes::PointXY a, b, intersectionPoint;
	float distance;
};
std::vector< IntersectionInfo > getIntersections(rplanes::serverdata::Bullet & bullet, rplanes::serverdata::Plane & target)
{
	std::vector< IntersectionInfo > intersections;
	for (size_t moduleNo = 0; moduleNo < target.modules.size(); moduleNo++)
	{
		auto & module = target.modules[moduleNo];
		auto & points = module->hitZone.shape.points;

		//TODO missiles are not implemented yet
		if (auto *missile = dynamic_cast<rplanes::planedata::Missile *>(module))
		{
			if (missile->isEmpty)
			{
				continue;
			}
		}

		if (points.size() < 2)
		{
			continue;
		}
		for (int j = 0; j < points.size(); j++)
		{
			rplanes::PointXY pointA1(bullet.x, bullet.y);
			rplanes::PointXY pointA2(bullet.prevX, bullet.prevY);

			rplanes::PointXY pointB1(points[j].x + target.position.x, points[j].y + target.position.y);
			size_t k = (j + 1) % points.size();
			rplanes::PointXY pointB2(points[k].x + target.position.x, points[k].y + target.position.y);

			rplanes::PointXY intersectionPoint;
			if (getLineSegmentsIntersection(pointA1, pointA2, pointB1, pointB2, intersectionPoint))
			{
				IntersectionInfo newIntersection;
				newIntersection.a = pointB1;
				newIntersection.b = pointB2;
				newIntersection.intersectionPoint = intersectionPoint;
				newIntersection.moduleNo = moduleNo;
				newIntersection.distance = distance(pointA2, intersectionPoint);
				intersections.push_back(newIntersection);
			}
		}
	}

	//sorting the intersections by the distance from the previous position of the bullet
	std::sort(intersections.begin(), intersections.end(), [](IntersectionInfo a, IntersectionInfo b)
	{
		return a.distance < b.distance;
	});
	return intersections;
}

rplanes::PointXY getDeflectedPoint(DestroyablePlane * target, rplanes::PointXY gunPosition, rplanes::planedata::Gun & gun, float shooterSpeed /*= 0.f*/)
{
	//this solution is rough

	//counting current distance to the target
	float d = distance(gunPosition, target->position);

	//calculating mean bullet speed flying to the current target position
	float vMean = (gun.speed * rplanes::configuration().shooting.speedFactor
		+ shooterSpeed * rplanes::configuration().flight.speedFactor)
		+ gun.acceleration * rplanes::configuration().shooting.accelerationFactor * gun.getHitTime(d, shooterSpeed) / 2.f;


	//calculating ratio of the target speed to the mean bullet speed 
	float k = (target->target.V * rplanes::configuration().flight.speedFactor) / vMean;
	
	//calculating the deflected point
	rplanes::PointXY retval;
	retval.x = target->position.x + k * d * std::cos(target->position.angle / 180.f * M_PI);
	retval.y = target->position.y + k * d * std::sin(target->position.angle / 180.f * M_PI);

	return retval;
}

void correctAngle(float & x)
{
	while (x > 360.f){ x -= 360.f; } while (x < 0.f){ x += 360.f; }
}


void Player::checkCollisions()
{
	for (auto & player : bulletPlayers_)
	{
		if (player->getID() == getID())
		{
			continue;
		}

		auto & target = player->plane_;
		for (size_t i = 0; i < bullets_.size(); i++)
		{
			int totalDamage = 0;

			auto & bullet = bullets_[i];

			if (bullet.isSpent())
			{
				collisionsRegistrar_.deleteProjectile(bullet.ID);
				bullets_.erase(bullets_.begin() + i);
				i--;
				continue;
			}

			auto dist = rplanes::distance(bullet, target.position);
			if (dist > rplanes::configuration().collisions.rammingDistance)
			{
				continue;
			}
			//in case of collision of any type the DestroyBullet message would be sent to the client
			MDestroyBullets::BulletInfo bi;
			bi.bulletID = bullet.ID;
			bool bulletDestroyed = false;


			std::vector< IntersectionInfo > intersections = getIntersections(bullet, target);

			for (size_t intersectionNo = 0; intersectionNo < intersections.size(); intersectionNo++)
			{
				auto & intersection = intersections[intersectionNo];
				auto & module = target.modules[intersection.moduleNo];
				auto & shape = module->hitZone.shape;
				auto & heightRange = shape.heightRange;

				//just register the border intersection if the bullet is upper or lower than the module
				if ((bullet.z < heightRange.a || bullet.z > heightRange.b))
				{
					collisionsRegistrar_.handleCollision(bullet.ID,
						CollisionsRegistrar::CollisionInfo(target.getId(), intersection.moduleNo));
					continue;
				}

				//otherwise we can tell the border collision occured


				//check if the bullet penetrated the armor
				float theta = (angleFromPoints(bullet, intersection.intersectionPoint)
					- angleFromPoints(intersection.a, intersection.b)) / 180 * M_PI;

				auto penetriation
					= std::abs(sin(theta))
					* bullet.getCurrentPenetration();

				if (penetriation > module->armor
					&& rand() / static_cast<float>(RAND_MAX) > rplanes::configuration().collisions.randomRicochetChance)
				{
					//in case of penetration

					//damage the mdoule
					module->damage(bullet.getCurrentDamage(), getID(), rplanes::planedata::ModuleHP::HIT);
					totalDamage += bullet.getCurrentDamage();

					//changing the bullet parameters
					bullet.speedXY -= bullet.speedXY * module->armor / penetriation;
					boost::random::normal_distribution<float> dist(0.f, rplanes::configuration().collisions.bulletDeflectionSigma);
					bullet.angleXY += dist(gen);

					//set the bullet destroy reason
					bi.reason = MDestroyBullets::BulletInfo::HIT;

					//updating the collision registrar
					collisionsRegistrar_.handleCollision(bullet.ID,
						CollisionsRegistrar::CollisionInfo(target.getId(), intersection.moduleNo));
				}
				else
				{
					//in case of ricochet

					//changing bullet parameters
					bullet.angleXY -= theta * 360 / M_PI;
					bullet.speedXY *= 1 - std::pow(std::abs(sin(theta)), 4.f);


					bullet.prevX = intersection.intersectionPoint.x;
					bullet.prevY = intersection.intersectionPoint.y;

					//rotating the bullet position around the intersection point
					sf::Vector2f bulletPos(bullet.x, bullet.y);
					bulletPos = sf::Transform()
						.rotate(-theta * 360 / M_PI, sf::Vector2f(bullet.prevX, bullet.prevY))
						.transformPoint(bulletPos);

					bullet.x = bulletPos.x;
					bullet.y = bulletPos.y;

					//recalculating intersections
					intersections = getIntersections(bullet, target);
					intersectionNo = 0;

					bi.reason = MDestroyBullets::BulletInfo::RICOCHET;
				}
				//sending the destroyed bullet message
				if (!bulletDestroyed)
				{
					messagesInfo_.destroyedBullets.push_back(bi);
					bulletDestroyed = true;
				}
			}//finished intersection handling

			//////////////////////////////////////////////////////////////////////////

			//handling the bullets hitting top and bottom of the modules

			auto includingModules = collisionsRegistrar_.getIncludingModules(bullet.ID, target.getId());

			auto includingModulesLambda = [&totalDamage, &includingModules, &bulletDestroyed, &target, &bullet, &bi, this](bool bottom)
			{
				if (bottom)
				{
					//sort by increasing of bottom heigh
					std::sort(includingModules.begin(), includingModules.end(), [target](size_t a, size_t b)
					{
						auto & moduleA = target.modules[a];
						auto & moduleB = target.modules[b];
						return moduleA->hitZone.shape.heightRange.a < moduleB->hitZone.shape.heightRange.a;
					});
				}
				else
				{
					//sort by decreasing of top heigh
					std::sort(includingModules.begin(), includingModules.end(), [target](size_t a, size_t b)
					{
						auto & moduleA = target.modules[a];
						auto & moduleB = target.modules[b];
						return moduleA->hitZone.shape.heightRange.b > moduleB->hitZone.shape.heightRange.b;
					});

				}
				for (auto & moduleNo : includingModules)
				{
					auto & module = target.modules[moduleNo];
					float level;
					if (bottom)
						level = module->hitZone.shape.heightRange.a;
					else
						level = module->hitZone.shape.heightRange.b;


					//check if the bullet intersected the module top
					if ((level - bullet.z) * (level - bullet.prevZ) > 0)
					{
						continue;
					}
					float penetration = std::abs(bullet.getCurrentPenetration() * bullet.speedZ / bullet.speedXY) * 2.f;
					if (penetration > module->armor)
					{
						{
							module->damage(bullet.getCurrentDamage(), getID(), rplanes::planedata::ModuleHP::HIT);
							totalDamage += bullet.getCurrentDamage();
						}

						bullet.speedXY -= bullet.speedXY * module->armor / penetration;
						bullet.prevZ = level;

						bi.reason = MDestroyBullets::BulletInfo::HIT;
					}
					else
					{

						bullet.speedZ *= -1;
						bullet.z = (bullet.prevZ + level) / 2.f;

						bi.reason = MDestroyBullets::BulletInfo::RICOCHET;
					}


					if (!bulletDestroyed)
					{
						messagesInfo_.destroyedBullets.push_back(bi);
						bulletDestroyed = true;
					}
				}

			};

			//check tops
			includingModulesLambda(false);

			//check bottoms
			includingModulesLambda(true);

			if (!bullet.isSpent() && bulletDestroyed)
			{
				messagesInfo_.newRicochetes.push_back(bullet);
			}

			//updating statistics
			if (bulletDestroyed)
			{
				if (plane_.nation == target.nation)
				{
					statistics.friendlyDamage += totalDamage;
					statistics.money -= rplanes::configuration().profile.damagePenalty * totalDamage;
				}
				else
				{
					if (bullet.isVirgin())
					{
						bullet.rape();

						statistics.hits++;
						statistics.exp += rplanes::configuration().profile.hitExperience * std::pow(bullet.caliber / 100.f, 2.f);
						player->statistics.hitsReceived++;
					}
					statistics.damage += totalDamage;
					statistics.money += totalDamage * rplanes::configuration().profile.damageReward;
					player->statistics.damageReceived += totalDamage;
				}
			}
		}
	}
}

void Player::addPlayer(std::shared_ptr<Player> Player)
{
	if (Player->plane_.isDestroyed())
	{
		return;
	}
	if (visiblePlayers_.count(Player->id_) != 0)
	{
		return;
	}
	if (rplanes::distance(Player->plane_.position, plane_.position) >
		rplanes::configuration().collisions.visibilityDistance)
	{
		return;
	}
	messages.createPlanes.planes.push_back(
		Player->messagesInfo_.clientPlane);
	visiblePlayers_[Player->id_] = Player;
}

void Player::updatePlayers()
{
	std::vector<size_t> idsToDelete;
	missilePlayers_.clear();
	bulletPlayers_.clear();
	rammingPlayers_.clear();
	for (auto player : visiblePlayers_)
	{
		auto & foreignPlane = player.second->plane_;
		//mark destroyed planes
		if (foreignPlane.isDestroyed())
		{
			idsToDelete.push_back(player.first);
			messages.destroyPlanes.planes.push_back(foreignPlane.getDestructionInfo());
			continue;
		}

		auto dist = rplanes::distance(foreignPlane.position, plane_.position);
		//mark vanished planes
		if (dist >
			rplanes::configuration().collisions.visibilityDistance)
		{
			idsToDelete.push_back(player.first);
			messages.destroyPlanes.planes.push_back(foreignPlane.getDestructionInfo());
		}
		//fill vectors
		if (dist < rplanes::configuration().shooting.maxDistance )
		{
			bulletPlayers_.push_back(player.second);
		}
		if (dist < plane_.interim.maxMissileDistance)
		{
			missilePlayers_.push_back(player.second);
		}
		if (dist < rplanes::configuration().collisions.rammingDistance)
		{
			missilePlayers_.push_back(player.second);
		}
	}
	for (auto ID : idsToDelete)
	{
		visiblePlayers_.erase(ID);
	}
}

bool Player::isDestroyed() const
{
	return plane_.isDestroyed();
}

void Player::clearTemporaryData()
{
	bulletPlayers_.clear();
	missilePlayers_.clear();
	rammingPlayers_.clear();
	messagesInfo_.destroyedBullets.clear();
	messagesInfo_.destroyedMissiles.clear();
	messagesInfo_.newBullets.clear();
	messagesInfo_.newMissiles.clear();
	messagesInfo_.newRicochetes.clear();
	messagesInfo_.updatedModules.clear();

	messages.changeMapMessages.clear();
	messages.createBullets.bullets.clear();
	messages.createMissiles.missiles.clear();
	messages.createPlanes.planes.clear();
	messages.createRicochetes.bullets.clear();
	messages.destroyBullets.bullets.clear();
	messages.destroyMissiles.ids.clear();
	messages.destroyPlanes.planes.clear();
	messages.roomInfos.clear();
	messages.setPlanesPositions.positions.clear();
	messages.stringMessages.clear();
	messages.updateModules.modules.clear();
}

void Player::updateInterfaceMessage()
{
	if (!isDestroyed())
	{
		messages.interfaceData.update(plane_);
	}
}

void Player::shoot(float frameTime, float serverTime, IdGetter & idGetter)
{
	if (plane_.isDestroyed())
	{
		return;
	}
	messagesInfo_.newBullets = plane_.shoot(frameTime, getID(), serverTime);
	for (auto & Bullet : messagesInfo_.newBullets)
	{
		Bullet.ID = idGetter.getID();
	}
	statistics.shots += static_cast<int>(messagesInfo_.newBullets.size());
	bullets_.insert(bullets_.end(), messagesInfo_.newBullets.begin(), messagesInfo_.newBullets.end());
}

void Player::move(float frameTime)
{
	for (auto & Bullet : bullets_)
	{
		Bullet.move(frameTime);
	}

	if (!plane_.isDestroyed())
	{
		plane_.updateInterim();
		plane_.move(frameTime);

		for (auto & Module : plane_.modules)
		{
			Module->hitZone.spin(plane_.position.angle, plane_.position.roll);
		}
	}
	messagesInfo_.clientPlane.init(plane_, this->name, this->getID());
}

bool Player::destroyIfNeed(float frameTime)
{
	if (plane_.isDestroyed())
	{
		return false;
	}
	size_t pos = 0;
	bool retval = false;
	if (!plane_.isFilled())
	{
		destroy(MDestroyPlanes::FUEL, 0);
		retval = true;
	}
	for (auto & module : plane_.modules)
	{
		module->hp.decrementDefectTimer(frameTime);

		if (module->hp.checkConditionChange())
		{
			//sending updated modules message
			MUpdateModules::Module m;
			m.defect = module->hp.isDefected();
			m.hp = module->hp;
			m.moduleNo = pos;
			m.planeID = id_;
			m.reason = module->hp.getReason();
			messagesInfo_.updatedModules.push_back(m);
			//if hp < 0 destroy the plane
			if (module->hp < 0
				&& module->getType() != rplanes::GUN
				&& module->getType() != rplanes::TURRET)
			{
				retval = true;
				if (module->hp.getReason() == rplanes::planedata::ModuleHP::FIRE)
				{
					destroy(MDestroyPlanes::FIRE, pos);
				}
				else
				{
					destroy(MDestroyPlanes::MODULE_DESTROYED, pos);
				}
				break;
			}
		}
		pos++;
	}
	return retval;
}

void Player::destroy(MDestroyPlanes::Reason reason, size_t moduleNo)
{
	plane_.destroy(reason, moduleNo);

	//updating statistics
	auto module = plane_.modules.begin();

	for (auto i = plane_.modules.begin(); i != plane_.modules.end(); i++)
	{
		if ((*i)->hp < (*module)->hp)
		{
			module = i;
		}
	}
	auto & killer = visiblePlayers_.find((*module)->hp.getLastHitClient());

	if (killer != visiblePlayers_.end())
	{
		auto & killerStat = killer->second->statistics;
		auto hp = plane_.getTotalHp();

		if (killer->second->plane_.nation == plane_.nation)
		{
			killerStat.friensDestroyed++;
			killer->second->killingStatistics().friendsDestroyed = killerStat.friensDestroyed;
			killerStat.money -= hp * rplanes::configuration().profile.damagePenalty;
		}
		else
		{
			killerStat.enemyDestroyed++;
			killer->second->killingStatistics().destroyed = killerStat.enemyDestroyed;
			killerStat.money += hp * rplanes::configuration().profile.damageReward;
		}
	}
	statistics.crashes++;
	killingStatistics().crashes = statistics.crashes;
}

void Player::setDestroyed(MDestroyPlanes::Reason reason, size_t moduleNo)
{
	if (!isDestroyed())
		plane_.destroy(reason, moduleNo);
}


void Player::respawn(float x, float y, float angle)
{
	if (!isJoined)
	{
		return;
	}
	plane_.respawn(x, y, angle);
}

void Player::updateStaticalIfNeed()
{
	bool need = false;
	for (auto & Module : plane_.modules)
	{
		if (Module->hp.checkDefectUpdate())
		{
			need = true;
		}
	}
	if (need)
	{
		plane_.updateStatical();
		plane_.updateDependent();
	}
}

bool Player::isZombie()
{
	return
		!isJoined
		&& isDestroyed()
		&& bullets_.size() == 0;
}

size_t Player::getID()
{
	return id_;
}

void Player::setID(size_t id)
{
	id_ = id;
	plane_.setID(id);
}


std::string Player::getGroupName()
{
	return groupName_;
}


rplanes::serverdata::Plane::PlanePosition Player::getPosition()
{
	return plane_.position;
}

rplanes::serverdata::Plane::PlanePosition Player::getPrevPosition()
{
	return plane_.previousPosition;
}

rplanes::Nation Player::getNation()
{
	return plane_.nation;
}

void Player::reload()
{
	plane_.reload(id_);
}

void Player::controlTurrets(float frameTime)
{
	for (auto & turret : plane_.turrets)
	{

		turret.isShooting = false;

		auto RotatedGunsPositions = turret.getRotatedGunsPositions(plane_.position.angle, plane_.position.roll);

		auto centerGunPostion = RotatedGunsPositions[RotatedGunsPositions.size() / 2];

		if (RotatedGunsPositions.size() == 0 || turret.hp < 0)
		{
			continue;
		}
		centerGunPostion.x += plane_.position.x;
		centerGunPostion.y += plane_.position.y;

		//searching enemies in the aim sector
		float gunMaxDistance = turret.gun.getMaxDistance(0);

		auto enemies = lookAround().getEnemies(gunMaxDistance);
		auto targets = lookAround().getPlanesInSector(enemies, turret.startAngle, turret.sector);
		if (targets.size() == 0)
		{
			turret.aimAngle = turret.startAngle;
			continue;
		}
		//selecting the closest target
		DestroyablePlane * nearestTarget = targets.front();
		for (auto & target : targets)
		{
			if (distance(nearestTarget->position, centerGunPostion) > distance(target->position, centerGunPostion))
			{
				nearestTarget = target;
			}
		}

		auto & targetPosition = nearestTarget->position;


		if (distance(targetPosition, centerGunPostion) > gunMaxDistance)
		{
			continue;
		}
		//handling the aim timer

		if (turret.aimTimer < 0.f)
		{
			//reset the timer
			boost::normal_distribution<float>
				timerDist(rplanes::configuration().turrets.aimTimerMean,
				rplanes::configuration().turrets.aimTimerSigma);

			turret.aimTimer = timerDist(gen);

			//setting aim error
			boost::normal_distribution<float>
				errorDist(0.f, rplanes::configuration().turrets.aimErrorSigma);

			turret.aimError.x = errorDist(gen);
			turret.aimError.y = errorDist(gen);

			rplanes::PointXY & aimError = turret.aimError;
			//the ideal conditions for aiming is a firing at a close target that is pursuing us with the same speed
			//while our plane is flying horizontally

			//speed difference penalty
			aimError = aimError * (1.f + std::abs(plane_.target.V - nearestTarget->target.V) / plane_.target.V);

			//direction difference penalty
			float angleResidual = std::abs(plane_.position.angle - targetPosition.angle);
			if (angleResidual > 180.f)
				angleResidual = 360.f - angleResidual;

			aimError = aimError * (1.f + angleResidual / 180.f);

			//roll penalty
			aimError = aimError * (1.f + std::abs(plane_.position.roll) / 90.f);
			//distance penalty
			aimError = aimError * (1.f + distance(centerGunPostion, targetPosition) / 300.f);
		}
		else
			turret.aimTimer -= frameTime;

		//calculating deflected point
		auto deflectedPoint = getDeflectedPoint(nearestTarget, rplanes::PointXY(centerGunPostion.x, centerGunPostion.y), turret.gun);

		deflectedPoint = deflectedPoint + turret.aimError;

		//turning the turret and shooting
		float targetDistance = distance(centerGunPostion, deflectedPoint);
		if (targetDistance < gunMaxDistance)
		{
			float aimAngle = angleFromPoints(centerGunPostion, deflectedPoint) - plane_.position.angle;
			float angleResidual = turret.aimAngle - aimAngle;

			correctAngle(angleResidual);


			if (angleResidual > 180.f)
			{
				turret.aimAngle += (360.f - angleResidual) * frameTime * rplanes::configuration().turrets.aimIntense;
			}
			else
			{
				turret.aimAngle -= angleResidual * frameTime * rplanes::configuration().turrets.aimIntense;
			}


			turret.aimDistance += (targetDistance - turret.aimDistance) * frameTime * rplanes::configuration().turrets.aimIntense;

			//shooting
			if (angleResidual < rplanes::configuration().turrets.aimExp
				|| angleResidual > 360.f - rplanes::configuration().turrets.aimExp)
			{

				if (turret.shootTimer < 0.f && turret.cooldownTimer < 0.f)
				{
					boost::normal_distribution<float> 
						shootTimeDist( rplanes::configuration().turrets.shootTimeMean, 
							rplanes::configuration().turrets.shootTimeSigma),

						cooldownTimeDist( rplanes::configuration().turrets.cooldownTimeMean,
							rplanes::configuration().turrets.cooldownTimeSigma);

					turret.shootTimer = shootTimeDist(gen);
					turret.cooldownTimer = cooldownTimeDist(gen);
				}
			}
			if (turret.shootTimer > 0.f)
			{
				turret.shootTimer -= frameTime;
				turret.isShooting = true;
			}
			else if (turret.cooldownTimer > 0.f)
			{
				turret.cooldownTimer -= frameTime;
			}
		}




		//correcting angles
		float leftBorder = turret.startAngle - turret.sector / 2.f,
			rightBorder = turret.startAngle + turret.sector / 2.f;

		correctAngle(leftBorder);
		correctAngle(rightBorder);
		correctAngle(turret.aimAngle);
		correctAngle(turret.startAngle);

		//check the aim is in the aim sector
		{
			float angleResidual = std::abs(turret.aimAngle - turret.startAngle);
			if (angleResidual > 180.f)
				angleResidual = 360.f - angleResidual;

			if (angleResidual > turret.sector / 2.f)
			{
				turret.aimAngle = turret.startAngle;
			}
		}

		//forbid fire through the plane
		if (plane_.position.roll > 15.f)
		{
			if (turret.gunsPosition.z < 0.f && turret.aimAngle < 180.f)
				turret.aimAngle = 360.f - turret.aimAngle;
			if (turret.gunsPosition.z > 0.f && turret.aimAngle > 180.f)
				turret.aimAngle = 360.f - turret.aimAngle;
		}
		if (plane_.position.roll < -15.f)
		{
			if (turret.gunsPosition.z < 0.f && turret.aimAngle > 180.f)
				turret.aimAngle = 360.f - turret.aimAngle;
			if (turret.gunsPosition.z > 0.f && turret.aimAngle < 180.f)
				turret.aimAngle = 360.f - turret.aimAngle;
		}


		//check if the line from the turret to the aim position intersects any module
		if (!turret.isShooting)
		{
			continue;
		}

		rplanes::PointXYZ a(centerGunPostion);
		rplanes::PointXYZ b(a);
		a.x += rplanes::configuration().turrets.gunLength * std::cos((turret.aimAngle + plane_.position.angle) / 180.0 * M_PI);
		a.y += rplanes::configuration().turrets.gunLength * std::sin((turret.aimAngle + plane_.position.angle) / 180.0 * M_PI);
		a.z *= 1 - rplanes::configuration().turrets.gunLength / turret.aimDistance;

		b.x += turret.aimDistance * std::cos((turret.aimAngle + plane_.position.angle) / 180.0 * M_PI);
		b.y += turret.aimDistance * std::sin((turret.aimAngle + plane_.position.angle) / 180.0 * M_PI);
		b.z = 0.f;

		//check framework cockpit wing or engine
		ContainersMerger< rplanes::planedata::Module  > cm;
		cm.addContainer(plane_.wings);
		cm.addContainer(plane_.engines);
		cm.addElement(plane_.framework);
		cm.addElement(plane_.tail);
		cm.addElement(plane_.cabine);
		cm.for_each([&a, &b, &turret, this](rplanes::planedata::Module & module)
		{
			auto & checkCollisionsLambda = [&a, &b, this](const rplanes::planedata::Module & module)->bool
			{
				auto & points = module.hitZone.shape.points;

				if (points.size() < 2)
				{
					return false;
				}
				for (int j = 0; j < points.size(); j++)
				{
					rplanes::PointXY pointB1(points[j].x, points[j].y);
					size_t k = (j + 1) % points.size();
					rplanes::PointXY pointB2(points[k].x, points[k].y);

					pointB1.x += plane_.position.x ;
					pointB1.y += plane_.position.y;
					pointB2.x += plane_.position.x;
					pointB2.y += plane_.position.y;

					rplanes::PointXY intersectionPoint;
					if (getLineSegmentsIntersection(a, b, pointB1, pointB2, intersectionPoint))
					{
						float intersectionZ = a.z * distance(b, intersectionPoint) / distance(a, b);
						if (intersectionZ > module.hitZone.shape.heightRange.a  && intersectionZ < module.hitZone.shape.heightRange.b)
						{
							return true;
						}
					}
				}
				return false;
			};
			//if the intersection found forbid the fire
			if (checkCollisionsLambda(module))
			{
				turret.isShooting = false;
			}
		});
	}
}

CombatSituation Player::lookAround()
{
	return CombatSituation(*this);
}

void DestroyablePlane::setID(size_t id)
{
	id_ = id;
}

void DestroyablePlane::destroy(MDestroyPlanes::Reason reason, size_t modulePos)
{
	destructionInfo_.moduleNo = modulePos;
	destructionInfo_.reason = reason;
	destructionInfo_.planeID = id_;
	if (modulePos < modules.size())
		destructionInfo_.killerId = modules[modulePos]->hp.getLastHitClient();
	else
		destructionInfo_.killerId = destructionInfo_.planeID;

	destroyed_ = true;
}

void DestroyablePlane::respawn(float x, float y, float angle)
{
	destructionInfo_.reason = MDestroyPlanes::VANISH;
	destructionInfo_.planeID = id_;
	destructionInfo_.moduleNo = 0;
	destructionInfo_.killerId = id_;
	reload(id_);
	destroyed_ = false;


	target.angularVelocity = 0.f;
	target.angularAcceleration = 0.f;
	target.faintTimer = -1.f;
	target.acceleration = 0.f;
	target.aimSize = 0.f;
	target.faintVal = 0.f;

	updateStatical();
	target.V = statical.Vmax;
	position.x = x;
	position.y = y;
	position.angle = angle;
	previousPosition = position;
}

bool DestroyablePlane::isDestroyed() const
{
	return destroyed_;
}

MDestroyPlanes::DestroyedPlane DestroyablePlane::getDestructionInfo()
{
	return destructionInfo_;
}

DestroyablePlane::DestroyablePlane(const Plane & playerPlane) : rplanes::serverdata::Plane(playerPlane)
{
	initModules();
	destroyed_ = true;
	destructionInfo_.nation = nation;
	destructionInfo_.planeID = id_;
}

int DestroyablePlane::getTotalHp() const
{
	int retval = 0;
	for (auto & module : modules)
	{
		if (module->hp > 0)
		{
			retval += module->hp;
		}
	}
	return retval;
}

void DestroyablePlane::move(float frameTime)
{
	previousPosition = position;
	rplanes::serverdata::Plane::move(frameTime);
}

size_t DestroyablePlane::getId()
{
	return id_;
}

std::vector<DestroyablePlane *> CombatSituation::getPlanesInSector(std::vector<DestroyablePlane *> planes, float angle, float sector)
{
	std::vector<DestroyablePlane *> retval;

	for (auto & plane : planes)
	{
		float innerAngle = (rplanes::angleFromPoints(player_.getPosition(), plane->position)
			- rplanes::angleFromPoints(player_.getPrevPosition(), player_.getPosition()));
		while (innerAngle < 0.f)
		{
			innerAngle += 360.f;
		}
		while (angle < 0.f)
		{
			angle += 360.f;
		}
		while (angle > 360.f)
		{
			angle -= 360.f;
		}
		float angleResidual = std::abs(innerAngle - angle);
		if (angleResidual > 180.f)
			angleResidual = 360.f - angleResidual;

		if (angleResidual < sector / 2.f)
		{
			retval.push_back(plane);
		}
	}
	return retval;
}


DestroyablePlane & CombatSituation::getThisPlane()
{
	return player_.plane_;
}

std::vector< DestroyablePlane * > CombatSituation::getFriends(float radius /*= -1.f*/)
{

	std::vector< DestroyablePlane * > retval;
	if (radius < 0.f)
	{
		for (auto & visiblePlayer : player_.visiblePlayers_)
		{
			if (visiblePlayer.second->getNation() == player_.getNation() && visiblePlayer.second->getID() != player_.getID())
			{
				retval.push_back(&(visiblePlayer.second->plane_));
			}
		}
	}
	else for (auto & visiblePlayer : player_.visiblePlayers_)
	{
		if (rplanes::distance(visiblePlayer.second->getPosition(), player_.getPosition()) < radius)
		{
			if (visiblePlayer.second->getNation() == player_.getNation() && visiblePlayer.second->getID() != player_.getID())
			{
				retval.push_back(&(visiblePlayer.second->plane_));
			}
		}
	}
	return retval;
}

std::vector< DestroyablePlane * > CombatSituation::getEnemies(float radius /*= -1.f */)
{
	std::vector< DestroyablePlane * > retval;
	if (radius < 0.f)
	{
		for (auto & visiblePlayer : player_.visiblePlayers_)
		{
			if (visiblePlayer.second->getNation() != player_.getNation())
			{
				retval.push_back(&(visiblePlayer.second->plane_));
			}
		}
	}
	else for (auto & visiblePlayer : player_.visiblePlayers_)
	{
		if (rplanes::distance(visiblePlayer.second->getPosition(), player_.getPosition()) < radius)
		{
			if (visiblePlayer.second->getNation() != player_.getNation())
			{
				retval.push_back(&(visiblePlayer.second->plane_));
			}
		}
	}
	return retval;
}