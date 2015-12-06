#pragma once
#include "stdafx.h"
#include "room.h"

using namespace rplanes::network;


//limitations:
//clients cannot access other clients. Client condition cannot affect other clients
//this class is singlethreaded
class Client : private boost::noncopyable
{
public:
	friend class Server;

	Client(std::shared_ptr<Connection> connection);
	~Client();
	size_t getId() 
	{
		return id_;
	}

	void sendMessage(const Message & mess)
	{
		connection_->sendMessage(mess);
	}

	void setControllable( rplanes::serverdata::Plane::ControllableParameters controllable );

	void login( std::string name, std::string password );

	void logout();

	void prepareRoomJoin(const MJoinRoomRequest & message);

	void prepareRoomExit();

	ClientStatus getStatus();

	rplanes::playerdata::Profile & profile();

	void endSession()
	{
		try
		{
			switch (getStatus())
			{
			case rplanes::network::UNLOGGED:
				break;
			case rplanes::network::HANGAR:
				logout();
				break;
			case rplanes::network::ROOM:
				onExitRoom();
				logout();
				break;
			}
			connection_->close();
		}
		catch (std::exception & e)
		{
			std::wcout << _rstrw("Unexpected error deleting client. {0}", e.what()).str() << std::endl;
		}
		std::wcout << _rstrw("Lost connection. {0}", connection_->getIP()).str() << std::endl;
	}

	std::string getRoomName()
	{
		return _roomToJoin;
	}

	std::shared_ptr<Player> sharePlayer()
	{
		return player_;
	}
private:


	void setMessageHandlers();

	void sendRoomMessages();

	//call this method when leaving the room
	//saves statistics resets player and sending a message to the client side
	void onExitRoom();

	struct ProfilesInfo
	{
		std::set<std::string> loggedInProfiles;
		Mutex Mutex;
		size_t clientsCount = 0;
	};

	static ProfilesInfo profilesInfo_;
	static IdGetter _idGetter;
	const size_t id_ = _idGetter.getID(); 
	float disconnectTimer_;
	ClientStatus status_;
	std::shared_ptr<Connection> connection_;
	std::string _roomToJoin;

	rplanes::playerdata::Profile profile_;
	std::shared_ptr< Player > player_;
};
