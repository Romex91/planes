#include "client.h"
using namespace rplanes;
extern std::shared_ptr<odb::database> planesDB;
extern std::shared_ptr<odb::database> profilesDB;

Client::ProfilesInfo Client::profilesInfo_;

bool clientIsInRoom( size_t clientID ) /*определить статус по id */
{
	return clientID >= configuration().server.maxClientsNumber;
}

size_t convertIDToPos( size_t clienID ) /*получить положение в векторе из id */
{
	if ( clientIsInRoom(clienID) )
	{
		return clienID - configuration().server.maxClientsNumber;
	}
	return clienID;
}

size_t convertPosToID( size_t posInVector, bool inRoom )
{
	if ( inRoom )
	{
		return posInVector + configuration().server.maxClientsNumber;
	}
	return posInVector;
}

void Client::exitRoom()
{
	if ( status_ != ROOM )
	{
		throw PlanesException( _str("Client is out of any room."));
	}
	status_ = HANGAR;
	

	//сохраняем прогресс
	profile_.statistics[player_->getPlaneName()]+= player_->statistics;
	profile_.statistics["total"]+=player_->statistics;
	profile_.money+= player_->statistics.money;
	profile_.pilot.addExp(player_->statistics.exp);
	
	profile_.openedMaps.insert(player_->openedMaps.begin(), player_->openedMaps.end());
	profile_.openedPlanes.insert(player_->openedPlanes.begin(), player_->openedPlanes.end());
	//выводим сообщение
	std::cout <<  _str("{0} leaved room.", profile_.login).str() << std::endl;

	//сообщаем комнате, что клиент вышел
	player_->isJoined = false;
	player_.reset();
	//пытаемся сообщить клиенту
	try
	{
		sendMessage(network::bidirectionalmessages::ExitRoom());
	}
	catch (...)
	{}
}

void Client::joinRoom( Room & room, size_t planeNo)
{
	if ( status_ != HANGAR )
	{
		throw PlanesException( _str( "Cannot join room. Client is out of hangar." ));
	}
	if ( planeNo >= profile_.planes.size()  )
	{
		throw PlanesException(_str("Cannot join room. Wrong parameters."));
	}

	if (!profile_.planes[planeNo].isReadyForJoinRoom())
	{
		throw PlanesException(_str("Cannot join room. Ensure that wings engines missiles and guns are mounted symmetrically"));
	}

	serverdata::Plane plane;
	plane = profile_.planes[planeNo].buildPlane(profile_.pilot, planesDB);

	player_ = std::shared_ptr<Player>( new Player( plane, profile_.login ) );
	player_->openedMaps = profile_.openedMaps;
	player_->openedPlanes = profile_.openedPlanes;

	room.addPlayer(player_);

	status_ = ROOM;
}

playerdata::Profile & Client::profile()
{
	if ( status_!=HANGAR )
	{
		throw PlanesException(_str("Cannot get profile. Client is out of hangar."));
	}
	return profile_;
}

network::ClientStatus Client::getStatus()
{
	return status_;
}

void Client::logout()
{
	if ( status_ == UNLOGGED )
	{
		throw PlanesException (_str("Client is not logged in."));
	}
	status_ = UNLOGGED;
	{
		MutexLocker locker( profilesInfo_.Mutex );
		if( profilesInfo_.loggedInProfiles.erase(profile_.login) != 1 )
		{
			throw PlanesException(_str("Logged in profiles set does not contain {0}", profile_.login));
		}
	}
	profile_.save(profilesDB);
	std::cout << _str("{0} logged out.", profile_.login).str() << std::endl;
}

void Client::login( std::string name, std::string password )
{
	if ( status_ != UNLOGGED )
	{
		throw PlanesException(_str("Client has already logged in."));
	}
	//заблокируем множество авторизованных пользователей
	MutexLocker locker( profilesInfo_.Mutex );
	//проверим занятость профиля
	if ( profilesInfo_.loggedInProfiles.count(name) > 0 )
	{
		throw PlanesException(_str("{0} is locked by other player."));
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
		throw PlanesException(_str("Wrong name or password."));
	}
	//проверим пароль
	if ( profile_.password != password )
	{
		throw PlanesException(_str("Wrong name or password."));
	}
	//если все предыдущие операции удались, значит авторизация легальна
	//загружаем самолеты профиля
	profile_.loadPlanes(profilesDB);
	//регистрируем имя
	profilesInfo_.loggedInProfiles.insert(name);
	//изменяем статус
	status_ = HANGAR;
	//выводим сообщение
	std::cout << profile_.login << " зашел в игру с адреса " 
		<< connection_.getIP()
		<< std::endl;
}

void Client::setControllable( serverdata::Plane::ControllableParameters controllable )
{
	if ( status_ != ROOM  || !player_)
	{
		throw PlanesException(_str("Cannot set controllable parameters. Client is out of room."));
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
	disconnectTimer_( configuration().server.unloginedDisconnectTime ),
	status_(UNLOGGED)
{
	setID(clientID);
	MutexLocker( profilesInfo_.Mutex );
	if ( profilesInfo_.clientsCount >= configuration().server.maxClientsNumber )
	{
		throw PlanesException(_str("Server is overloaded."));
	}
	profilesInfo_.clientsCount++;
}

void Client::sendRoomMessages()
{
	if (status_ != ROOM)
	{
		throw PlanesException(_str("Failed sanding room messages. Client is out of room."));
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
		throw PlanesException(_str("Cannot exit room. Client is out of room."));
	}
	player_->isJoined = false;
}
