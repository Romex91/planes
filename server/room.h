#pragma once
#include "stdafx.h"
#include "player.h"
#include "map.h"




class Room
{
public:
	friend class consoleHandler;

	std::string creator;
	std::string description;
	std::set< std::string > banlist;

	IdGetter projectileIDGetter,
		playerIDGetter;

	~Room();

	Room(std::string mapName = "bot.map")
	{ 
		changeMap(mapName);
	}

	void addPlayer(std::shared_ptr< Player > player);

	std::map< rplanes::Nation, rplanes::network::servermessages::hangar::RoomList::RoomInfo::SlotInfo > getPlayerNumber();

	void deleteUnlogined();

	void iterate(float frameTime, float serverTime);

	void restart();

	void regroup();

	void executeScript(const std::vector<ScriptLine> & script, std::shared_ptr<Player> thisPlayer);

	void kickPlayers(std::vector< std::string > & names);

	void changeMap( std::string mapName );

	void setPlayerDestroyed( std::string playerName );

	void clearTemroraryData();

private:

	BotTargetsStorage botTargetsSorage_;

	void checkPlayersOpenedMaps();

	void checkMapEscapes(std::vector<std::shared_ptr<Player>> players);

	void handleTimers( float frameTime );

	void handleDeath( std::shared_ptr<Player> player );

	void spawn(std::shared_ptr<Player> player, float timeDelay);

	void handleTriggers();

	void updateRoomInfoMessage(std::vector<std::shared_ptr<Player>> players);

	Map map_;

	//блокируется ТОЛЬКО во время обработки входящих сообщений клиентов
	Mutex roomMutex_;
	std::vector< std::shared_ptr< Player > > players_;

	updatable< servermessages::room::RoomInfo > roomInfo_;
};