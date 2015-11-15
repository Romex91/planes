#include "client.h"
using namespace rplanes;
extern std::shared_ptr<odb::database> planesDB;
extern std::shared_ptr<odb::database> profilesDB;

Client::ProfilesInfo Client::profilesInfo_;

bool clientIsInRoom( size_t clientID )
{
	return clientID >= configuration().server.maxClientsNumber;
}

size_t convertIDToPos( size_t clienID )
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
		throw PlanesException( _rstrw("Client is out of any room."));
	}
	status_ = HANGAR;
	

	//saving the progress
	profile_.statistics[player_->getPlaneName()]+= player_->statistics;
	profile_.statistics["total"]+=player_->statistics;
	profile_.money+= player_->statistics.money;
	profile_.pilot.addExp(player_->statistics.exp);
	
	profile_.openedMaps.insert(player_->openedMaps.begin(), player_->openedMaps.end());
	profile_.openedPlanes.insert(player_->openedPlanes.begin(), player_->openedPlanes.end());
	//logging the message
	std::wcout <<  _rstrw("{0} leaved room.", profile_.login).str() << std::endl;

	//reporting to the room that client logged out
	player_->isJoined = false;
	player_.reset();
	//trying to send a message to a client
	try
	{
		sendMessage(network::MExitRoom());
	}
	catch (...)
	{}
}

void Client::joinRoom( Room & room, size_t planeNo)
{
	if ( status_ != HANGAR )
	{
		throw RPLANES_EXCEPTION("Cannot join room. Client is out of hangar.");
	}
	if ( planeNo >= profile_.planes.size()  )
	{
		throw RPLANES_EXCEPTION("Cannot join room. Wrong parameters.");
	}

	if (!profile_.planes[planeNo].isReadyForJoinRoom())
	{
		throw RPLANES_EXCEPTION("Cannot join room. Ensure that wings engines missiles and guns are mounted symmetrically");
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
		throw RPLANES_EXCEPTION("Cannot get profile. Client is out of hangar.");
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
		throw PlanesException (_rstrw("Client is not logged in."));
	}
	status_ = UNLOGGED;
	{
		MutexLocker locker( profilesInfo_.Mutex );
		if( profilesInfo_.loggedInProfiles.erase(profile_.login) != 1 )
		{
			throw RPLANES_EXCEPTION("the profiles set does not contain {0}", profile_.login);
		}
	}
	profile_.save(profilesDB);
	std::wcout << _rstrw("{0} logged out.", profile_.login).str() << std::endl;
}

void Client::login( std::string name, std::string password )
{
	if ( status_ != UNLOGGED )
	{
		throw RPLANES_EXCEPTION("Client has already logged in.");
	}

	//check if the profile is already locked
	MutexLocker locker( profilesInfo_.Mutex );
	if ( profilesInfo_.loggedInProfiles.count(name) > 0 )
	{
		throw RPLANES_EXCEPTION("{0} is locked by other player.");
	}
	//trying to load the profile from database
	try
	{
		odb::transaction t(profilesDB->begin());
		profile_ = *profilesDB->load<rplanes::playerdata::Profile>(name);
		t.commit();
	}
	catch(...)
	{
		throw RPLANES_EXCEPTION("Wrong name or password.");
	}
	//verifying the password
	if ( profile_.password != password )
	{
		throw RPLANES_EXCEPTION("Wrong name or password.");
	}
	//if all the verifications passed tha authorization succeedeed
	//loading profile planes
	profile_.loadPlanes(profilesDB);
	//locking the profile
	profilesInfo_.loggedInProfiles.insert(name);
	status_ = HANGAR;

	std::wcout << _rstrw("{0} logged in from ip {1}", profile_.login, connection_.getIP()).str() << std::endl;
}

void Client::setControllable( serverdata::Plane::ControllableParameters controllable )
{
	if ( status_ != ROOM  || !player_)
	{
		throw RPLANES_EXCEPTION("Cannot set controllable parameters. Client is out of room.");
	}
	player_->setControllable(controllable);
}

void Client::setID( size_t id )
{
	id_ = id;
}

Client::~Client()
{
	MutexLocker ml( profilesInfo_.Mutex );
	profilesInfo_.clientsCount--;
}

Client::Client( boost::asio::io_service& io_service , size_t clientID /*= 0 */ ) :
	connection_(io_service),
	disconnectTimer_( configuration().server.unloginedDisconnectTime ),
	status_(UNLOGGED)
{
	setID(clientID);
	MutexLocker ml( profilesInfo_.Mutex );
	if ( profilesInfo_.clientsCount >= configuration().server.maxClientsNumber )
	{
		throw RPLANES_EXCEPTION("Server is overloaded.");
	}
	profilesInfo_.clientsCount++;
}

void Client::sendRoomMessages()
{
	if (status_ != ROOM)
	{
		throw RPLANES_EXCEPTION("Failed sanding room messages. Client is out of room.");
	}
	if (player_->messages.createPlanes.planes.size() > 0)
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
	for (auto & message : player_->messages.stringMessages)
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
		throw RPLANES_EXCEPTION("Cannot exit room. Client is out of room.");
	}
	player_->isJoined = false;
}

void Client::setMessageHandlers()
{
	connection_.setHandler<MStatusRequest>([this](const MStatusRequest &) {
		MStatus mess;
		mess.status = status_;
		sendMessage(mess);
	});

	connection_.setHandler<MSendControllable>(std::bind(&Client::setControllable, this, std::placeholders::_1));


	connection_.setHandler<MProfileRequest>([this](const MProfileRequest &) {
		network::MProfile mess;
		mess.profile = profile();
		sendMessage(mess);
	});

	connection_.setHandler<MSellModuleRequest>([this](const MSellModuleRequest & message) {
		sendMessage(MResourceString(profile().sellModule(message.moduleName, message.nModulesToSell, planesDB)));
	});

	connection_.setHandler<MSellPlaneRequest>([this](const MSellPlaneRequest & message) {
		sendMessage(MResourceString(profile().sellPlane(message.planeName, planesDB)));
	});
	connection_.setHandler<MBuyModuleRequest>([this](const MBuyModuleRequest & message) {
		if (message.setToAllSlots) {
			network::MResourceString mess;
			sendMessage(MResourceString(profile().buyModules(message.planeName, message.moduleName, planesDB)));
		} else {
			sendMessage(MResourceString(profile().buyModule(message.planeName, message.moduleNo, message.moduleName, planesDB)));
		}
	});

	connection_.setHandler<MBuyPlaneRequest>([this](const MBuyPlaneRequest & message) {
		sendMessage(MResourceString(profile().buyPlane(message.planeName, planesDB)));
	});
	connection_.setHandler<MUpSkillRequest>([this](const MUpSkillRequest & message) {
		auto & pilot = profile().pilot;
		switch (message.skill)
		{
		case MUpSkillRequest::FLIGHT:
			pilot.up_flight(message.experienceToSpend);
			break;
		case MUpSkillRequest::ENDURANCE:
			pilot.up_endurance(message.experienceToSpend);
			break;
		case MUpSkillRequest::SHOOTING:
			pilot.up_shooting(message.experienceToSpend);
			break;
		case MUpSkillRequest::ENGINE:
			pilot.up_engine(message.experienceToSpend);
			break;
		}
	});
	connection_.setHandler<MLogin>([this](const MLogin & message) {
		login(message.name, message.encryptedPassword);
		sendMessage(MServerConfiguration(configuration()));
	});

	connection_.setHandler<MExitRoom>([this](const MExitRoom & message) {
		prepareRoomExit();
	});

}

