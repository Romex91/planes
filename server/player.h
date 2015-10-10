#pragma once

#include "stdafx.h"
void correctAngle(float & x);

template< class T >
class ContainersMerger
{
public:
	template< class V >
	void addContainer(V & container)
	{
		for (auto & element : container)
		{
			elements_.push_back(&element);
		}
	}
	void for_each(std::function<void(T&)> handler)
	{
		for (auto & element : elements_)
		{
			handler(*element);
		}
	}
	void addElement(T & element)
	{
		elements_.push_back(&element);
	}
	void clear()
	{
		elements_.clear();
	}
private:
	std::vector< T * > elements_;
	
};


template<class T>
class updatable
{
public:
	updatable(const T & value) : value_(value), needUpdate_(true)
	{
	}
	updatable() : needUpdate_(true)
	{
	}
	const T & getValue() const
	{
		return value_;
	}
	T & operator()()
	{
		needUpdate_ = true;
		return value_;
	}
	//check if the value was changed after the last call of the method confirmUpdate
	bool isUpdated() const
	{
		return needUpdate_;
	}
	void confirmUpdate()
	{
		needUpdate_ = false;
	}

private:
	T value_;
	bool needUpdate_;
};

using namespace rplanes::network;

class Goal
{
public:
	Goal(float x = 0.f, float y = 0.f, float speed = 0.f, std::string descr = "")
	{
		position.x = x;
		position.y = y;
		recommendedSpeed = speed;
		description = descr;
	}
	rplanes::PointXY position;
	std::string description;
	short recommendedSpeed;
};

class HumanGroup;
class Player;

class DestroyablePlane : public rplanes::serverdata::Plane
{
public:
	DestroyablePlane(rplanes::serverdata::Plane & playerPlane);
	servermessages::room::DestroyPlanes::DestroyedPlane getDestructionInfo();
	bool isDestroyed()const;
	void respawn(float x, float y, float angle);
	void destroy(servermessages::room::DestroyPlanes::Reason reason, size_t modulePos);
	void setID(size_t id);
	int getTotalHp()const;
	void move(float frameTime);
	size_t getId();
	rplanes::serverdata::Plane::PlanePosition previousPosition;
private:
	bool destroyed_;
	//using in the updatePlayers method
	servermessages::room::DestroyPlanes::DestroyedPlane destructionInfo_;
	size_t id_;
};

rplanes::PointXY getDeflectedPoint(
	DestroyablePlane * target,
	rplanes::PointXY gunPosition,
	rplanes::planedata::Gun & gun,
	float shooterSpeed = 0.f);


class CombatSituation
{
public:
	CombatSituation(Player & player) : player_(player)
	{}
	CombatSituation(CombatSituation & cs) : player_(cs.player_)
	{}

	std::vector< DestroyablePlane * > getEnemies( float radius = -1.f);
	std::vector< DestroyablePlane * > getFriends( float radius = -1.f);
	std::vector< DestroyablePlane *> getPlanesInSector( std::vector<DestroyablePlane *> planes, float angle, float sector);
	DestroyablePlane & getThisPlane();

private:
	Player & player_;
};

class Player
{
public:
	friend class consoleHandler;
	friend class HumanGroup;
	friend class BotGroup;
	friend class CombatSituation;

	class messages_t
	{
	public:
		servermessages::room::CreatePlanes createPlanes;
		servermessages::room::DestroyPlanes destroyPlanes;
		servermessages::room::SetPlanesPositions setPlanesPositions;
		servermessages::room::UpdateModules updateModules;

		servermessages::room::CreateBullets createBullets;
		servermessages::room::CreateRicochetes createRicochetes;
		servermessages::room::DestroyBullets destroyBullets;

		servermessages::room::CreateMissiles createMissiles;
		servermessages::room::DestroyMissiles destroyMissiles;

		servermessages::room::InterfaceData interfaceData;

		std::vector< bidirectionalmessages::ResourceStringMessage > stringMessages;

		std::vector< servermessages::room::ChangeMap > changeMapMessages;

		std::vector< servermessages::room::RoomInfo > roomInfos;
	}messages;



	std::set<std::string> openedMaps;
	std::set<std::string> openedPlanes;

	bool isJoined;
	std::string name;

	updatable<Goal> goal;
	updatable< servermessages::room::RoomInfo::Statistics > killingStatistics;

	rplanes::playerdata::Statistics statistics;

	CombatSituation lookAround();

	void setID(size_t id);
	size_t getID();

	bool isZombie();

	Player(rplanes::serverdata::Plane & Plane, std::string Name);

	std::string getGroupName();


	rplanes::serverdata::Plane::PlanePosition getPosition();
	rplanes::serverdata::Plane::PlanePosition getPrevPosition();
	rplanes::Nation getNation();

	//the next methods does not use data of the other players

	void updateStaticalIfNeed();

	void respawn(float x, float y, float angle);
	void reload();

	//if the module has changed the state send the update message
	//if hp of some module < 0 or the plane is out of gas  then destroy the plane and update statistics
	bool destroyIfNeed(float frameTime);

	//destroy the plane updating statistics
	void destroy(servermessages::room::DestroyPlanes::Reason reason, size_t moduleNo);

	//destroy the plane without updating statistics
	void setDestroyed(servermessages::room::DestroyPlanes::Reason reason, size_t moduleNo);


	//move the plane and the bullets
	void move(float frameTime);

	void shoot(float frameTime, float serverTime, IdGetter & idGetter);

	void updateInterfaceMessage();

	void clearTemporaryData();

	bool isDestroyed()const;

	//the next methods use data of other players

	void controlTurrets( float frameTime);

	void updatePlayers();

	void addPlayer(std::shared_ptr<Player> Player);

	void checkCollisions();

	void updateProjectilesMessages(float serverTime);

	void updatePositionsMessage(float serverTime);

	void updateModulesMessage();

	std::string getPlaneName()const;

	void setControllable(rplanes::serverdata::Plane::ControllableParameters controllable);


private:
	//this class is using to handle projectiles hitting top and bottom of modules
	class CollisionsRegistrar
	{
	public:
		typedef  size_t ProjectileId;

		class CollisionInfo
		{
		public:
			CollisionInfo(size_t pId, size_t mNo) : planeId(pId), moduleNo(mNo)
			{};
			size_t planeId;
			size_t moduleNo;
			bool operator==(CollisionInfo ci)
			{
				return ci.planeId == planeId && ci.moduleNo == moduleNo;
			}
		};

		void handleCollision(ProjectileId projectileId, CollisionInfo collisionInfo)
		{
			auto & thisProjectileCollisions = collisions_[projectileId];
			//delete the collision if the projectile intersects the hitzone border the second time
			for (auto i = thisProjectileCollisions.begin();
				i != thisProjectileCollisions.end();
				i++)
			{
				if (collisionInfo == *i)
				{
					thisProjectileCollisions.erase(i);
					return;
				}
			}
			//ohterwise this intersection is the first for this module
			//so add the collision
			thisProjectileCollisions.push_back(collisionInfo);
		}

		std::vector<size_t> getIncludingModules(ProjectileId projectileId, size_t planeId)
		{
			std::vector<size_t> retval;
			auto i = collisions_.find(projectileId);
			if (i != collisions_.end())
			{
				for (auto & k : i->second)
				{
					if (k.planeId == planeId)
					{
						retval.push_back(k.moduleNo);
					}
				}
			}
			return retval;
		}

		void deleteProjectile(ProjectileId projectileId)
		{
			auto iter = collisions_.find(projectileId);
			if (iter != collisions_.end())
			{
				collisions_.erase(iter);
			}
		}

	private:
		std::map<ProjectileId, std::vector<CollisionInfo> > collisions_;
	}collisionsRegistrar_;

	class MessagesInfo
	{
	public:
		std::vector< servermessages::room::UpdateModules::Module> updatedModules;
		//updating in the method 'move' using in the method 'addPlayers'
		servermessages::room::Plane clientPlane;
		std::vector< rplanes::serverdata::Bullet > newBullets;
		std::vector< rplanes::serverdata::Bullet> newRicochetes;
		std::vector< servermessages::room::DestroyBullets::BulletInfo > destroyedBullets;
		std::vector< rplanes::serverdata::LaunchedMissile> newMissiles;
		std::vector<size_t> destroyedMissiles;
	}messagesInfo_;


	DestroyablePlane plane_;

	std::string groupName_;
	size_t id_;
	std::map< size_t, std::shared_ptr<Player> > visiblePlayers_;
	std::vector<rplanes::serverdata::Bullet> bullets_;
	std::vector<std::shared_ptr<Player>> missilePlayers_;
	std::vector<std::shared_ptr<Player>> bulletPlayers_;
	std::vector<std::shared_ptr<Player>> rammingPlayers_;

};
