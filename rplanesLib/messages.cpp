#include "messages.h"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#define _USE_MATH_DEFINES
#include < math.h >

template <typename Archive>
void rplanes::network::clientmessages::room::SendControllable::serialize( Archive& ar, const unsigned int version )
{
	ar & params.isShooting;
	ar & params.launchMissile;
	ar & params.power;
	ar & params.shootingDistanceOffset;
	ar & params.turningVal;
	ar & params.missileAim;
}


template <typename Archive>
void rplanes::network::clientmessages::hangar::RoomListRequest::serialize( Archive& ar, const unsigned int version )
{
}


template <typename Archive>
void rplanes::network::clientmessages::hangar::CreateRoomRequest::serialize( Archive& ar, const unsigned int version )
{
	ar & mapName;
	ar & description;
}


template <typename Archive>
void rplanes::network::clientmessages::hangar::JoinRoomRequest::serialize( Archive& ar, const unsigned int version )
{
	ar & planeNo;
	ar & playerName;
}


template <typename Archive>
void rplanes::network::clientmessages::hangar::ProfileRequest::serialize( Archive& ar, const unsigned int version )
{
}

template <typename Archive>
void rplanes::network::clientmessages::hangar::PlayerProfileRequest::serialize(Archive& ar, const unsigned int version)
{
	ar & playerName;
}


template <typename Archive>
void rplanes::network::clientmessages::unlogined::Login::serialize( Archive& ar, const unsigned int version )
{
	ar & name;
	ar & encryptedPassword;
}


template <typename Archive>
void rplanes::network::clientmessages::unlogined::Registry::serialize( Archive& ar, const unsigned int version )
{
	ar & name;
	ar & password;
}


template <typename Archive>
void rplanes::network::clientmessages::hangar::BuyPlaneRequest::serialize( Archive& ar, const unsigned int version )
{
	ar & planeName;
}


template <typename Archive>
void rplanes::network::clientmessages::hangar::BuyModuleRequest::serialize( Archive& ar, const unsigned int version )
{
	ar & moduleName;
	ar & planeName;
	ar & setToAllSlots;
	ar & moduleNo;
	ar & moduleType;
}


template <typename Archive>
void rplanes::network::clientmessages::hangar::SellPlaneRequest::serialize( Archive& ar, const unsigned int version )
{
	ar & planeName;
}


template <typename Archive>
void rplanes::network::clientmessages::hangar::SellModuleRequest::serialize( Archive& ar, const unsigned int version )
{
	ar & moduleName;
	ar & nModulesToSell;
}


template <typename Archive>
void rplanes::network::clientmessages::hangar::UpSkillRequest::serialize( Archive& ar, const unsigned int version )
{
	ar & skill;
	ar & experienceToSpend;
}


template <typename Archive>
void rplanes::network::servermessages::room::InterfaceData::serialize( Archive& ar, const unsigned int version )
{
	ar & faintVal;
	ar & aimSize;
	ar & shootingDistance;
	ar & Vmax;
	ar & Vmin;
	ar & V;

	ar & thermometers;
	ar & ammunitions;
	ar & gasTank;
}



template <typename Archive>
void rplanes::network::servermessages::room::CreateBullets::serialize( Archive& ar, const unsigned int version )
{
	ar & bullets;
	ar & time;
}


template <typename Archive>
void rplanes::network::servermessages::room::CreateRicochetes::serialize( Archive& ar, const unsigned int version )
{
	ar & bullets;
	ar & time;
}


template <typename Archive>
void rplanes::network::servermessages::room::CreateMissiles::serialize( Archive& ar, const unsigned int version )
{
	ar & missiles;
	ar & time;
}


template <typename Archive>
void rplanes::network::servermessages::room::DestroyBullets::serialize( Archive& ar, const unsigned int version )
{
	ar & bullets;
}


template <typename Archive>
void rplanes::network::servermessages::room::DestroyBullets::BulletInfo::serialize( Archive& ar, const unsigned int version )
{
	ar & bulletID;
	ar & reason;
}


template <typename Archive>
void rplanes::network::servermessages::room::DestroyMissiles::serialize( Archive& ar, const unsigned int version )
{
	ar & ids;
}


template <typename Archive>
void rplanes::network::servermessages::room::Plane::serialize( Archive& ar, const unsigned int version )
{
	ar & planeName;
	ar & playerName;
	ar & nation;
	ar & id;
	ar & pos;
	ar & extrapolationData;
	ar & modules;
}

void rplanes::network::servermessages::room::Plane::extrapolate( float frameTime )
{
	//поступательное движение
	extrapolationData.speed += extrapolationData.acceleration * frameTime;
	pos.x += extrapolationData.speed * std::cos( pos.angle / 180.0 * M_PI) * frameTime * configuration().flight.speedFactor;
	pos.y += extrapolationData.speed * std::sin( pos.angle / 180.0 * M_PI ) * frameTime * configuration().flight.speedFactor;
	//вращательное движение
	pos.angle += extrapolationData.angleVelocity 
		* frameTime * configuration().flight.angleVelocityFactor;
}

void rplanes::network::servermessages::room::Plane::init( const serverdata::Plane & Plane , std::string PlayerName, size_t ID )
{
	planeName = Plane.name;
	playerName = PlayerName;
	id = ID;
	nation = Plane.nation;
	pos.x = Plane.position.x;
	pos.y = Plane.position.y;
	pos.roll = Plane.position.roll;
	pos.angle = Plane.position.angle;

	extrapolationData.acceleration = Plane.target.acceleration;
	extrapolationData.angleVelocity = Plane.target.angularVelocity;
	extrapolationData.speed = Plane.target.V;
	modules.clear();
	for( auto & module : Plane.modules )
	{
		Module m;
		m.defected = module->hp.isDefected();
		m.hp = module->hp;
		m.hpMax = module->hpMax;
		m.hitZone = module->hitZone;
		m.type = module->getType();
		modules.push_back(m);
	}
}


template <typename Archive>
void rplanes::network::servermessages::room::Plane::Module::serialize( Archive& ar, const unsigned int version )
{
	ar & hp;
	ar & hpMax;
	ar & hitZone;
	ar & defected;
	ar & type;
}


template <typename Archive>
void rplanes::network::servermessages::room::CreatePlanes::serialize( Archive& ar, const unsigned int version )
{
	ar & Planes;
}


template <typename Archive>
void rplanes::network::servermessages::room::SetPlanesPositions::serialize( Archive& ar, const unsigned int version )
{
	ar & positions;
	ar & time;
}


template <typename Archive>
void rplanes::network::servermessages::room::SetPlanesPositions::PlanePos::serialize( Archive& ar, const unsigned int version )
{
	ar & planeID;
	ar & pos;
	ar & extrapolationData;
}


template <typename Archive>
void rplanes::network::servermessages::room::UpdateModules::serialize( Archive& ar, const unsigned int version )
{
	ar & modules;
}


template <typename Archive>
void rplanes::network::bidirectionalmessages::TextMessage::serialize( Archive& ar, const unsigned int version )
{
	ar & text;
}


template <typename Archive>
void rplanes::network::bidirectionalmessages::ExitRoom::serialize( Archive& ar, const unsigned int version )
{

}


template <typename Archive>
void rplanes::network::servermessages::room::Plane::Position::serialize( Archive& ar, const unsigned int version )
{
	ar & x;
	ar & y;
	ar & angle;
	ar & roll;
}


template <typename Archive>
void rplanes::network::servermessages::room::Plane::ExtrapolationData::serialize( Archive& ar, const unsigned int version )
{
	ar & speed;
	ar & angleVelocity;
	ar & acceleration;
}


template <typename Archive>
void rplanes::network::servermessages::room::UpdateModules::Module::serialize( Archive& ar, const unsigned int version )
{
	ar & planeID;
	ar & moduleNo;
	ar & hp;
	ar & defect;
	ar & reason;
}


template <typename Archive>
void rplanes::network::servermessages::hangar::SendProfile::serialize( Archive& ar, const unsigned int version )
{
	ar & profile;
}


template <typename Archive>
void rplanes::network::servermessages::room::RoomInfo::serialize( Archive& ar, const unsigned int version )
{
	ar & disconnectedPlayers;
	ar & newPlayers;
	ar & updatedStatistics;
	ar & goal;
}

void rplanes::network::servermessages::room::RoomInfo::clear()
{
	newPlayers.clear();
	disconnectedPlayers.clear();
	updatedStatistics.clear();
}



template <typename Archive>
void rplanes::network::servermessages::hangar::RoomList::serialize( Archive& ar, const unsigned int version )
{
	ar & rooms;
}


template <typename Archive>
void rplanes::network::servermessages::hangar::RoomList::RoomInfo::serialize( Archive& ar, const unsigned int version )
{
	ar & mapName;
	ar & slots;
	ar & creatorName;
	ar & description;
}


template <typename Archive>
void rplanes::network::servermessages::room::DestroyPlanes::serialize( Archive& ar, const unsigned int version )
{
	ar & planes;
}

template <typename Archive>
void rplanes::network::servermessages::room::ChangeMap::serialize(Archive& ar, const unsigned int version)
{
	ar & mapName;
}


template <typename Archive>
void rplanes::network::servermessages::room::DestroyPlanes::DestroyedPlane::serialize( Archive& ar, const unsigned int version )
{
	ar & moduleNo;
	ar & planeID;
	ar & this->killerId;
	ar & this->nation;
	ar & reason;
}


template <typename Archive>
void rplanes::network::servermessages::hangar::ServerConfiguration::serialize( Archive& ar, const unsigned int version )
{
	ar & conf.collisions.rammingDistance;
	ar & conf.collisions.visibilityDistance;

	ar & conf.defect.chanceFactor;
	ar & conf.defect.mobilityFactor;
	ar & conf.defect.pilotSkillsFactor;
	ar & conf.defect.shootRateFactor;
	ar & conf.defect.surfaceFactor;
	ar & conf.defect.tMaxFactor;
	ar & conf.defect.vMaxFactor;

	ar & conf.engine.additionalPowerFactor;
	ar & conf.engine.overheatDamageFactor;
	ar & conf.engine.overheatDefectChanceFactor;

	ar & conf.flight.angleAccelerationFactor;
	ar & conf.flight.angleVelocityFactor;
	ar & conf.flight.forceFactor;
	ar & conf.flight.maneuverFrictionFactor;
	ar & conf.flight.maxAngularVelocityFactor;
	ar & conf.flight.reverseAngleAccelerationFactor;
	ar & conf.flight.speedFactor;
	ar & conf.flight.turningExp;

	ar & conf.missile.accelerationFactor;
	ar & conf.missile.accuracyFactor;
	ar & conf.missile.damageFactor;
	ar & conf.missile.gravity;
	ar & conf.missile.radiusFactor;
	ar & conf.missile.speedAccuracyFactor;
	ar & conf.missile.speedFactor;
	ar & conf.missile.ttlFactor;

	ar & conf.pilot.pilotFaintTime;
	ar & conf.pilot.pilotMaxG;

	ar & conf.profile.damagePenalty;
	ar & conf.profile.damageReward;
	ar & conf.profile.hitExperience;
	ar & conf.profile.maxBanlistSize;
	ar & conf.profile.startExp;
	ar & conf.profile.startMoney;

	ar & conf.profile.startMaps;
	ar & conf.profile.startPlanes;

	ar & conf.server.hangarFrameTime;
	ar & conf.server.hangarMessagesPerFrame;
	ar & conf.server.maxClientsNumber;
	ar & conf.server.numThreads;
	ar & conf.server.port;
	ar & conf.server.roomFrameTime;
	ar & conf.server.roomMessagesPerFrame;
	ar & conf.server.unloginedDisconnectTime;

	ar & conf.shooting.accelerationFactor;
	ar & conf.shooting.accuracyFactor;
	ar & conf.shooting.angleImpactFactor;
	ar & conf.shooting.damageFactor;
	ar & conf.shooting.gravity;
	ar & conf.shooting.impactFactor;
	ar & conf.shooting.impactRandomnesFactor;
	ar & conf.shooting.maxDistance;
	ar & conf.shooting.minDistance;
	ar & conf.shooting.penetFactor;
	ar & conf.shooting.shootRateFactor;
	ar & conf.shooting.speedFactor;
	ar & conf.shooting.ttlFactor;
}


template <typename Archive>
void rplanes::network::servermessages::StatusMessage::serialize( Archive& ar, const unsigned int version )
{
	ar & status;
}


template <typename Archive>
void rplanes::network::servermessages::room::ServerTime::serialize( Archive& ar, const unsigned int version )
{
	ar & time;
}

template<class Archive>
void boost::serialization::serialize( Archive & ar, rplanes::playerdata::Statistics & Stat, const unsigned int version )
{
	ar & Stat.friendlyDamage;
	ar & Stat.damage;
	ar & Stat.damageReceived;

	ar & Stat.friensDestroyed;
	ar & Stat.enemyDestroyed;

	ar & Stat.shots;

	ar & Stat.hits;
	ar & Stat.hitsReceived;

	ar & Stat.crashes;

	ar & Stat.exp;
	ar & Stat.money;
}

template<class Archive>
void boost::serialization::serialize( Archive & ar, rplanes::playerdata::Plane & pt, const unsigned int version )
{
	ar & pt.ammunitions;
	ar & pt.cabine;
	ar & pt.engines;
	ar & pt.framework;
	ar & pt.guns;
	ar & pt.id.planeName;
	ar & pt.id.profileName;
	ar & pt.missiles;
	ar & pt.nation;
	ar & pt.tail;
	ar & pt.tanks;
	ar & pt.wings;
}

template<class Archive>
void boost::serialization::serialize( Archive & ar, rplanes::playerdata::Profile & Profile, const unsigned int version )
{
	ar & Profile.statistics;
	ar & Profile.planes;
	ar & Profile.pilot;
	ar & Profile.money;
	ar & Profile.moduleStore;
	ar & Profile.login;
	ar & Profile.banlist;
	ar & Profile.openedMaps;
	ar & Profile.openedPlanes;
}

template<class Archive>
void boost::serialization::serialize( Archive & ar, rplanes::serverdata::LaunchedMissile & Missile, const unsigned int version )
{
	ar & Missile.acceleration;
	ar & Missile.angleXY;
	ar & Missile.planeID;
	ar & Missile.damage;
	ar & Missile.ID;
	ar & Missile.radius;
	ar & Missile.speedXY;
	ar & Missile.speedZ;
	ar & Missile.startSpeed;
	ar & Missile.startTTL;
	ar & Missile.TTL;
	ar & Missile.x;
	ar & Missile.y;
	ar & Missile.z;
	ar & Missile.model;
}

template<class Archive>
void boost::serialization::serialize( Archive & ar, rplanes::serverdata::Bullet & Bullet, const unsigned int version )
{
	ar & Bullet.acceleration;
	ar & Bullet.angleXY;
	ar & Bullet.planeID;
	ar & Bullet.gunNo;
	ar & Bullet.damage;
	ar & Bullet.ID;
	ar & Bullet.penetration;
	ar & Bullet.speedXY;
	ar & Bullet.x;
	ar & Bullet.y;
	ar & Bullet.z;
	ar & Bullet.speedZ;
	ar & Bullet.startSpeed;
	ar & Bullet.caliber;
}

template <typename Archive>
void rplanes::network::clientmessages::hangar::DestroyRoomRequest::serialize(Archive& ar, const unsigned int version)
{

}

template <typename Archive>
void rplanes::network::clientmessages::room::AdministerRoom::serialize(Archive& ar, const unsigned int version)
{
	ar & operation;
	ar & options;
}

template <typename Archive>
void rplanes::network::servermessages::room::RoomInfo::NewPlayerInfo::serialize(Archive& ar, const unsigned int version)
{
	ar & name;
	ar & planeName;
	ar & nation;
}

template <typename Archive>
void rplanes::network::servermessages::room::RoomInfo::Goal::serialize(Archive& ar, const unsigned int version)
{
	ar & position;
	ar & description;
	ar & recommendedSpeed;
}

template <typename Archive>
void rplanes::network::servermessages::room::RoomInfo::Statistics::serialize(Archive& ar, const unsigned int version)
{
	ar & destroyed;
	ar & friendsDestroyed;
	ar & crashes;
}

template <typename Archive>
void rplanes::network::servermessages::hangar::RoomList::RoomInfo::SlotInfo::serialize(Archive& ar, const unsigned int version)
{
	ar & nPlayers;
	ar & nPlayersMax;
}

rplanes::network::servermessages::hangar::RoomList::RoomInfo::SlotInfo::SlotInfo() : nPlayers(0), nPlayersMax(0)
{

}

template <typename Archive>
void rplanes::network::bidirectionalmessages::ResourceStringMessage::serialize(Archive& ar, const unsigned int version)
{
	ar & string;
}
