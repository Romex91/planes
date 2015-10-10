#include "room.h"
using namespace rplanes;

void Room::iterate( float frameTime, float serverTime )
{
	deleteUnlogined();
	std::vector< std::shared_ptr<Player> > handleList;

	ContainersMerger<PlayerGroup> groups;
	groups.addContainer(map_.humanGroups);
	groups.addContainer(map_.botGroups);
	groups.for_each([ &handleList ](PlayerGroup & group)
	{
		auto players = group.getPlayers();
		handleList.insert(handleList.end(), players.begin(), players.end());
	});

	checkMapEscapes(handleList);

	for (int i = 0; i < handleList.size(); i++)
	{
		auto & player = *handleList[i];
		for (auto & otherPlayer : handleList)
		{
			player.addPlayer(otherPlayer);
		}
		player.updatePlayers();
		player.updateStaticalIfNeed();
		player.move(frameTime);
		player.updateInterfaceMessage();
		player.controlTurrets(frameTime);
		player.shoot( frameTime, serverTime, projectileIDGetter );
	}
	for (auto &player : handleList)
	{
		player->checkCollisions();
	}
	for (int i = 0; i < handleList.size(); i++)
	{
		auto & player = *handleList[i];
		if (player.destroyIfNeed(frameTime))
		{
			handleDeath(handleList[i]);
		}
	}

	for (auto & spawnPoint : map_.spawnPoints)
	{
		spawnPoint.second->decrementSpawnTimer(frameTime);
	}
	handleTriggers();
	handleTimers(frameTime);
	for (int i = 0; i < handleList.size(); i++)
	{
		auto & player = *handleList[i];
		player.updatePositionsMessage( serverTime );
		player.updateModulesMessage();
		player.updateProjectilesMessages( serverTime );
	}
	updateRoomInfoMessage(handleList);

	for ( auto & botGroup : map_.botGroups )
	{
		botGroup.control(frameTime, botTargetsSorage_);
	}

}

void Room::deleteUnlogined()
{
	for ( int i = 0; i < players_.size(); i++ )
	{
		if ( players_[i]->isZombie() )
		{
			roomInfo_().disconnectedPlayers.push_back(players_[i]->name);
			players_.erase(players_.begin()+i);
			i--;
		}
	}
}

std::map< rplanes::Nation, rplanes::network::servermessages::hangar::RoomList::RoomInfo::SlotInfo > Room::getPlayerNumber()
{
	std::map< rplanes::Nation, rplanes::network::servermessages::hangar::RoomList::RoomInfo::SlotInfo > retval;
	for ( auto & group : map_.humanGroups )
	{
		retval[group.getNation()].nPlayersMax += group.getPlayerNumber().second;
	}
	for ( auto & player : players_ )
	{
		retval[player->getNation()].nPlayers++;
	}
	return retval;
}

void Room::addPlayer( std::shared_ptr< Player > player )
{
	bool added = false;
	for ( auto & group: map_.humanGroups )
	{
		if ( group.getPlayerNumber().first < group.getPlayerNumber().second && group.getNation() == player->getNation() )
		{
			group.addPlayer(player);
			added = true;
			break;
		}
	}
	if ( !added )
	{
		throw PlanesException(_rstrw("Cannot add player. Room is filled."));
	}
	
	player->setID( playerIDGetter.getID() );
	players_.push_back(player);

	for ( auto & script :  map_.joinScripts )
	{
		executeScript(script, player);
	}

	std::vector< std::shared_ptr<Player> > handleList;
	{
		ContainersMerger<PlayerGroup> groups;
		groups.addContainer(map_.humanGroups);
		groups.addContainer(map_.botGroups);
		groups.for_each([&handleList](PlayerGroup & group)
		{
			auto players = group.getPlayers();
			handleList.insert(handleList.end(), players.begin(), players.end());
		});
	}

	servermessages::room::RoomInfo::NewPlayerInfo newPlayerInfo;
	newPlayerInfo.name = player->name;
	newPlayerInfo.nation = player->getNation();
	newPlayerInfo.planeName = player->getPlaneName();
	roomInfo_().newPlayers.push_back(newPlayerInfo);

	servermessages::room::RoomInfo roomInfoMessage;
	for ( auto & i: handleList )
	{
		newPlayerInfo.name = i->name;
		newPlayerInfo.nation = i->getNation();
		newPlayerInfo.planeName = i->getPlaneName();
		roomInfoMessage.newPlayers.push_back(newPlayerInfo);

		roomInfoMessage.updatedStatistics[i->name] = i->killingStatistics.getValue();
	}

	roomInfoMessage.goal.position = player->goal.getValue().position;
	roomInfoMessage.goal.description = player->goal.getValue().description;
	roomInfoMessage.goal.recommendedSpeed = player->goal.getValue().recommendedSpeed;

	player->messages.roomInfos.push_back(roomInfoMessage);

	checkPlayersOpenedMaps();
}

void Room::executeScript(const std::vector<ScriptLine> & script, std::shared_ptr<Player> thisPlayer)
{
	auto getOperandValue = [this]( const  std::string & operand)->int
	{
		int retval;

		auto variable = map_.variables.find(operand);

		auto group = std::find_if(map_.humanGroups.begin(),
			map_.humanGroups.end(), [&operand]( HumanGroup  group)
		{
			if (group.getName() == operand)
				return true;
			return false;
		});

		auto botGroup = std::find_if(map_.botGroups.begin(),
			map_.botGroups.end(), [&operand](BotGroup  group)
		{
			if (group.getName() == operand)
				return true;
			return false;
		});

		if (variable == map_.variables.end())
		{
			if (group != map_.humanGroups.end())
			{
				retval = static_cast<int>(group->getAlivePlayerNumber());
			}
			else if ( botGroup!= map_.botGroups.end() )
			{
				retval = static_cast<int>(botGroup->getAlivePlayerNumber());
			}
			else
			{
				retval = atoi(operand.c_str());
			}
		}
		else
		{
			retval = variable->second;
		}

		return retval;
	};
	auto getGroupPlayers = [this, &thisPlayer]( const std::string & groupName)->std::vector<std::shared_ptr<Player>>
	{
		bool groupFound = false;
		std::vector<std::shared_ptr<Player>> retval;
		if ( groupName == "this" )
		{
			if ( !thisPlayer )
			{
				throw PlanesException(_rstrw("Script error. this does not exist."));
			}
			retval.push_back(thisPlayer);
			groupFound = true;
		}

		ContainersMerger<PlayerGroup> groups;
		groups.addContainer(map_.humanGroups);
		groups.addContainer(map_.botGroups);
		groups.for_each([&groupFound, &groupName, &retval](PlayerGroup & group)
		{
			if (group.getName() == groupName || groupName == "all")
			{
				auto playersToAdd = group.getPlayers();
				retval.insert(retval.begin(), playersToAdd.begin(), playersToAdd.end());
				groupFound = true;
			}
		});
		if ( !groupFound )
		{
			throw PlanesException(_rstrw("Script error. Group {0} is not found.", groupName));
		}
		return retval;
	};

	for (auto & scriptLine : script)
	{
		auto & command = scriptLine.command;
		auto & options = scriptLine.options;

		auto checkOptionsSize = [&options, &command](size_t minSize)
		{
			if (options.size() < minSize)
			{
				throw PlanesException(_rstrw("Script error. Wrong arguments {0}.", command));
			}
		};

		if (
			command == "+"
			|| command == "-"
			|| command == "*"
			|| command == "/"
			|| command == "="
			|| command == "%")
		{
			checkOptionsSize(2);
			//searching the left variable
			auto left = map_.variables.find(options[0]);
			if (left == map_.variables.end())
			{
				throw PlanesException(_rstrw("Script error. Variable {0} is not found.", options[0]));
			}

			//get the right value
			int right = getOperandValue(options[1]);
			
			//execute operation
			switch (command[0])
			{
			case '=':
				left->second = right;
				break;
			case '-':
				left->second -= right;
				break;
			case '+':
				left->second += right;
				break;
			case '*':
				left->second *= right;
				break;
			case '/':
				left->second /= right;
				break;
			case  '%':
				left->second %= right;
			default:
				break;
			}
		}


		else if (command == ">"
			|| command == "=="
			|| command == "!="
			|| command == "<")
		{
			checkOptionsSize(2);
			int left = getOperandValue(options[0]);
			int right = getOperandValue( options[1] );

			switch (command[0])
			{
			case '>':
				if (left <= right)
				{
					return;
				}
				break;
			case '<':
				if (left >= right)
				{
					return;
				}
				break;
			case '=':
				if (left != right)
				{
					return;
				}
				break;
			case '!':
				if (left == right)
				{
					return;
				}
				break;
			default:
				break;
			}

		}
		else if (command == "regroup")
		{
			regroup();
		}
		else if (command == "open")
		{
			checkOptionsSize(3);
			auto players = getGroupPlayers(options[0]);
			if ( options[1] == "map" )
			{

				for ( auto & player : players )
				{
					if ( player->openedMaps.count( options[2] ) == 0 )
					{
						rplanes::network::bidirectionalmessages::ResourceStringMessage message;
						message.string =  _rstrw("Map {0} is unlocked.", options[2]);
						player->messages.stringMessages.push_back(message);
						player->openedMaps.insert(options[2]);
					}
				}
			}
			else
			{
				for (auto & player : players)
				{
					if (player->openedPlanes.count(options[2]) == 0)
					{
						rplanes::network::bidirectionalmessages::ResourceStringMessage message;
						message.string = _rstrw("Plane {0} is unlocked.", options[2]);
						player->messages.stringMessages.push_back(message);
						player->openedPlanes.insert(options[2]);
					}
				}
			}
		}
		else if (command == "spawn")
		{
			checkOptionsSize(2);
			auto players = getGroupPlayers(options[0]);
			for ( auto & player : players )
			{
				if (player->isDestroyed())
				{
					spawn(player, atof(options[1].c_str()));
				}
			}
		}
		else if (command == "respawn")
		{
			checkOptionsSize(2);
			auto players = getGroupPlayers(options[0]);
			for (auto & player : players)
			{
				spawn(player, atof(options[1].c_str()));
			}
		}
		else if (command == "award")
		{
			checkOptionsSize(2);
			auto players = getGroupPlayers(options[0]);
			for (auto & player : players)
			{
				player->statistics.money += atoi(options[1].c_str());
			}
		}
		else if (command == "penalty")
		{
			checkOptionsSize(2);
			auto players = getGroupPlayers(options[0]);
			for (auto & player : players)
			{
				player->statistics.money += atoi(options[1].c_str());
			}
		}
		else if (command == "reload")
		{
			checkOptionsSize(1);
			auto players = getGroupPlayers(options[0]);
			for (auto & player : players)
			{
				player->reload();
			}
		}
		else if (command == "restart")
		{
			restart();
		}
		else if (command == "goal")
		{
			checkOptionsSize(2);
			auto players = getGroupPlayers(options[0]);
			if ( map_.goals.count(options[1]) == 0 )
			{
				throw PlanesException(_rstrw("{0} is not found.", options[1]));
			}
			for (auto & player : players)
			{
				player->goal() = map_.goals[options[1]] ;
			}
		}
		else if (command == "message")
		{
			checkOptionsSize(2);

			std::string messageText;
			for (auto i = options.begin() + 1; i < options.end(); i++)
			{
				std::string s;
				if ( (*i)[0] == '%' )
				{
					std::string name;
					name.insert( name.end() ,i->begin() + 1, i->end());
					if ( name == "this" )
					{
						if ( !thisPlayer )
						{
							throw PlanesException(_rstrw("this does not exist."));
						}
						s = thisPlayer->name;
					}
					else
					{
						std::stringstream ss;
						ss << getOperandValue(name);
						s = ss.str();
					}
				}
				else
				{
					s = *i;
				}

				messageText += s + " ";
			}
			auto players = getGroupPlayers(options[0]);
			for ( auto & player : players )
			{
				rplanes::network::bidirectionalmessages::ResourceStringMessage message;
				message.string = _rstrw("{0}", messageText);
				player->messages.stringMessages.push_back(message);
			}
		}
		else if (command == "kill" || command == "setDestroyed")
		{
			checkOptionsSize(2);
			auto players = getGroupPlayers(options[0]);
			for (auto & player : players)
			{
				std::string reasonString = options[1];
				typedef rplanes::network::servermessages::room::DestroyPlanes::Reason Reason;
				Reason reason;
				size_t moduleNo = 0;
				if ( reasonString == "FIRE")
				{
					checkOptionsSize(3);
					moduleNo = atoi(options[2].c_str());
					reason = Reason::FIRE;
				}
				else if (reasonString == "FUEL")
				{
					reason = Reason::FUEL;
				}
				else if (reasonString == "MODULE_DESTROYED")
				{
					checkOptionsSize(3);
					moduleNo = atoi(options[2].c_str());
					reason = Reason::MODULE_DESTROYED;

				}
				else if (reasonString == "VANISH")
				{
					reason = Reason::VANISH;
				}
				else if (reasonString == "RAMMED")
				{
					checkOptionsSize(3);
					moduleNo = atoi(options[2].c_str());
					reason = Reason::RAMMED;
				}
				else
				{
					throw PlanesException(_rstrw("Script error. Unknown argument. {0}", reasonString));
				}
				if ( command == "kill" )
				{
					player->destroy(reason, moduleNo);
					handleDeath(player);
				}
				if ( command == "setDestroyed" )
				{
					player->setDestroyed(reason, moduleNo);
				}
			}
		}
		else if( command.size()!=0)
		{
			if ( command[0] != '/' )
			{
				throw PlanesException(_rstrw("Script error. Unknown command. {0}", command));
			}
		}

	}
}

void Room::handleDeath(std::shared_ptr<Player> player)
{
	std::vector< Map::ScriptText > scripts;
	std::for_each(map_.killedScripts.begin(),
		map_.killedScripts.end(),
		[&player, &scripts](Map::KilledScript killedScript)
	{
		if (killedScript.group == player->getGroupName())
			scripts.push_back(killedScript.text);
	});

	for (auto script : scripts)
	{
		executeScript(script, player);
	}
}

void Room::spawn(std::shared_ptr<Player> player, float timeDelay)
{
	size_t groupsFound = 0;

	ContainersMerger<PlayerGroup> groups;
	groups.addContainer(map_.humanGroups);
	groups.addContainer(map_.botGroups);
	groups.for_each([&groupsFound, &player, &timeDelay](PlayerGroup & group)
	{
		if (group.getName() == player->getGroupName())
		{
			group.spawnPoint().spawn(player, timeDelay);
			groupsFound++;
		}
	});
	if (groupsFound == 0)
	{
		throw PlanesException(_rstrw("{0} is unknown group.", player->getGroupName()));
	}
	else if ( groupsFound > 1 )
	{
		throw PlanesException(_rstrw("{0} is ambiguous.", player->getGroupName()));
	}
}

void Room::handleTriggers()
{
	for (auto & script : map_.triggerScripts)
	{
		//searching the trigger
		if ( map_.triggers.count(script.trigger) == 0 )
		{
			throw PlanesException(_rstrw("{0} is not found.", script.trigger));
		}

		auto & trigger = map_.triggers[script.trigger];

		std::vector< std::shared_ptr< Player > > players;

		size_t groupFound = false;

		ContainersMerger<PlayerGroup> groups;
		groups.addContainer(map_.humanGroups);
		groups.addContainer(map_.botGroups);
		groups.for_each([&script, & players, &groupFound](PlayerGroup & group)
		{
			if (script.group == "any" || script.group == group.getName())
			{
				auto playersToAdd = group.getPlayers();
				players.insert(players.begin(), playersToAdd.begin(), playersToAdd.end());
				groupFound = true;
			}
		});
		if ( !groupFound )
		{
			throw PlanesException(_rstrw("{0} is unknown group.", script.group));
		}

		for (auto & player : players)
		{
			if ( player->isDestroyed() )
			{
				continue;
			}
			//check the trigger intersection
			if ((rplanes::distance(player->getPosition(), trigger) - trigger.radius)
				* (rplanes::distance(player->getPrevPosition(), trigger) - trigger.radius)
			> 0)
			{
				continue;
			}
			//check the trigger type
			if (rplanes::distance(player->getPosition(), trigger) < trigger.radius && script.type == Map::TriggerScript::ENTER
				|| rplanes::distance(player->getPosition(), trigger) > trigger.radius && script.type == Map::TriggerScript::ESCAPE)
			{
				executeScript(script.text, player);
			}
		}
	}
}

void Room::restart()
{
	botTargetsSorage_.clear();
	for ( auto & spawnPoint : map_.spawnPoints )
	{
		spawnPoint.second->clear();
	}
	for ( auto & timerScript: map_.timerScripts )
	{
		timerScript.restart();
	}
	for ( auto & script : map_.startScripts )
	{
		executeScript( script, std::shared_ptr<Player>());
	}
}


void Room::regroup()
{
	for (auto & group : map_.humanGroups)
	{
		group.clear();
	}
	for (auto & player : players_)
	{
		size_t startGroupNo = static_cast<size_t>(static_cast<float>(rand()) / RAND_MAX * map_.humanGroups.size() * 2);
		size_t i = 0;
		bool added = false;
		for (i = 0; i < map_.humanGroups.size(); i++)
		{
			size_t groupNo = ( startGroupNo + i ) % map_.humanGroups.size();
			if (map_.humanGroups[groupNo].getNation() == player->getNation()
				&& map_.humanGroups[groupNo].getPlayerNumber().first < map_.humanGroups[i].getPlayerNumber().second)
			{
				map_.humanGroups[groupNo].addPlayer(player);
				added = true;
				break;
			}
		}
		if (!added)
		{
			rplanes::network::bidirectionalmessages::ResourceStringMessage tm;
			tm.string = _rstrw("Room is full.");
			player->messages.stringMessages.push_back(tm);
			player->isJoined = false;
		}
	}
}


Room::~Room()
{
	for (auto & player : players_)
	{
		player->isJoined = false;
	}
}

void Room::kickPlayers(std::vector< std::string > & names)
{
	for (size_t i = 0; i < players_.size(); i++)
	{
		for (auto & name : names)
		{
			if (name == players_[i]->name)
			{
				rplanes::network::bidirectionalmessages::ResourceStringMessage tm;
				
				tm.string = _rstrw("You are banned by {0}", creator);

				players_[i]->messages.stringMessages.push_back(tm);
				players_[i]->destroy(rplanes::network::servermessages::room::DestroyPlanes::FIRE, 0);
				players_[i]->isJoined = false;
				break;
			}
		}
	}
}

void Room::changeMap(std::string mapName)
{
	for (auto & player : players_)
	{
		player->setDestroyed(rplanes::network::servermessages::room::DestroyPlanes::VANISH, 0);
	}
	map_.load(mapName);
	rplanes::network::servermessages::room::ChangeMap changeMapMessage;
	changeMapMessage.mapName = mapName;
	for (auto & player : players_)
	{
		player->messages.changeMapMessages.push_back(changeMapMessage);
	}
	for (auto & group : map_.botGroups)
	{
		group.initialize(playerIDGetter);
	}
	regroup();
	checkPlayersOpenedMaps();
}

void Room::setPlayerDestroyed(std::string playerName)
{
	for (auto & player : players_)
	{
		if (player->name == playerName)
		{
			player->setDestroyed(rplanes::network::servermessages::room::DestroyPlanes::FIRE, 0);
			handleDeath(player);
		}
	}
}

void Room::clearTemroraryData()
{
	for (int i = 0; i < players_.size(); i++)
	{
		auto & player = *players_[i];
		player.clearTemporaryData();
	}
}

void Room::handleTimers(float frameTime)
{
	for (auto & timerScript : map_.timerScripts)
	{
		if (timerScript.decrementTimer(frameTime))
		{
			executeScript(timerScript.text, std::shared_ptr<Player >());
		}
	}
}

void Room::checkMapEscapes( std::vector<std::shared_ptr<Player>> players )
{
	for (auto & player : players)
	{
		if (player->isDestroyed())
		{
			continue;
		}


		auto position = player->getPosition();
		float innerAngle = ( rplanes::angleFromPoints(position, map_.size * 0.5f)
			- rplanes::angleFromPoints(player->getPrevPosition(), position) );
		while ( innerAngle < 0.f )
		{
			innerAngle += 360;
		}

		//assuming control if the player is out of the room border
		if (
			position.x < 0
			|| position.y < 0
			|| position.x > map_.size.x
			|| position.y > map_.size.y)
		{
			rplanes::serverdata::Plane::ControllableParameters controllable;
			controllable.isShooting = false;
			controllable.launchMissile = false;
			controllable.power = 0;

			if (innerAngle > 180.f)
				controllable.turningVal = -100;
			else
				controllable.turningVal = 100;

			if (player->messages.interfaceData.faintVal > 70 || innerAngle < 10 || innerAngle > 350)
			{
				controllable.turningVal = 0;
			}
			player->setControllable(controllable);
		}
	}
}

void Room::checkPlayersOpenedMaps()
{
	for (auto & player : players_)
	{
		if ( player->openedMaps.count( map_.name ) == 0 )
		{
			rplanes::network::bidirectionalmessages::ResourceStringMessage message;

			message.string = _rstrw("Map {0} is closed. A client application should check opened maps in a user profile.", map_.name) ;
			player->messages.stringMessages.push_back(message);
			player->isJoined = false;
		}
	}
}

void Room::updateRoomInfoMessage(std::vector<std::shared_ptr<Player>> players)
{
	for (auto & player : players)
	{
		if (player->killingStatistics.isUpdated())
		{
			roomInfo_().updatedStatistics[player->name] = player->killingStatistics.getValue();
			player->killingStatistics.confirmUpdate();
		}
	}
	
	for (auto & player : players)
	{
		if (roomInfo_.isUpdated() || player->goal.isUpdated())
		{
			auto ri = roomInfo_.getValue();
			ri.goal.position = player->goal.getValue().position;
			ri.goal.description = player->goal.getValue().description;
			ri.goal.recommendedSpeed = player->goal.getValue().recommendedSpeed;
			player->messages.roomInfos.push_back(ri);
			player->goal.confirmUpdate();
		}
	}
	roomInfo_().clear();
	roomInfo_.confirmUpdate();
}


