#pragma once
#include "stdafx.h"
#include "player.h"
#include "bot.h"

class ScriptLine
{
public:
	std::string command;
	std::vector<std::string> options;
	ScriptLine(std::string line);
};


class SpawnPoint
{
public:

	SpawnPoint(float x, float y, float angle);
	void spawn(std::vector< std::shared_ptr<Player> > players, float timeDelay);
	void spawn(std::shared_ptr<Player> player, float timeDelay);
	void decrementSpawnTimer(float frameTime);
	void clear();
private:
	typedef std::pair< std::shared_ptr<Player>, float > SpawnPair;
	std::vector<SpawnPair> spawnQueue;
	float coolDown;
	float x_, y_, angle_;
};

class Trigger
{
public:
	float radius, x, y;
};

class PlayerGroup
{
public:
	SpawnPoint & spawnPoint()
	{
		if (!spawnPoint_)
		{
			throw rplanes::eRoomError(" ��������� �� ������� ������ ����. ");
		}
		return *spawnPoint_;
	}

	rplanes::Nation getNation()
	{
		return nation_;
	}

	std::string getName()
	{
		return name_;
	}

	virtual size_t getAlivePlayerNumber() = 0;

	virtual std::vector< std::shared_ptr<Player> > getPlayers() = 0;

	PlayerGroup(std::string name, std::shared_ptr<SpawnPoint> spawnPoint, rplanes::Nation nation) :
		name_(name),
		spawnPoint_(spawnPoint),
		nation_(nation)
	{}
protected:
	std::string name_;
	std::shared_ptr<SpawnPoint>	spawnPoint_;
	rplanes::Nation nation_;

};

class HumanGroup : public PlayerGroup
{
public:

	void clear()
	{
		players_.clear();
	}

	size_t getAlivePlayerNumber()
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

	std::pair<size_t, size_t> getPlayerNumber()
	{
		deleteEmptyPlayers();
		return std::pair< size_t, size_t >(players_.size(), maxPlayerNumber_);
	}

	std::vector< std::shared_ptr<Player> > getPlayers()
	{
		std::vector< std::shared_ptr<Player> > retval;
		deleteEmptyPlayers();
		for (auto & player : players_)
		{
			retval.push_back(player.lock());
		}
		return retval;
	}

	void addPlayer(std::shared_ptr<Player> player)
	{
		if (!player)
		{
			throw rplanes::eRoomError("� ������ ������� ������ ���������. ");
		}
		players_.push_back(player);
		player->groupName_ = name_;
	}

	HumanGroup(std::string name, std::shared_ptr<SpawnPoint> spawnPoint, rplanes::Nation nation, size_t maxPlayerNumber) :
		PlayerGroup(name, spawnPoint, nation),
		maxPlayerNumber_(maxPlayerNumber)
	{}

private:

	void deleteEmptyPlayers()
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
	std::vector< std::weak_ptr<Player> > players_;
	size_t			maxPlayerNumber_;

};

class BotGroup : public PlayerGroup
{
public:
	BotGroup(std::string name, 
	std::shared_ptr<SpawnPoint> spawnPoint, 
	size_t nBots,
	std::string planeName,
	Bot::Type botType);

	virtual size_t getAlivePlayerNumber()
	{
		if ( initialized == false )
		{
			throw rplanes::eRoomError("���������� ������������� ������ �����.");
		}
		size_t retval = 0;
		for ( auto & bot : bots_ )
		{
			if (!bot.getPlayer()->isDestroyed())
			{
				retval++;
			}
		}
		return retval;
	};

	virtual std::vector< std::shared_ptr<Player> > getPlayers()
	{
		if (initialized == false)
		{
			throw rplanes::eRoomError("���������� ������������� ������ �����.");
		}
		std::vector< std::shared_ptr<Player> > retval;
		for (auto & bot : bots_)
		{
			retval.push_back(bot.getPlayer());
		}
		return retval;
	}
	void initialize( IdGetter & idGetter )
	{
		for ( auto & bot: bots_ )
		{
			size_t id = idGetter.getID();
			bot.getPlayer()->setID(id);
			std::stringstream ss;
			ss << "bot" << id;
			bot.getPlayer()->name = ss.str();
		}
		initialized = true;
	}

	void control(float frameTime, BotTargetsStorage & bts)
	{
		for ( auto & bot : bots_ )
		{
			bot.control( frameTime, bts );
			bot.getPlayer()->clearTemporaryData();
		}
	}
private:
	bool initialized;
	std::vector< Bot > bots_;
};

class Map
{
public:
	std::string name;
	rplanes::PointXY size;

	class TriggerScript
	{
	public:
		enum Type
		{
			ENTER,
			ESCAPE
		}type;
		std::string trigger;
		std::string group;
		std::vector<ScriptLine> text;
	};

	class KilledScript
	{
	public:
		std::string group;
		std::vector<ScriptLine> text;
	};

	class TimerScript
	{
	public:
		std::vector<ScriptLine> text;
		TimerScript(float period);
		bool decrementTimer(float frameTime);
		void restart();
	private:
		float period_;
		float timer_;
	};
	std::map< std::string, std::shared_ptr< SpawnPoint > > spawnPoints;
	std::map< std::string, Trigger > triggers;

	std::vector< HumanGroup > humanGroups;

	std::vector< BotGroup > botGroups;

	std::map< std::string, int> variables;

	std::map< std::string, Goal > goals;

	typedef std::vector<ScriptLine> ScriptText;

	//������ ����������� ��� ������ ����
	std::vector<ScriptText> startScripts;

	//������ ����������� ��� ������������� ������ ������
	std::vector<ScriptText> joinScripts;

	//������ ����������� ��� ����������� ������� ��������
	std::vector<TriggerScript> triggerScripts;

	//������ ����������� ��� �������� ������
	std::vector<KilledScript> killedScripts;

	//������ ����������� ����������
	std::vector<TimerScript> timerScripts;

	void load(std::string filename);
};