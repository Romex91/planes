#pragma once
#include "stdafx.h"
#include "room.h"
#include "client.h"

extern std::shared_ptr<odb::database> planesDB;
extern std::shared_ptr<odb::database> profilesDB;

using namespace rplanes;
using namespace rplanes::network;
class Server
{
public:
	class RoomListMessage
	{
	public:
		MRoomList get()
		{
			MutexLocker l(_mutex);
			return _message;
		}

		void set(const MRoomList & message)
		{
			MutexLocker l(_mutex);
			_message = message;
		}

	private:
		Mutex _mutex;
		MRoomList _message;
	}roomListMessage;

	void updateRoomMessage();

	float getTime()const;

	Server();

	Client & getClient(size_t clientID);


	void joinRoom(std::shared_ptr<Client> client);

	void createRoom(std::shared_ptr<Client> client, const MCreateRoomRequest & message);

	void destroyRoom(std::shared_ptr<Client> client);

	//handling hangar clients and new connections
	//single-threaded
	void hangarLoop();

	//handling in-room clients
	//multi-threaded
	void roomLoop();

	//asynchronous network handling
	void networkLoop();

private:
	friend class consoleHandler;


	class ClientsList
	{
	public:
		std::vector<std::shared_ptr<Client>> clients;
		Mutex mutex;
	};

	class ClientsQueue
	{
		std::queue<std::shared_ptr<Client>> clients;
		Mutex mutex;
	public:
		void join( const std::shared_ptr<Client> & client );
		
		//returns null if the queue is empty
		std::shared_ptr<Client> pop();
	};

	//in-room clients
	ClientsList roomClients_;
	
	//in-hangar clients
	ClientsList hangarClients_;
	
	//queues for clients to move from the hangar to rooms and back
	 ClientsQueue hangarQueue_, deleteQueue_, _newClientsQueue, _roomQueue;


	std::map< std::string, std::shared_ptr<Room> > rooms_;

	float time_;

	std::shared_ptr<Client> & emptyClient( ClientsList & cl, size_t & pos );

	std::shared_ptr<Client> & getClientPtr( size_t clientID );

	//hangarLoopMethods

	void handleHangarInput();

	void deleteUnlogined( float frameTime );

	//roomLoopMethods
	void handleRoomInput();


	void administerRoom(const MAdministerRoom & message, Client & client);

	void setMessageHandlers(std::weak_ptr<Client> clientPtr);
};
