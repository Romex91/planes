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

	void createRoom(std::shared_ptr<Client> client, const MCreateRoomRequest & message);

	void destroyRoom(std::shared_ptr<Client> client);

	//handling hangar clients and new connections
	//single-threaded
	void hangarLoop();

	//handling in-room clients
	//multi-threaded
	void roomLoop();


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


	std::map< std::string, std::shared_ptr<Room> > rooms_;

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


	void administerRoom(const MAdministerRoom & message, Client & client);

	void setMessageHandlers( std::weak_ptr<Client> clientPtr )
	{
		auto client = clientPtr.lock();
		client->connection_.setHandler<MServerTimeRequest>([this, clientPtr](const MServerTimeRequest &)
		{
			MServerTime mess;
			mess.time = getTime();
			clientPtr.lock()->sendMessage(mess);
		});

		client->connection_.setHandler<MAdministerRoom>([this, clientPtr](const MAdministerRoom & message) 
		{
			auto client = clientPtr.lock();
			administerRoom(message, *client);
		});

		client->connection_.setHandler<MPlayerProfileRequest>([this, clientPtr](const MPlayerProfileRequest & message)
		{
			auto client = clientPtr.lock();
			if (client->getStatus() != ClientStatus::HANGAR)
				throw RPLANES_EXCEPTION("you can't observe other players statistics from a place other than the hangar");
			try
			{
				odb::transaction t(profilesDB->begin());
				client->sendMessage(MProfile(*profilesDB->load<rplanes::playerdata::Profile>(message.playerName)));
				t.commit();
			}catch (PlanesException & e) {
				throw e;
			} catch (...) {
				throw RPLANES_EXCEPTION("Profile {0} is not found in database.", message.playerName);
			}
		});

		client->connection_.setHandler<MJoinRoomRequest>([this, clientPtr](const MJoinRoomRequest & message)
		{
			joinRoom(clientPtr.lock()->getId(), message.playerName, message.planeNo);
		});

		client->connection_.setHandler<MCreateRoomRequest>([this, clientPtr](const MCreateRoomRequest & message)
		{
			createRoom(clientPtr.lock(), message);
		});

		client->connection_.setHandler<MDestroyRoomRequest>([this, clientPtr](const MDestroyRoomRequest & message)
		{
			destroyRoom(clientPtr.lock());
		});

		client->connection_.setHandler<MRoomListRequest>([this, clientPtr](const MRoomListRequest & message)
		{
			clientPtr.lock()->profile();//just to check the client is in hangar
			MutexLocker ml(roomListMessage.mutex);
			clientPtr.lock()->sendMessage(roomListMessage.message);
		});

		client->connection_.setHandler<MRegistry>([](const MRegistry & message)
		{
			playerdata::Profile profile;
			profile.login = message.name;
			profile.password = message.password;
			odb::transaction t(profilesDB->begin());
			profilesDB->persist(profile);
			t.commit();
		});
	}
};
