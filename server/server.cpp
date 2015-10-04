#include "server.h"

using namespace rplanes;
using namespace network;

using boost::asio::ip::tcp;
std::shared_ptr<odb::database> planesDB;
std::shared_ptr<odb::database> profilesDB;



void Server::ClientsQueue::join(std::shared_ptr<Client> & client)
{
	MutexLocker ml(mutex);
	if (client)
	{
		clients.push_back(client);
		client.reset();
	}
	else
	{
		throw PlanesException(_rstrw("Null ptr in clients queue."));
	}
}

std::shared_ptr<Client> Server::ClientsQueue::pop()
{
	MutexLocker ml(mutex);
	if (clients.size() == 0)
	{
		return std::shared_ptr<Client>();
	}
	auto retval = clients.back();
	clients.pop_back();
	return retval;
}

std::shared_ptr<Client> & Server::emptyClient(ClientsList & cl, size_t & pos)
{
	//ищем свободную позицию в ангарном векторе 
	size_t newClientPos = 0;
	for (; newClientPos != cl.clients.size(); newClientPos++)
	{
		if (!cl.clients[newClientPos])
		{
			break;
		}
	}
	//если свободный клиент не найден, добавим новый
	if (newClientPos == cl.clients.size())
	{
		//создаем новый клиент
		cl.clients.push_back(std::shared_ptr<Client>());
	}
	pos = newClientPos;
	return cl.clients[newClientPos];
}

std::shared_ptr<Client> & Server::getClientPtr(size_t clientID)
{
	size_t pos = convertIDToPos(clientID);
	if (clientIsInRoom(clientID))
	{
		if (pos < roomClients_.clients.size())
		{
			return roomClients_.clients[pos];
		}
	}
	else
	{
		if (pos < hangarClients_.clients.size())
		{
			return hangarClients_.clients[pos];
		}
	}
	throw PlanesException(_rstrw("client is not connected"));
}

void Server::deleteClient(std::shared_ptr<Client> & client)
{
	//пытаемся корректно завершить работу клиента
	try
	{
		switch (client->getStatus())
		{
		case rplanes::network::UNLOGGED:
			break;
		case rplanes::network::HANGAR:
			client->logout();
			break;
		case rplanes::network::ROOM:
			client->exitRoom();
			client->logout();
			break;
		}
	}
	catch (std::exception & e)
	{
		std::wcout << _rstrw("Unexpected error deleting client. {0}", e.what()).str() << std::endl;
	}
	//выводим сообщение
	std::wcout << _rstrw("Lost connection. {0}", client->connection_.getIP()).str() << std::endl;
	//удаляем клиент
	client.reset();
}

void Server::listen()
{
	while (true)
	{
		//пробуем подключитсья
		static std::shared_ptr<Client> newClient;
		{
			try
			{
				if (!newClient)
				{
					newClient = std::shared_ptr<Client>(new Client(io_service_));
				}

				boost::system::error_code err = newClient->connection_.accept(acceptor_);
				if (err)
				{
					//если новых подключений не найдено, завершаем listen
					return;
				}
				newClient->connection_.non_blocking(true);
			} catch (std::exception & e) {
				std::cout << e.what() << std::endl;
				return;
			}
		}
		//заносим новый клиент в ангарный вектор
		size_t id;
		emptyClient(hangarClients_, id) = newClient;
		newClient->setID(id);
		//выводим сообщение
		std::cout << "получено новое подключение " << newClient->connection_.getIP() << std::endl;
		newClient.reset();
	}
}

void Server::handleHangarInput()
{
	for (size_t i = 0; i < hangarClients_.clients.size(); i++)
	{
		size_t handledMessages = 0;
		while (hangarClients_.clients[i]) //цикл выполняется до тех пор, пока клиент не будет  удален из ангарного списка, либо не кончатся сообщения
		{
			//пытаемся получить сообщение
			try
			{
				if (!hangarClients_.clients[i]->connection_.handleInput())
				{
					break;
				}
				//если сообщение получено, проверяем количество обработанных сообщений
				handledMessages++;
				if (handledMessages > configuration().server.hangarMessagesPerFrame)
				{
					std::cout << handledMessages << std::endl;
					throw PlanesException(_rstrw("Client has sent {0} messages per one frame. {1} messages per frame permitted.", handledMessages, configuration().server.hangarMessagesPerFrame));
				}
			}
			catch (std::exception & e)
			{
				//возникла ошибка, удаляем клиент
				std::wcout << _rstrw("Failed handling message {0}. {1}", 
					hangarClients_.clients[i]->connection_.getLastMessageId(), e.what()).str() << std::endl;
				deleteClient(hangarClients_.clients[i]);
			}
		}
	}
}

void Server::deleteUnlogined(float frameTime)
{
	for (std::shared_ptr<Client> & client : hangarClients_.clients)
	{
		if (client)
		{
			if (client->getStatus() == UNLOGGED)
			{
				client->disconnectTimer_ -= frameTime;
				if (client->disconnectTimer_ < 0)
				{
					deleteClient(client);
				}
			}
		}
	}
}

void Server::handleRoomInput()
{
	for (int i = 0; i < roomClients_.clients.size(); i++)
	{
		size_t handledMessages = 0;
		//цикл выполняется до тех пор, пока клиент не будет  удален из комнатного списка, либо не кончатся сообщения
		while (roomClients_.clients[i])
		{
			//пытаемся получить сообщение
			try
			{
				if (!roomClients_.clients[i]->connection_.handleInput())
				{
					break;
				}
				//если сообщение получено, проверяем количество обработанных сообщений
				handledMessages++;
				if (handledMessages > configuration().server.roomMessagesPerFrame)
				{
					std::cout << handledMessages << std::endl;
					throw PlanesException(_rstrw("Client has sent {0} messages per one frame. {1} messages per frame permitted.", handledMessages, configuration().server.roomMessagesPerFrame));
				}
			}
			//возникла ошибка, обрываем связь и помещаяем клиент в очередь удаления
			catch (std::exception & e)
			{
				std::wcout << _rstrw("Failed handling message {0}. {1}", 
					roomClients_.clients[i]->connection_.getLastMessageId(), e.what()).str() << std::endl;
				roomClients_.clients[i]->connection_.close();
				deleteQueue_.join(roomClients_.clients[i]);
			}
		}
	}
}

void Server::updateRoomMessage()
{
	MutexLocker ml(roomListMessage.mutex);
	roomListMessage.message.rooms.clear();
	for (auto & room : rooms_)
	{
		roomListMessage.message.rooms.push_back(servermessages::hangar::RoomList::RoomInfo());
		roomListMessage.message.rooms.back().description = room.second.description;
		roomListMessage.message.rooms.back().mapName = "not specified";
		roomListMessage.message.rooms.back().creatorName = room.first;
		roomListMessage.message.rooms.back().slots = room.second.getPlayerNumber();
	}
}

float Server::getTime() const
{
	return time_;
}

Server::Server() :acceptor_(io_service_, tcp::endpoint(tcp::v4(), configuration().server.port))
{
	std::cout << "Server listens port " << configuration().server.port << std::endl;
	acceptor_.non_blocking(true);
	time_ = 0.0;
}

Client & Server::getClient(size_t clientID)
{
	auto client = getClientPtr(clientID);
	return *client;
}

void Server::joinRoom(size_t clientID, std::string creatorName, size_t planeNumber)
{
	auto & client = getClientPtr(clientID);
	if (client->getStatus() != HANGAR)
	{
		throw PlanesException(_rstrw("Cannot join room. Player is out of hangar."));
	}
	//если клиент является создателем комнаты, он присоединится именно к своей комнате
	auto room = rooms_.find(client->profile_.login);
	if (room == rooms_.end())
	{
		room = rooms_.find(creatorName);
	}

	if (room == rooms_.end())
	{
		throw PlanesException(_rstrw("Room is not found."));
	}

	if (std::find(room->second.banlist.begin(), room->second.banlist.end(), client->profile_.login)
		!= room->second.banlist.end())
	{
		throw PlanesException(_rstrw("Player is in the banlist."));
	}


	MutexLocker ml(roomClients_.mutex);
	{
		client->joinRoom(room->second, planeNumber);
		size_t pos;
		emptyClient(roomClients_, pos) = client;
		client->setID(convertPosToID(pos, true));
	}

	std::wcout << _rstrw("{0} connected to room {1}", client->profile().login, room->first).str() << std::endl;
	client.reset();
}

void Server::createRoom(size_t clientID, std::string description, std::string mapName)
{
	auto & client = getClient(clientID);
	if (client.getStatus() != HANGAR)
	{
		throw PlanesException(_rstrw("Cannot create room. Player is out of hangar."));
	}
	if (rooms_.count(client.profile().login) != 0)
	{
		throw PlanesException(_rstrw("Player has already created a room."));
	}

	Room room(mapName);
	room.creator = client.profile().login;
	room.description = description;
	room.banlist = client.profile_.banlist;

	MutexLocker ml(roomClients_.mutex);//блокируем мьютекс комнатной петли
	{
		rooms_[client.profile().login] = room;
	}

	std::wcout << _rstrw("{0} created a room.", client.profile().login).str() << std::endl;
}


void Server::destroyRoom(size_t clientID)
{
	auto & client = getClient(clientID);
	if (client.getStatus() != HANGAR)
	{
		throw PlanesException(_rstrw("Cannot destroy room. Player is out of hangar."));
	}
	if (rooms_.erase(client.profile_.login) == 0)
	{
		throw PlanesException(_rstrw("Player has no room to destroy."));
	}
}


void Server::hangarLoop()
{
	std::chrono::steady_clock::time_point iterationBegin;
	std::chrono::microseconds iterationTime;
	std::chrono::microseconds configFrameTime(
		static_cast<long long>(
		configuration().server.hangarFrameTime * 1000000));
	std::chrono::duration<float> frameTime;
	while (true)
	{
		iterationBegin = std::chrono::steady_clock::now();
		{
			MutexLocker ml(hangarClients_.mutex);
			//подключаем новых клиентов
			listen();
			//выполняем команды клиентов
			handleHangarInput();
			//удаляем неавторизованных клиентов
			deleteUnlogined(frameTime.count());

			//обработать очередь выхода из комнаты
			while (auto client = hangarQueue_.pop())
			{
				client->exitRoom();
				size_t pos;
				emptyClient(hangarClients_, pos) = client;
				client->setID(convertPosToID(pos, false));
				std::cout << "Клиент " << client->profile().login << " выброшен из комнаты" << std::endl;
			}
			//обработать очередь удаления
			while (auto client = deleteQueue_.pop())
			{
				deleteClient(client);
			}

		}
		iterationTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - iterationBegin);
		if (iterationTime < configFrameTime)
		{
			std::this_thread::sleep_for(configFrameTime - iterationTime);
		}
		frameTime = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - iterationBegin);
	}
}

void Server::roomLoop()
{
	//время начала итерации
	std::chrono::steady_clock::time_point iterationBegin;

	//время кадра, указанное в конфигурации
	std::chrono::microseconds configFrameTime(
		static_cast<long long>(
		configuration().server.roomFrameTime * 1000000));

	//полное время кадра. складывается из времени итерации и времени сна
	std::chrono::duration<float> frameTime;

	//время выполнения итерации
	std::chrono::microseconds iterationTime;
	std::shared_ptr< MutexLocker > locker;
	{
		iterationBegin = std::chrono::steady_clock::now();
		while (true)
		{

			locker = std::shared_ptr<MutexLocker>(new MutexLocker(roomClients_.mutex));

			//выполняем команды клиентов
			handleRoomInput();

			//обрабатываем каждую комнату
			for (auto & room : rooms_)
			{
				room.second.iterate(frameTime.count(), getTime());
			}

			//отправляем сообщения игрокам
			for (int i = 0; i < roomClients_.clients.size(); i++)
			{
				if (!roomClients_.clients[i])continue;
				auto & client = *roomClients_.clients[i];
				try
				{
					client.sendRoomMessages();
				}
				catch (std::exception & e)
				{
					std::wcout << _rstrw("Failed sending message: {0} ", e.what()).str() << std::endl;
					client.connection_.close();
					deleteQueue_.join(roomClients_.clients[i]);
				}
			}

			//выбрасываем в ангар отключенных клиентов
			for (auto & client : roomClients_.clients)
			{
				if (!client)
				{
					continue;
				}
				if (!client->player_->isJoined)
				{
					hangarQueue_.join(client);
				}
			}

			updateRoomMessage();

			for (auto & room : rooms_)
			{
				room.second.clearTemroraryData();
			}


			locker.reset();

			iterationTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - iterationBegin);
			if (iterationTime < configFrameTime)
			{
				std::this_thread::sleep_for(configFrameTime - iterationTime);
			}
			frameTime = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - iterationBegin);
			iterationBegin = std::chrono::steady_clock::now();
			time_ += frameTime.count();
		}
	}
}

void Server::administerRoom(size_t clientID, rplanes::network::clientmessages::room::AdministerRoom::Operation operation, std::vector<std::string> options)
{

	auto & client = getClient(clientID);
	if (client.getStatus() != ROOM)
	{
		throw PlanesException(_rstrw("Join room first."));
	}

	if (rooms_.count(client.profile_.login) == 0)
	{
		throw PlanesException(_rstrw("Player has no room to administer."));
	}
	auto & room = rooms_[client.profile_.login];

	switch (operation)
	{
	case rplanes::network::clientmessages::room::AdministerRoom::KICK_PLAYERS:
		room.kickPlayers(options);
		break;
	case rplanes::network::clientmessages::room::AdministerRoom::BAN_PLAYERS:
		if ( client.profile_.banlist.size() > rplanes::configuration().profile.maxBanlistSize )
		{
			throw PlanesException(_rstrw("Banlist overflowed."));
		}
		client.profile_.banlist.insert(options.begin(), options.end());
		room.banlist.insert(options.begin(), options.end());
		room.kickPlayers(options);
		break;
	case rplanes::network::clientmessages::room::AdministerRoom::UNBAN_PLAYERS:
		for (auto & name : options)
		{
			room.banlist.erase(name);
		}
		break;
	case rplanes::network::clientmessages::room::AdministerRoom::CHANGE_MAP:
		if (options.size() != 1)
		{
			throw PlanesException(_rstrw("Cannot change map. Wrong argument."));
		}
		room.changeMap(options[0]);
		break;
	case rplanes::network::clientmessages::room::AdministerRoom::RESTART:
		room.restart();
		break;
	case rplanes::network::clientmessages::room::AdministerRoom::KILL_PLAYERS:
		for (auto & name : options)
		{
			room.setPlayerDestroyed(name);
		}
		break;
	default:
		break;
	}
}
