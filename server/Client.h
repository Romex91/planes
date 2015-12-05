#pragma once
#include "stdafx.h"
#include "room.h"

using namespace rplanes::network;


//TODO: remove this insanity
bool clientIsInRoom(size_t clientID );
size_t convertIDToPos( size_t clienID );
size_t convertPosToID( size_t posInVector, bool inRoom  );

//limitations:
//clients cannot access other clients. Client condition cannot affect other clients
//this class is singlethreaded
class Client : private boost::noncopyable
{
public:
	friend class Server;

	Client(std::shared_ptr<Connection> connection, size_t clientID = 0 );
	~Client();
	void setID( size_t id );
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

	void prepareRoomExit();

	ClientStatus getStatus();

	rplanes::playerdata::Profile & profile();

private:


	void setMessageHandlers();

	void sendRoomMessages();

	//call this method from the hungar loop before moving the client to the room queue
	void joinRoom( Room& room, size_t planeNo );

	//call this method when leaving the room
	void exitRoom();

	struct ProfilesInfo
	{
		std::set<std::string> loggedInProfiles;
		Mutex Mutex;
		size_t clientsCount = 0;
	};

	static ProfilesInfo profilesInfo_;
	size_t id_; 
	float disconnectTimer_;
	ClientStatus status_;
	std::shared_ptr<Connection> connection_;

	rplanes::playerdata::Profile profile_;
	std::shared_ptr< Player > player_;
};
