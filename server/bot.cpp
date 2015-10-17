#include "bot.h"
boost::random::mt19937 gen;

using namespace rplanes;

Bot::Bot(std::shared_ptr<Player> player, Bot::Type type)
{

	player_ = player;

	switch (type)
	{
	case Bot::EASY:
		condition_ = std::shared_ptr< BotCondition >(new botconditions::easy::Patrol);
		break;
	case Bot::HARD:
		condition_ = std::shared_ptr< BotCondition >(new botconditions::easy::Patrol);
		break;
	case Bot::PEACEFULL:
		condition_ = std::shared_ptr< BotCondition >(new botconditions::Peacefull);
		break;
	default:
		break;
	}
	condition_->initializeConditionList();


}

std::shared_ptr< Player > Bot::getPlayer()
{
	return player_;
}

void Bot::control(float frameTime, BotTargetsStorage & botTargetsStorage)
{
	condition_->control(*player_, frameTime, botTargetsStorage);
	auto newCondition = condition_->handleConditionList(*player_);
	if (newCondition)
	{
		condition_ = newCondition;
		condition_->initializeConditionList();
	}
}

std::shared_ptr<BotCondition> BotCondition::handleConditionList(Player & player)
{
	for (auto & condition : conditionList_)
	{
		if (condition.first(player))
		{
			return condition.second;
		}
	}
	return std::shared_ptr<BotCondition>();
}

void BotCondition::turnToPoint(Player & player,
	rplanes::PointXY point,
	/*уровень обморока до которого будет происходить набор угловой скорости. от 0 до 100 */
	unsigned short maxFaint,
	/*интенсивность поворота */
	unsigned short turnValue,
	/*точность следования направлению */
	float angleExp)
{
	auto position = player.getPosition();
	float innerAngle = (rplanes::angleFromPoints(position, point)
		- rplanes::angleFromPoints(player.getPrevPosition(), position));
	while (innerAngle < 0.f)
	{
		innerAngle += 360;
	}

	if (innerAngle > 180.f)
		controllable_.turningVal = -turnValue;
	else
		controllable_.turningVal = turnValue;

	if (player.messages.interfaceData.faintVal > maxFaint || innerAngle < angleExp || innerAngle > 360 - angleExp)
	{
		controllable_.turningVal = 0;
	}
}

void BotCondition::accelerate(
	Player & player,
	unsigned short speed,
	float exp,
	short increaseValue,
	short decreaseValue)
{
	auto & interfaceData = player.messages.interfaceData;
	//если скорость ниже требуемой
	if (interfaceData.V < speed)
	{
		//наращиваем мощность двигателя пока возможно
		bool ableToIncreasePower = true;
		for (auto & thermometer : interfaceData.thermometers)
		{
			if (thermometer.temperature > exp * thermometer.criticalTemperature)
			{
				ableToIncreasePower = false;
			}
			if (thermometer.dT > exp * thermometer.dTmax)
			{
				ableToIncreasePower = false;
			}
		}
		if (ableToIncreasePower)
		{
			controllable_.power += increaseValue;
		}
		else
		{
			controllable_.power -= decreaseValue;
		}
	}
	else
	{
		controllable_.power -= decreaseValue;
	}

	if (controllable_.power > 100)
	{
		controllable_.power = 100;
	}
	if (controllable_.power < 0)
	{
		controllable_.power = 0;
	}
}




void botconditions::easy::Patrol::initializeConditionList()
{
	conditionList_.clear();
	std::pair < std::function< bool(Player &) >, std::shared_ptr<BotCondition> > attackCondition;
	attackCondition.first = [this](Player & player)->bool
	{
		for (auto & updatedModule : player.messages.updateModules.modules)
		{
			if (updatedModule.reason == rplanes::planedata::ModuleHP::HIT && updatedModule.planeID == player.getID())
			{
				return true;
			}
		}

		for (auto & destroyedPlane : player.messages.destroyPlanes.planes)
		{
			if ((destroyedPlane.reason == rplanes::network::MDestroyPlanes::MODULE_DESTROYED
				|| destroyedPlane.reason == rplanes::network::MDestroyPlanes::FIRE)
				&& destroyedPlane.nation == player.getNation())
			{
				return true;
			}
		}

		auto enemies = player.lookAround().getEnemies();

		if ( player.lookAround().getPlanesInSector( enemies, watchAngle_, 30.f).size() != 0)
		{
			return true;
		}
		return false;
	};
	attackCondition.second = std::shared_ptr<BotCondition>(new Attack);
	conditionList_.push_back(attackCondition);

	std::pair < std::function< bool(Player &) >, std::shared_ptr<BotCondition> > flee;
	flee.first = [this](Player & player)->bool
	{
		if (std::abs(controllable_.turningVal) < 10)
		{
			auto friends = player.lookAround().getFriends( configuration().bots.rammingWarningDistance);
			if ( player.lookAround().getPlanesInSector( friends, 0, 60).size() != 0)
			{
				return true;
			}
		}
		return false;
	};
	flee.second.reset(new Flee);
	conditionList_.push_back(flee);

	watchAngle_ = rand() / static_cast<float>(RAND_MAX)* 180.f - 90.f;
}

void botconditions::easy::Patrol::control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage)
{
	turnToPoint(player, player.goal.getValue().position, 70, 70, 10.f);
	accelerate(player, player.goal.getValue().recommendedSpeed, 0.6f, 1, 1);
	if (watchAngle_ < -90.f || watchAngle_ > 90.f)
	{
		watchAngle_ = -90.f;
	}
	watchAngle_ += configuration().bots.easyPatrolScanSpeed * frameTime;
}



void botconditions::easy::Flee::initializeConditionList()
{
	conditionList_.clear();
	std::pair < std::function< bool(Player &) >, std::shared_ptr<BotCondition> > patrol;
	patrol.first = [this](Player & player)->bool
	{
		return fleeTimer_ < 0;
	};
	patrol.second.reset(new Patrol);
	conditionList_.push_back(patrol);

	boost::random::normal_distribution<float> dist(configuration().bots.easyFleeTimeMean, configuration().bots.easyFleeTimeSigma);
	fleeTimer_ = dist(gen);
	fleeTurnVal_ = rand() / static_cast<float>(RAND_MAX)* 40 + 30;
	if (rand() / static_cast<float>(RAND_MAX) > 0.5f)
	{
		fleeTurnVal_ *= -1;
	}
}

void botconditions::easy::Flee::control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage)
{
	controllable_.turningVal = fleeTurnVal_;
	player.goal.getValue();
	accelerate(player, player.goal.getValue().recommendedSpeed, 0.9f, 1, 1);
	fleeTimer_ -= frameTime;
}



void botconditions::easy::Attack::initializeConditionList()
{
	conditionList_.clear();

	std::pair < std::function< bool(Player &) >, std::shared_ptr<BotCondition> > patrolCondition,
		escapeCondition;

	patrolCondition.first = [this](Player & player)->bool
	{
		return player.lookAround().getEnemies().size() == 0 || targetNo_ < 0;
	};
	patrolCondition.second = std::shared_ptr<BotCondition>(new Patrol);
	conditionList_.push_back(patrolCondition);

	escapeCondition.first = [this](Player & player)->bool
	{
		for (auto & destroyedPlane : player.messages.destroyPlanes.planes)
		{
			if (destroyedPlane.nation == player.getNation() &&
				(destroyedPlane.reason == rplanes::network::MDestroyPlanes::MODULE_DESTROYED
				|| destroyedPlane.reason == rplanes::network::MDestroyPlanes::FIRE))
			{
				if (player.lookAround().getEnemies().size() >= player.lookAround().getFriends().size()
					&& rand() < configuration().bots.easyPanicChance * RAND_MAX)
				{
					return true;
				}
			}
		}
		return false;
	};
	escapeCondition.second = std::shared_ptr<BotCondition>(new Escape);
	conditionList_.push_back(escapeCondition);

	targetNo_ = -1;
	selectTimer_ = -1.f;
}

void botconditions::easy::Attack::control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage)
{
	selectTimer_ -= frameTime;

	auto enemies = player.lookAround().getEnemies();
	auto friends = player.lookAround().getFriends();
	if (enemies.size() == 0)
	{
		return;
	}
	//если картина боя меняется, переопределяем цель
	if (player.messages.createPlanes.planes.size() != 0 || player.messages.destroyPlanes.planes.size() != 0 || targetNo_ < 0 || selectTimer_ < 0.f)
	{
		boost::random::normal_distribution<float> dist(configuration().bots.easyTargetSelectionTimeMean, configuration().bots.easyTargerSelectionTimeSigma);
		selectTimer_ = dist(gen);
		selectTarget(player, enemies, botTargetsStorage);
	}
	if (targetNo_ < 0)
	{
		return;
	}
	auto & target = *enemies[targetNo_ % enemies.size()];

	//сдвигаем рамку прицела непосредственно на цель
	
	controllable_.shootingDistanceOffset
		= (rplanes::distance(player.getPosition(), target.position) - player.messages.interfaceData.shootingDistance)* 
		configuration().bots.easyShootingDistanceAgility * frameTime;

	//выбираем точку упреждения ориентируясь по собственной скорости
	rplanes::PointXY deflectedTarget(target.position.x, target.position.y);

	deflectedTarget.x += player.messages.interfaceData.aimSize * std::cos(target.position.angle / 180.f * M_PI);
	deflectedTarget.y += player.messages.interfaceData.aimSize * std::sin(target.position.angle / 180.f * M_PI);

	//доворачиваем к точке упреждения
	turnToPoint(player, deflectedTarget, 80, 80, 1.f);

	//если угол прицеливания верен, производим стрельбу
	float innerAngle = (rplanes::angleFromPoints(player.getPosition(), deflectedTarget)
		- rplanes::angleFromPoints(player.getPrevPosition(), player.getPosition()));

	while ( innerAngle < 0.f )
	{
		innerAngle += 360.f;
	}
	if ( innerAngle > 180.f  )
	{
		innerAngle = 360.f - innerAngle;
	}

	bool shootAbility = std::abs(rplanes::distance(player.getPosition(), deflectedTarget) - player.messages.interfaceData.shootingDistance)
		< player.messages.interfaceData.aimSize;

	if ( innerAngle < configuration().bots.easyAttackSector
		&& shootAbility)
	{
		if (player.lookAround().getPlanesInSector(
			player.lookAround().getFriends(player.messages.interfaceData.shootingDistance)
			, 0	, 30)
			.size() == 0)
		{
			controllable_.isShooting = true;
		}
	}
	else
	{
		controllable_.isShooting = false;
	}
	//полный вперед
	if (shootAbility)
		accelerate(player, target.target.V + 1.f, 0.8f, 10, 10);
	else
		accelerate(player, target.target.V + 20.f, 0.8f, 1, 1);
}

void botconditions::easy::Attack::selectTarget(Player & player,
	std::vector< DestroyablePlane * > & targets,
	BotTargetsStorage & botTargetsStorage)
{
	targetNo_ = 0;
	if (player.isDestroyed())
	{
		botTargetsStorage.deselect(player.getID());
		return;
	}
	bool targetFound = false;

	for (int i = 0; i < targets.size(); i++)
	{
		if (botTargetsStorage.countAttackers(targets[i]->getId()) > configuration().bots.easyMaxAttakersCount
			&& botTargetsStorage.getTarget(player.getID()) != targets[i]->getId())
		{
			continue;
		}

		float angle = angleFromPoints(player.getPosition(), targets[i]->position);

		if (targetNo_ == 0 || 
			rplanes::distance(targets[targetNo_]->position, player.getPosition()) >
			rplanes::distance(targets[i]->position, player.getPosition()) )
		{
			targetFound = true;
			targetNo_ = i;
			botTargetsStorage.selectTarget(player.getID(), targets[i]->getId());
		}
	}
	if (!targetFound)
	{
		targetNo_ = -1;
		botTargetsStorage.deselect(player.getID());
	}
}




void botconditions::easy::Escape::initializeConditionList()
{
	conditionList_.clear();
	std::pair < std::function< bool(Player &) >, std::shared_ptr<BotCondition> > patrolCondition;
	patrolCondition.first = [this](Player & player)
	{
		return player.lookAround().getEnemies().size() == 0 || panicTime_ < 0;
	};
	patrolCondition.second = std::shared_ptr<BotCondition>(new Patrol);
	conditionList_.push_back(patrolCondition);

	boost::random::normal_distribution<float> dist(configuration().bots.easyPanicTimeMean,
		configuration().bots.easyPanicTimeSigma);
	panicTime_ = dist(gen);
}

void botconditions::easy::Escape::control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage)
{
	panicTime_ -= frameTime;
	controllable_.turningVal = 0;
	auto enemies = player.lookAround().getEnemies();
	for (auto & enemy : enemies)
	{
		float innerAngle = (rplanes::angleFromPoints(enemy->position, player.getPosition())
			- rplanes::angleFromPoints(player.getPrevPosition(), player.getPosition()));

		while (innerAngle < 0.f)
		{
			innerAngle += 360;
		}

		if (innerAngle > 180.f)
			controllable_.turningVal -= 50;
		else
			controllable_.turningVal += 50;
	}
	controllable_.power = 100;
}




botconditions::Peacefull::Peacefull()
{
	hitCoolDown = -1.f;
	hitTurn = 0;
}

void botconditions::Peacefull::control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage)
{
	if (player.isDestroyed())
	{
		return;
	}
	controllable_.isShooting = false;
	controllable_.launchMissile = false;

	bool needTurn = false;
	{
		for (auto & updatedModule : player.messages.updateModules.modules)
		{
			if (updatedModule.reason == rplanes::planedata::ModuleHP::HIT && updatedModule.planeID == player.getID())
			{
				if (static_cast<float> (rand()) / RAND_MAX * 1 > 0.6f)
				{
					needTurn = true;
				}
			}
		}

		for (auto & destroyedPlane : player.messages.destroyPlanes.planes)
		{
			if (destroyedPlane.reason == rplanes::network::MDestroyPlanes::MODULE_DESTROYED
				&& destroyedPlane.nation == player.getNation())
			{
				needTurn = true;
			}
		}
	}
	if (needTurn)
	{
		hitCoolDown = 5 * static_cast<float>(rand()) / RAND_MAX;
		hitTurn = static_cast<float>(rand()) / RAND_MAX * 200 - 100;
	}

	if (hitCoolDown < 0)
	{
		turnToPoint(player, player.goal.getValue().position, 30, 50, 3);
	}
	else
	{
		controllable_.turningVal = hitTurn;
		hitCoolDown -= frameTime;
	}

	bool rammingWarning = false;
	{
		auto friends = player.lookAround().getFriends( rplanes::configuration().collisions.rammingDistance * 1.5f);

		if (player.lookAround().getPlanesInSector(friends, 0, 60).size() != 0)
		{
			rammingWarning = true;
		}

	}

	if (rammingWarning)
	{
		accelerate(player, player.messages.interfaceData.Vmin, 0.8f, 1, 10);
	}
	else
	{
		accelerate(player, player.goal.getValue().recommendedSpeed, 0.8f, 10, 1);
	}
}

void botconditions::Peacefull::initializeConditionList()
{
	conditionList_.clear();
}


void BotTargetsStorage::clear()
{
	targets.clear();
	nAttackersMap.clear();
}

size_t BotTargetsStorage::getTarget(size_t botId)
{
	auto i = targets.find(botId);
	if (i != targets.end())
	{
		return i->second;
	}
	return botId;
}

size_t BotTargetsStorage::countAttackers(size_t botId)
{
	auto i = nAttackersMap.find(botId);
	if (i != nAttackersMap.end())
	{
		return i->second;
	}
	return 0;
}

void BotTargetsStorage::deselect(size_t botId)
{
	selectTarget(botId, botId);
}

void BotTargetsStorage::selectTarget(size_t botId, size_t targetId)
{
	size_t comparableValue;
	auto compare = [&comparableValue](const std::pair<size_t, size_t> & i)->bool
	{
		return i.second == comparableValue && i.second != i.first;
	};


	//если бот имел цель, пересчитываем количество игроков, целящихся в нее
	if (targets.count(botId) != 0)
	{
		comparableValue = targets[botId];
		nAttackersMap[targets[botId]] = count_if(targets.begin(), targets.end(), compare);
	}

	//присваиваем новую цель
	targets[botId] = targetId;

	//пересчитываем количество игроков, целящихся в новую цель
	comparableValue = targetId;
	nAttackersMap[targetId] = count_if(targets.begin(), targets.end(), compare);
}
