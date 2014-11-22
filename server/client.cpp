#include "client.h"

extern std::shared_ptr<odb::database> planesDB;
extern std::shared_ptr<odb::database> profilesDB;

Client::ProfilesInfo Client::profilesInfo_;

bool clientIsInRoom( size_t clientID ) /*���������� ������ �� id */
{
	return clientID >= rplanes::configuration().server.maxClientsNumber;
}

size_t convertIDToPos( size_t clienID ) /*�������� ��������� � ������� �� id */
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
		throw rplanes::eClientStatusError( "������ �� ������� � �������. " );
	}
	status_ = HANGAR;
	
	if ( !player_ )
	{
		throw rplanes::eClientStatusError( "��������� ������ ����. ");
	}

	//��������� ��������
	profile_.statistics[player_->getPlaneName()]+= player_->statistics;
	profile_.statistics["total"]+=player_->statistics;
	profile_.money+= player_->statistics.money;
	profile_.pilot.addExp(player_->statistics.exp);
	
	profile_.openedMaps.insert(player_->openedMaps.begin(), player_->openedMaps.end());
	profile_.openedPlanes.insert(player_->openedPlanes.begin(), player_->openedPlanes.end());
	//������� ���������
	std::cout << profile_.login 
		<< " ����� � ����� " << std::endl;

	//�������� �������, ��� ������ �����
	player_->isJoined = false;
	player_.reset();
	//�������� �������� �������
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
		throw rplanes::eClientStatusError( "��� ����� � ������� ���������� �����������. " );
	}
	if ( planeNo >= profile_.planes.size()  )
	{
		throw rplanes::eProfileError("�������� �������� ���������. ");
	}

	if (!profile_.planes[planeNo].isReadyForJoinRoom())
	{
		throw rplanes::eRoomError(" ������������ �������� �� �������� �����������. ������, ���������, ������ � ����� ������ ���� ����������� �����������. ");
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
		throw rplanes::eClientStatusError("������� �������� ������� ��� ������.");
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
		throw rplanes::eClientStatusError("������ ������ �� ����������������� �������.");
	}
	status_ = UNLOGINED;
	{
		MutexLocker locker( profilesInfo_.Mutex );
		if( profilesInfo_.loginedProfiles.erase(profile_.login) != 1 )
		{
			throw rplanes::eProfileError("������ ��������� �������������� �������������.");
		}
	}
	try 
	{
		profile_.save(profilesDB);
	}
	catch(odb::exception & e)
	{
		throw rplanes::eProfileError( std::string() + "������ ���� ������. " + e.what() );
	}
	std::cout << profile_.login<<" ����� �� ����" << std::endl;
}

void Client::login( std::string name, std::string password )
{
	if ( status_ != UNLOGINED )
	{
		throw rplanes::eLoginFail("������� ��������� �����������. ");
	}
	//����������� ��������� �������������� �������������
	MutexLocker locker( profilesInfo_.Mutex );
	//�������� ��������� �������
	if ( profilesInfo_.loginedProfiles.count(name) > 0 )
	{
		throw rplanes::eLoginFail("������� �����. ");
	}
	//��������� ��������� ������� � ����� ������
	try
	{
		odb::transaction t(profilesDB->begin());
		profile_ = *profilesDB->load<rplanes::playerdata::Profile>(name);
		t.commit();
	}
	catch(...)
	{
		throw rplanes::eLoginFail("��� ������������ ��� ������ ������� �������. ");
	}
	//�������� ������
	if ( profile_.password != password )
	{
		throw rplanes::eLoginFail("��� ������������ ��� ������ ������� �������. ");
	}
	//���� ��� ���������� �������� �������, ������ ����������� ��������
	//��������� �������� �������
	try
	{
		profile_.loadPlanes(profilesDB);
	}
	catch(...)
	{
		throw rplanes::eLoginFail("������ ���� ������. ");
	}
	//������������ ���
	profilesInfo_.loginedProfiles.insert(name);
	//�������� ������
	status_ = HANGAR;
	//������� ���������
	std::cout << profile_.login << " ����� � ���� � ������ " 
		<< connection_.getIP()
		<< std::endl;
}

void Client::setControllable( rplanes::serverdata::Plane::ControllableParameters controllable )
{
	if ( status_ != ROOM  || !player_)
	{
		throw rplanes::eClientStatusError("�������� ������ ���������� �� �� �������. ");
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
		throw rplanes::eClientConnectionFail("���������� ������������ ���������� ��������. ");
	}
	profilesInfo_.clientsCount++;
}

void Client::sendRoomMessages()
{
	if (status_ != ROOM)
	{
		throw rplanes::eClientStatusError("������� ��������� ���������� ������ �� �� �������. ");
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
		throw rplanes::eClientStatusError("������ �������� ����� �� �������� � �������. ");
	}
	player_->isJoined = false;
}
