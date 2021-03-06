
#include "map.h"

extern std::shared_ptr<odb::database> planesDB;

using namespace rplanes;


void Map::load(std::string filename)
{
	std::fstream fs;
	fs.open(std::string("../Resources/maps/") + filename, std::ios_base::in);
	if (!fs)
	{
		throw RPLANES_EXCEPTION("{0} is not found", filename);
	}

	humanGroups.clear();
	botGroups.clear();
	joinScripts.clear();
	killedScripts.clear();
	name = filename;
	spawnPoints.clear();
	startScripts.clear();
	triggers.clear();
	triggerScripts.clear();
	variables.clear();

	std::string line;
	std::stringstream ss;
	while (std::getline(fs, line))
	{
		ScriptLine scriptLine(line);
		auto & command = scriptLine.command;
		if (command.size() == 0)
		{
			continue;
		}
		auto & options = scriptLine.options;

		auto checkOptionsSize = [&options, &command](size_t size)
		{
			if (options.size() < size)
			{
				throw RPLANES_EXCEPTION("Script error. Wrong arguments count. {0}", command );
			}
		};

		if (command == "mapSize")
		{
			checkOptionsSize(2);
			size.x = atoi(options[0].c_str());
			size.y = atoi(options[1].c_str());
		}
		else if (command == "spawnPoint")
		{
			checkOptionsSize(4);

			spawnPoints[options[0]].reset(
				new SpawnPoint(
				atof(options[1].c_str()),
				atof(options[2].c_str()),
				atof(options[3].c_str())));
		}
		else if (command == "trigger")
		{
			checkOptionsSize(4);
			triggers[options[0]].x = atof(options[1].c_str());
			triggers[options[0]].y = atof(options[2].c_str());
			triggers[options[0]].radius = atof(options[3].c_str());
		}
		else if (command == "group")
		{
			checkOptionsSize(4);
			rplanes::Nation nation = AXIS;
			if (options[1] == "allies")
			{
				nation = ALLIES;
			}

			auto spawnPoint = spawnPoints.find(options[2]);

			if (spawnPoint == spawnPoints.end())
			{
				throw RPLANES_EXCEPTION("Script error. {0} is not found", options[2]);
			}

			HumanGroup newGroup(
				options[0],
				spawnPoint->second,
				nation,
				atoi(options[3].c_str()));
			humanGroups.push_back(newGroup);
		}
		else if (command == "botGroup")
		{
			checkOptionsSize(5);

			Bot::Type botType;

			if (options[1] == "peacefull")
			{
				botType = Bot::PEACEFULL;
			}
			else if (options[1] == "easy")
			{
				botType = Bot::EASY;
			}
			else
			{
				botType = Bot::HARD;
			}

			auto spawnPoint = spawnPoints.find(options[3]);
			if (spawnPoint == spawnPoints.end())
			{
				throw RPLANES_EXCEPTION("Script error. {0} is not found", options[3]);
			}
			BotGroup newGroup(options[0], spawnPoint->second, atoi(options[4].c_str()), options[2], botType);
			botGroups.push_back(newGroup);
		}
		else if (command == "var")
		{
			checkOptionsSize(1);
			variables[options[0]] = atoi(options[1].c_str());
		}
		else if (command == "goal")
		{
			checkOptionsSize(2);
			Goal goal(atof(options[1].c_str()), atof(options[2].c_str()), atoi(options[3].c_str()));
			for_each(options.begin() + 3, options.end(), [&goal](std::string word)
			{
				goal.description += " " + word;
			});
			goals[options[0]] = goal;
		}
		else if (command == "start")
		{
			Map::ScriptText newScript;
			std::string subline;
			while (std::getline(fs, subline))
			{
				ScriptLine scriptSubLine(subline);
				if (scriptSubLine.command == "end")
				{
					break;
				}
				newScript.push_back(scriptSubLine);
			}
			startScripts.push_back(newScript);
		}
		else if (command == "join")
		{
			Map::ScriptText newScript;
			std::string subline;
			while (std::getline(fs, subline))
			{
				ScriptLine scriptSubLine(subline);
				if (scriptSubLine.command == "end")
				{
					break;
				}
				newScript.push_back(scriptSubLine);
			}
			joinScripts.push_back(newScript);
		}
		else if (command == "triggered")
		{
			checkOptionsSize(3);

			TriggerScript newScript;
			if (options[0] == "enter")
			{
				newScript.type = TriggerScript::ENTER;
			}
			else
			{
				newScript.type = TriggerScript::ESCAPE;
			}
			newScript.trigger = options[1];
			newScript.group = options[2];

			std::string subline;
			while (std::getline(fs, subline))
			{
				ScriptLine scriptSubLine(subline);
				if (scriptSubLine.command == "end")
				{
					break;
				}
				newScript.text.push_back(scriptSubLine);
			}
			triggerScripts.push_back(newScript);
		}
		else if (command == "timer")
		{
			checkOptionsSize(1);


			TimerScript newScript(atof(options[0].c_str()));
			std::string subline;
			while (std::getline(fs, subline))
			{
				ScriptLine scriptSubLine(subline);
				if (scriptSubLine.command == "end")
				{
					break;
				}
				newScript.text.push_back(scriptSubLine);
			}
			timerScripts.push_back(newScript);
		}
		else if (command == "killed")
		{
			checkOptionsSize(1);

			KilledScript newScript;
			newScript.group = options[0];

			std::string subline;
			while (std::getline(fs, subline))
			{
				ScriptLine scriptSubLine(subline);
				if (scriptSubLine.command == "end")
				{
					break;
				}
				newScript.text.push_back(scriptSubLine);
			}
			killedScripts.push_back(newScript);
		}
	}
}

ScriptLine::ScriptLine(std::string line)
{
	std::stringstream ss(line);
	if (ss >> command)
	{
		std::string option;
		while (ss >> option)
		{
			options.push_back(option);
		}
	}
}




SpawnPoint::SpawnPoint(float x, float y, float angle) : x_(x), y_(y), angle_(angle)
{
	coolDown = 0;
}

void SpawnPoint::decrementSpawnTimer(float frameTime)
{
	if (coolDown >= 0)
	{
		coolDown -= frameTime;
	}
	for (size_t i = 0; i < spawnQueue.size(); i++)
	{
		spawnQueue[i].second -= frameTime;
		if (spawnQueue[i].second < 0 && coolDown < 0)
		{
			spawnQueue[i].first->respawn(x_, y_, angle_);
			spawnQueue.erase(spawnQueue.begin() + i);
			i--;
			coolDown += configuration().server.spawnCooldownTime;
		}
	}
}

void SpawnPoint::spawn(std::shared_ptr<Player> player, float timeDelay)
{
	if (!player)
	{
		throw RPLANES_EXCEPTION("Empty pointer in SpawnPoint::spawn.");
	}
	spawnQueue.push_back(SpawnPair(player, timeDelay));
}

void SpawnPoint::spawn(std::vector< std::shared_ptr<Player> > players, float timeDelay)
{
	for (auto & player : players)
	{
		spawn(player, timeDelay);
	}
}

void SpawnPoint::clear()
{
	spawnQueue.clear();
}

Map::TimerScript::TimerScript(float period) : period_(period), timer_(period)
{

}

bool Map::TimerScript::decrementTimer(float frameTime)
{
	timer_ -= frameTime;
	if (timer_ < 0)
	{
		restart();
		return true;
	}
	return false;
}

void Map::TimerScript::restart()
{
	timer_ = period_;
}

BotGroup::BotGroup(std::string name, std::shared_ptr<SpawnPoint> spawnPoint, size_t nBots, std::string planeName, Bot::Type botType) : PlayerGroup(name, spawnPoint, rplanes::AXIS)
{
	rplanes::playerdata::Profile profile;
	profile.money = std::numeric_limits<int>::max();
	if (botType == Bot::HARD)
	{
		auto exp = rplanes::configuration().bots.hardStartExpiriance;
		profile.pilot.addExp(exp);
		profile.pilot.up_endurance(exp / 4);
		profile.pilot.up_engine(exp / 4);
		profile.pilot.up_flight(exp / 4);
		profile.pilot.up_shooting(exp / 4);
	}
	profile.openedPlanes.insert(planeName);
	rstring::_rstrw_t errorMessage = profile.buyPlane(planeName, planesDB);
	if (profile.planes.size() == 0)
	{
		throw PlanesException(errorMessage);
	}

	auto plane = profile.planes.front().buildPlane(profile.pilot, planesDB);

	nation_ = plane.nation;
	for (size_t i = 0; i != nBots; i++)
	{
		bots_.push_back(Bot(std::shared_ptr<Player>(new Player(plane, "bot")), botType));
		bots_.back().getPlayer()->groupName_ = name_;
	}
	initialized = false;
}

void BotGroup::control(float frameTime, BotTargetsStorage & bts)
{
	for (auto & bot : bots_)
	{
		bot.control(frameTime, bts);
		bot.getPlayer()->clearTemporaryData();
	}
}

void BotGroup::initialize(IdGetter & idGetter)
{
	for (auto & bot : bots_)
	{
		size_t id = idGetter.getID();
		bot.getPlayer()->setID(id);
		std::stringstream ss;
		ss << "bot" << id;
		bot.getPlayer()->name = ss.str();
	}
	initialized = true;
}

std::vector< std::shared_ptr<Player> > BotGroup::getPlayers()
{
	if (initialized == false)
	{
		throw RPLANES_EXCEPTION("Bot group should be initialized first.");
	}
	std::vector< std::shared_ptr<Player> > retval;
	for (auto & bot : bots_)
	{
		retval.push_back(bot.getPlayer());
	}
	return retval;
}

size_t BotGroup::getAlivePlayerNumber()
{
	if (initialized == false)
	{
		throw RPLANES_EXCEPTION("Bot group should be initialized first.");
	}
	size_t retval = 0;
	for (auto & bot : bots_)
	{
		if (!bot.getPlayer()->isDestroyed())
		{
			retval++;
		}
	}
	return retval;
}

void HumanGroup::deleteEmptyPlayers()
{
	for (size_t i = 0; i < players_.size(); i++)
	{
		if (!players_[i].lock())
		{
			players_.erase(players_.begin() + i);
			i--;
		}
	}
}

HumanGroup::HumanGroup(std::string name, std::shared_ptr<SpawnPoint> spawnPoint, rplanes::Nation nation, size_t maxPlayerNumber) :
PlayerGroup(name, spawnPoint, nation),
maxPlayerNumber_(maxPlayerNumber)
{

}

void HumanGroup::addPlayer(std::shared_ptr<Player> player)
{
	player->groupName_ = name_;
	players_.push_back(player);
}

std::vector< std::shared_ptr<Player> > HumanGroup::getPlayers()
{
	std::vector< std::shared_ptr<Player> > retval;
	deleteEmptyPlayers();
	for (auto & player : players_)
	{
		retval.push_back(player.lock());
	}
	return retval;
}

std::pair<size_t, size_t> HumanGroup::getPlayerNumber()
{
	deleteEmptyPlayers();
	return std::pair< size_t, size_t >(players_.size(), maxPlayerNumber_);
}

size_t HumanGroup::getAlivePlayerNumber()
{
	size_t retval = 0;
	deleteEmptyPlayers();
	for (auto &player : players_)
	{
		if (!player.lock()->isDestroyed())
		{
			retval++;
		}
	}
	return retval;
}

void HumanGroup::clear()
{
	players_.clear();
}
