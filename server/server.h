#pragma once
#include "stdafx.h"
#include "room.h"
#include "client.h"

extern std::shared_ptr<odb::database> planesDB;
extern std::shared_ptr<odb::database> profilesDB;


class Server
{
public:
	struct RoomListMessage
	{
		Mutex mutex;
		MRoomList message;
	}roomListMessage;

	void updateRoomMessage();

	float getTime()const;

	Server();

	Client & getClient(size_t clientID);

	void joinRoom(size_t clientID, std::string creatorName, size_t planeNumber);

	void createRoom(size_t clientID, std::string description, std::string mapName);

	void destroyRoom( size_t clientID );

	//handling hangar clients and new connections
	//single-threaded
	void hangarLoop();

	//handling in-room clients
	//multi-threaded
	void roomLoop();

	void administerRoom( size_t clientID , rplanes::network::MAdministerRoom::Operation operation,
		std::vector<std::string> options);


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
		std::vector<std::shared_ptr<Client>> clients;
		Mutex mutex;
	public:
		void join( std::shared_ptr<Client> & client );
		
		//returns null if the queue is empty
		std::shared_ptr<Client> pop();
	};

	boost::asio::io_service io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	
	//in-room clients
	ClientsList roomClients_;
	
	//in-hangar clients
	ClientsList hangarClients_;
	
	//queues for clients to move from the hangar to rooms and back
	ClientsQueue hangarQueue_, deleteQueue_;


	std::map< std::string, Room > rooms_;

	float time_;

	std::shared_ptr<Client> & emptyClient( ClientsList & cl, size_t & pos );

	std::shared_ptr<Client> & getClientPtr( size_t clientID );

	//hangarLoopMethods

	void deleteClient( std::shared_ptr<Client> & client );

	void listen();

	void handleHangarInput();

	void deleteUnlogined( float frameTime );

	//roomLoopMethods
	void handleRoomInput();

};
