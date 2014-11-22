#pragma once
#include "stdafx.h"
#include "room.h"

using namespace rplanes::network;


//определить статус по id
bool clientIsInRoom(size_t clientID );
//получить положение в векторе из id
size_t convertIDToPos( size_t clienID );
size_t convertPosToID( size_t posInVector, bool inRoom  );

//класс клиента.
//ограничения:
//Клиенты не могут иметь доступ к другим клиентам. Состояние клиента не должно влиять на состояние других клиентов
//Обращения к клиенту могут быть осуществлены только последовательно
class Client
{
public:
	friend class Server;

	Client( boost::asio::io_service& io_service , size_t clientID = 0 );
	~Client();
	void setID( size_t id );

	void sendMessage( Message & mess );

	void setControllable( rplanes::serverdata::Plane::ControllableParameters controllable );

	void login( std::string name, std::string password );

	void logout();

	void prepareRoomExit();

	ClientStatus getStatus();

	rplanes::playerdata::Profile & profile();

private:

	//отправить комнатные данные (позиции самолетов и т.п.)
	void sendRoomMessages();

	//запускается из ангарной петли перед занесением клиента в очередь комнаты
	void joinRoom( Room& room, size_t planeNo );

	//запускается из петли комнат при выходе из них
	void exitRoom();

	struct ProfilesInfo
	{
		std::set<std::string> loginedProfiles;
		Mutex Mutex;
		size_t clientsCount;
		ProfilesInfo()
		{
			clientsCount = 0;
		}
	};

	static ProfilesInfo profilesInfo_;
	size_t id_; 
	float disconnectTimer_;
	ClientStatus status_;
	Connection connection_;

	rplanes::playerdata::Profile profile_;
	std::shared_ptr< Player > player_;
};
