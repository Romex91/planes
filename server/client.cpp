#include "client.h"

extern std::shared_ptr<odb::database> planesDB;
extern std::shared_ptr<odb::database> profilesDB;

Client::ProfilesInfo Client::profilesInfo_;

bool clientIsInRoom( size_t clientID ) /*определить статус по id */
{
	return clientID >= rplanes::configuration().server.maxClientsNumber;
}

size_t convertIDToPos( size_t clienID ) /*получить положение в векторе из id */
{
	if ( clientIsInRoom(clienID) )
	{
		return clienID - rplanes::configuration().server.maxClientsNumber;
	}
	return clienID;
}

size_t convertPosToID( size_t posInVector, bool inRoom )
{
	if ( inRoom )
	{
		return posInVector + rplanes::configuration().server.maxClientsNumber;
	}
	return posInVector;
}

void Client::exitRoom()
{
	if ( status_ != ROOM )
	{
		throw rplanes::eClientStatusError( "Клиент не состоит в комнате. " );
	}
	status_ = HANGAR;
	
	if ( !player_ )
	{
		throw rplanes::eClientStatusError( "Указатель игрока пуст. ");
	}

	//сохраняем прогресс
	profile_.statistics[player_->getPlaneName()]+= player_->statistics;
	profile_.statistics["total"]+=player_->statistics;
	profile_.money+= player_->statistics.money;
	profile_.pilot.addExp(player_->statistics.exp);
	
	profile_.openedMaps.insert(player_->openedMaps.begin(), player_->openedMaps.end());
	profile_.openedPlanes.insert(player_->openedPlanes.begin(), player_->openedPlanes.end());
	//выводим сообщение
	std::cout << profile_.login 
		<< " вышел в ангар " << std::endl;

	//сообщаем комнате, что клиент вышел
	player_->isJoined = false;
	player_.reset();
	//пытаемся сообщить клиенту
	try
	{
		sendMessage(rplanes::network::bidirectionalmessages::ExitRoom());
	}
	catch (...)
	{}
}

void Client::joinRoom( Room & room, size_t planeNo)
{
	if ( status_ != HANGAR )
	{
		throw rplanes::eClientStatusError( "Для входа в комнату необходима авторизация. " );
	}
	if ( planeNo >= profile_.planes.size()  )
	{
		throw rplanes::eProfileError("Переданы неверные параметры. ");
	}

	if (!profile_.planes[planeNo].isReadyForJoinRoom())
	{
		throw rplanes::eRoomError(" Конфигурация самолета не отвечает требованиям. Крылья, двигатели, ракеты и пушки должны быть установлены симметрично. ");
	}

	rplanes::serverdata::Plane plane;
	try
	{
		plane = profile_.planes[planeNo].buildPlane(profile_.pilot, planesDB);
	}
	catch (std::exception & e)
	{
		throw rplanes::eRoomError(e.what());
	}

	player_ = std::shared_ptr<Player>( new Player( plane, profile_.login ) );
	player_->openedMaps = profile_.openedMaps;
	player_->openedPlanes = profile_.openedPlanes;

	room.addPlayer(player_);

	status_ = ROOM;
}

rplanes::playerdata::Profile & Client::profile()
{
	if ( status_!=HANGAR )
	{
		throw rplanes::eClientStatusError("Попытка получить профиль вне ангара.");
	}
	return profile_;
}

rplanes::network::ClientStatus Client::getStatus()
{
	return status_;
}

void Client::logout()
{
	if ( status_ == UNLOGINED )
	{
		throw rplanes::eClientStatusError("Запрос выхода от неавторизованного клиента.");
	}
	status_ = UNLOGINED;
	{
		MutexLocker locker( profilesInfo_.Mutex );
		if( profilesInfo_.loginedProfiles.erase(profile_.login) != 1 )
		{
			throw rplanes::eProfileError("Ошибка множества авторизованных пользователей.");
		}
	}
	try 
	{
		profile_.save(profilesDB);
	}
	catch(odb::exception & e)
	{
		throw rplanes::eProfileError( std::string() + "Ошибка базы данных. " + e.what() );
	}
	std::cout << profile_.login<<" вышел из игры" << std::endl;
}

void Client::login( std::string name, std::string password )
{
	if ( status_ != UNLOGINED )
	{
		throw rplanes::eLoginFail("Попытка повторной авторизации. ");
	}
	//заблокируем множество авторизованных пользователей
	MutexLocker locker( profilesInfo_.Mutex );
	//проверим занятость профиля
	if ( profilesInfo_.loginedProfiles.count(name) > 0 )
	{
		throw rplanes::eLoginFail("Профиль занят. ");
	}
	//попробуем загрузить профиль с таким именем
	try
	{
		odb::transaction t(profilesDB->begin());
		profile_ = *profilesDB->load<rplanes::playerdata::Profile>(name);
		t.commit();
	}
	catch(...)
	{
		throw rplanes::eLoginFail("Имя пользователя или пароль указаны неверно. ");
	}
	//проверим пароль
	if ( profile_.password != password )
	{
		throw rplanes::eLoginFail("Имя пользователя или пароль указаны неверно. ");
	}
	//если все предыдущие операции удались, значит авторизация легальна
	//загружаем самолеты профиля
	try
	{
		profile_.loadPlanes(profilesDB);
	}
	catch(...)
	{
		throw rplanes::eLoginFail("Ошибка базы данных. ");
	}
	//регистрируем имя
	profilesInfo_.loginedProfiles.insert(name);
	//изменяем статус
	status_ = HANGAR;
	//выводим сообщение
	std::cout << profile_.login << " зашел в игру с адреса " 
		<< connection_.getIP()
		<< std::endl;
}

void Client::setControllable( rplanes::serverdata::Plane::ControllableParameters controllable )
{
	if ( status_ != ROOM  || !player_)
	{
		throw rplanes::eClientStatusError("Отправка данных управления не из комнаты. ");
	}
	player_->setControllable(controllable);
}

void Client::sendMessage( Message & mess )
{
	connection_.sendMessage(mess);
}

void Client::setID( size_t id )
{
	id_ = id;
	connection_.setClientID( id_);
}

Client::~Client()
{
	MutexLocker( profilesInfo_.Mutex );
	profilesInfo_.clientsCount--;
}

Client::Client( boost::asio::io_service& io_service , size_t clientID /*= 0 */ ) :
	connection_(io_service),
	disconnectTimer_( rplanes::configuration().server.unloginedDisconnectTime ),
	status_(UNLOGINED)
{
	setID(clientID);
	MutexLocker( profilesInfo_.Mutex );
	if ( profilesInfo_.clientsCount >= rplanes::configuration().server.maxClientsNumber )
	{
		throw rplanes::eClientConnectionFail("Достигнуто максимальное количество клиентов. ");
	}
	profilesInfo_.clientsCount++;
}

void Client::sendRoomMessages()
{
	if (status_ != ROOM)
	{
		throw rplanes::eClientStatusError("Попытка отправить комнантные данные не из комнаты. ");
	}
	if (player_->messages.createPlanes.Planes.size() > 0)
		sendMessage(player_->messages.createPlanes);
	if (player_->messages.createBullets.bullets.size() > 0)
		sendMessage(player_->messages.createBullets);
	if (player_->messages.createMissiles.missiles.size() > 0)
		sendMessage(player_->messages.createMissiles);
	if (player_->messages.destroyBullets.bullets.size() > 0)
		sendMessage(player_->messages.destroyBullets);
	if (player_->messages.createRicochetes.bullets.size() > 0)
		sendMessage(player_->messages.createRicochetes);
	if (player_->messages.destroyMissiles.ids.size() > 0)
		sendMessage(player_->messages.destroyMissiles);
	if (player_->messages.destroyPlanes.planes.size() > 0)
		sendMessage(player_->messages.destroyPlanes);
	if (player_->messages.updateModules.modules.size() > 0)
	{
		sendMessage(player_->messages.updateModules);
	}
	if (player_->messages.setPlanesPositions.positions.size() > 0)
	{
		sendMessage(player_->messages.interfaceData);
		sendMessage(player_->messages.setPlanesPositions);
	}
	for (auto & message : player_->messages.textMessages)
	{
		sendMessage(message);
	}
	for (auto & message : player_->messages.changeMapMessages)
	{
		sendMessage(message);
	}
	for (auto & message : player_->messages.roomInfos)
	{
		sendMessage(message);
	}
}

void Client::prepareRoomExit()
{
	if (status_ != ROOM)
	{
		throw rplanes::eClientStatusError("Клиент пытается выйти не находясь в комнате. ");
	}
	player_->isJoined = false;
}
