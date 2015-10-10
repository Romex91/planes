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

	void clear();

	size_t getAlivePlayerNumber();

	std::pair<size_t, size_t> getPlayerNumber();

	std::vector< std::shared_ptr<Player> > getPlayers();

	void addPlayer(std::shared_ptr<Player> player);

	HumanGroup(std::string name, std::shared_ptr<SpawnPoint> spawnPoint, rplanes::Nation nation, size_t maxPlayerNumber);

private:

	void deleteEmptyPlayers();
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

	virtual size_t getAlivePlayerNumber();;

	virtual std::vector< std::shared_ptr<Player> > getPlayers();
	void initialize( IdGetter & idGetter );

	void control(float frameTime, BotTargetsStorage & bts);
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

	//running when the game starts
	std::vector<ScriptText> startScripts;

	//running when a new player joins the room
	std::vector<ScriptText> joinScripts;

	//running when intersecting a trigger
	std::vector<TriggerScript> triggerScripts;

	//running when a player is killed
	std::vector<KilledScript> killedScripts;

	//running by timer
	std::vector<TimerScript> timerScripts;

	void load(std::string filename);
};