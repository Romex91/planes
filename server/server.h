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
		servermessages::hangar::RoomList message;
	}roomListMessage;

	void updateRoomMessage();

	float getTime()const;

	Server();

	Client & getClient(size_t clientID);

	void joinRoom(size_t clientID, std::string creatorName, size_t planeNumber);

	void createRoom(size_t clientID, std::string description, std::string mapName);

	void destroyRoom( size_t clientID );

	//для клиентов, находящихся в ангаре и новых подключений
	//выполняется последовательно
	void hangarLoop();

	//для клиентов, находящихся в комнате
	//выполняется параллельно по комнатам
	void roomLoop();

	void administerRoom( size_t clientID , rplanes::network::clientmessages::room::AdministerRoom::Operation operation,
		std::vector<std::string> options);


private:
	friend class consoleHandler;

	//вектор клиентов с мьютексом. Мьютекс введен для возможности остановить итерацию
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
		
		//если очередь пуста, вернет пустой указатель
		std::shared_ptr<Client> pop();
	};

	boost::asio::io_service io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	
	//список клиентов, присоединенных к комнатам. Блокируется на время каждой итерации
	ClientsList roomClients_;
	
	//список клиентов, находящихся в ангаре. Блокируется на время каждой итерации
	ClientsList hangarClients_;
	
	//"мосты", по которым клиенты переходят из ангара в комнаты и обратно, блокируются на время обращения
	ClientsQueue hangarQueue_, deleteQueue_;


	std::map< std::string, Room > rooms_;

	float time_;

	std::shared_ptr<Client> & emptyClient( ClientsList & cl, size_t & pos );

	std::shared_ptr<Client> & getClientPtr( size_t clientID );

	//hangarLoopMethods


	//выбросить из комнаты, лишить гражданства и расстрелять
	void deleteClient( std::shared_ptr<Client> & client );

	//обработка подключений nonblocking выполняется в последовательной петле ангара
	void listen();

	void handleHangarInput();

	void deleteUnlogined( float frameTime );

	//roomLoopMethods
	void handleRoomInput();

};
