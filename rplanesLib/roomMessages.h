#pragma  once
#include "plane.h"
#include "configuration.h"
#include "messagesRegistration.h"

namespace boost {
	namespace serialization {
		template<class Archive>
		void serialize(Archive & ar, rplanes::serverdata::LaunchedMissile & Missile, const unsigned int version)
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
		void serialize(Archive & ar, rplanes::serverdata::Bullet & Bullet, const unsigned int version)
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
	}
}

namespace rplanes {
	namespace network {

		//////////////////////////////////////////////////////////////////////////
		//messages sending by clients
		//////////////////////////////////////////////////////////////////////////

		class MServerTimeRequest : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version){}
			RPLANES_MESSAGE_ID(14);
		};
		RPLANES_REGISTER_MESSAGE(MServerTimeRequest);

		class MSendControllable : public MessageBase, public serverdata::Plane::ControllableParameters
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & isShooting;
				ar & launchMissile;
				ar & power;
				ar & shootingDistanceOffset;
				ar & turningVal;
				ar & missileAim;
			}
			RPLANES_MESSAGE_ID(15);
		};
		RPLANES_REGISTER_MESSAGE(MSendControllable);

		//available only for the room master
		class MAdministerRoom : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & operation;
				ar & options;
			}
			enum Operation
			{
				//options must contain players to kick
				KICK_PLAYERS,
				//options mast contain players to ban
				BAN_PLAYERS,
				//the only option is a map name
				CHANGE_MAP,
				RESTART,
				//options mast contain players to destroy their planes
				KILL_PLAYERS,
				//options mast contain players to unban
				UNBAN_PLAYERS
			};
			Operation operation;
			std::vector<std::string> options;
			RPLANES_MESSAGE_ID(16);
		};
		RPLANES_REGISTER_MESSAGE(MAdministerRoom);

		//////////////////////////////////////////////////////////////////////////
		//messages sending by the server
		//////////////////////////////////////////////////////////////////////////
		class MCreateBullets : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & bullets;
				ar & time;
			}

			std::vector <serverdata::Bullet> bullets;
			float time;
			RPLANES_MESSAGE_ID(18);
		};
		RPLANES_REGISTER_MESSAGE(MCreateBullets);

		class MInterfaceData : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
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

			unsigned short
				//- g-force vision blackout level  [0,100]
				faintVal = 0,
				//- aim marker radius in meters
				aimSize = 0,
				//- a distance between guns and the aim marker. meters
				shootingDistance = 0,
				Vmax = 0,
				Vmin = 0,
				V = 0;
			class Thermometer
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & criticalTemperature;
					ar & temperature;
					ar & dTmax;
					ar & dT;
				}
				short int
					// 0.1 degree Celsius
					criticalTemperature,
					// 0.1 degree Celsius
					temperature,
					// 0.1 degree Celsius
					dTmax,
					// 0.1 degree Celsius
					dT;
			};
			//temperature date for each engine
			std::vector<Thermometer> thermometers;

			class Ammo
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & caliber;
					ar & capacity;
				}

				unsigned short
					//0.01 mm. caliber 7.62 would be 762
					caliber,
					capacity;
			};
			std::vector<Ammo> ammunitions;

			class GasTank
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & fuel;
					ar & capacity;
				}
				unsigned short
					fuel,
					capacity;
			} gasTank;

			void update(const serverdata::Plane & Plane);

			RPLANES_MESSAGE_ID(19);
		};
		RPLANES_REGISTER_MESSAGE(MInterfaceData);


		//logicaly identical to CreateBullets 
		class MCreateRicochetes : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & bullets;
				ar & time;
			}

			std::vector <serverdata::Bullet> bullets;
			float time;
			RPLANES_MESSAGE_ID(20);
		};
		RPLANES_REGISTER_MESSAGE(MCreateRicochetes);

		class MCreateMissiles : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & missiles;
				ar & time;
			}

			std::vector<serverdata::LaunchedMissile> missiles;
			float time;
			RPLANES_MESSAGE_ID(21);
		};
		RPLANES_REGISTER_MESSAGE(MCreateMissiles);

		//client also should check isSpent method and delete the bullet if it returns true
		class MDestroyBullets : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & bullets;
			}

			class BulletInfo
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & bulletID;
					ar & reason;
				}


				size_t bulletID;
				enum Reason
				{
					HIT,
					RICOCHET
				} reason;
			};
			std::vector<BulletInfo> bullets;
			RPLANES_MESSAGE_ID(22)
		};
		RPLANES_REGISTER_MESSAGE(MDestroyBullets);

		//client also should check isSpent method and delete the bullet if it returns true
		class MDestroyMissiles : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & ids;
			}
			std::vector<size_t> ids;
			RPLANES_MESSAGE_ID(23)
		};
		RPLANES_REGISTER_MESSAGE(MDestroyMissiles);

		class MChangeMap : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & mapName;
			}
			std::string mapName;
			RPLANES_MESSAGE_ID(24);
		};
		RPLANES_REGISTER_MESSAGE(MChangeMap);



		//new plane in the vision zone
		class MCreatePlanes : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & planes;
			}


			//the plane data transmitting from the server to clients
			class Plane
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & planeName;
					ar & playerName;
					ar & nation;
					ar & id;
					ar & pos;
					ar & extrapolationData;
					ar & modules;
				}


				std::string planeName,
					playerName;
				Nation nation;
				size_t id;

				class Position
				{
				public:
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version)
					{
						ar & x;
						ar & y;
						ar & angle;
						ar & roll;
					}
					float x,
						y,
						angle,
						roll;
				} pos;

				class  ExtrapolationData
				{
				public:
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version)
					{
						ar & speed;
						ar & angleVelocity;
						ar & acceleration;
					}
					float speed,
						angleVelocity,
						acceleration;
				} extrapolationData;

				class Module
				{
				public:
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version)
					{
						ar & hp;
						ar & hpMax;
						ar & hitZone;
						ar & defected;
						ar & type;
					}
					unsigned short hp, hpMax;
					serverdata::HitZone hitZone;
					bool defected;
					ModuleType type;
				};
				std::vector<Module> modules;

				//server-only method
				void init(const serverdata::Plane & Plane, std::string PlayerName, size_t ID);
				//client-only method
				void extrapolate(float frameTime);
			};
			std::vector< Plane > planes;
			RPLANES_MESSAGE_ID(25);
		};
		RPLANES_REGISTER_MESSAGE(MCreatePlanes);

		//specific plane is downing or vanished
		class MDestroyPlanes : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & planes;
			}
			enum Reason
			{
				MODULE_DESTROYED,
				RAMMED,
				FUEL,
				FIRE,
				VANISH
			};
			class DestroyedPlane
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & moduleNo;
					ar & planeID;
					ar & this->killerId;
					ar & this->nation;
					ar & reason;
				}

				//the module that initiated the downing
				size_t moduleNo;
				size_t planeID;
				size_t killerId;
				Reason reason;
				rplanes::Nation nation;
			};
			std::vector<DestroyedPlane> planes;
			RPLANES_MESSAGE_ID(26);
		};
		RPLANES_REGISTER_MESSAGE(MDestroyPlanes);

		//positions off all the planes in the vision zone
		class MPlanesPositions : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & positions;
				ar & time;
			}
			class PlanePos
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & planeID;
					ar & pos;
					ar & extrapolationData;
				}

				size_t planeID;
				MCreatePlanes::Plane::Position pos;
				MCreatePlanes::Plane::ExtrapolationData extrapolationData;
			};
			std::vector< PlanePos> positions;
			float time;
			RPLANES_MESSAGE_ID(27);
		};
		RPLANES_REGISTER_MESSAGE(MPlanesPositions);

		class MUpdateModules : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & modules;
			}

			class Module
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
						ar & planeID;
						ar & moduleNo;
						ar & hp;
						ar & defect;
						ar & reason;
				}
				size_t planeID, moduleNo;
				unsigned short hp;
				bool defect;
				planedata::ModuleHP::ChangeReason reason;
			};
			std::vector< Module > modules;
			RPLANES_MESSAGE_ID(28);
		};
		RPLANES_REGISTER_MESSAGE(MUpdateModules);

		//servers sends its time when sending each frame
		class MServerTime : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & time;
			}
			float time;
			RPLANES_MESSAGE_ID(29);
		};
		RPLANES_REGISTER_MESSAGE(MServerTime);

		class MRoomInfo : public MessageBase
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & disconnectedPlayers;
				ar & newPlayers;
				ar & updatedStatistics;
				ar & goal;
			}
			class NewPlayerInfo
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & name;
					ar & planeName;
					ar & nation;
				}
				std::string name;
				std::string planeName;
				Nation nation;
			};
			class Goal
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & position;
					ar & description;
					ar & recommendedSpeed;
				}
				rplanes::PointXY position;
				std::string description;
				short recommendedSpeed;
			};
			class Statistics
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & destroyed;
					ar & friendsDestroyed;
					ar & crashes;
				}
				int destroyed,
					friendsDestroyed,
					crashes;
			};

			void clear();

			std::vector<NewPlayerInfo> newPlayers;

			std::vector<std::string> disconnectedPlayers;

			//key is player name
			std::map< std::string, Statistics > updatedStatistics;

			Goal goal;

			RPLANES_MESSAGE_ID(30);
		};
		RPLANES_REGISTER_MESSAGE(MRoomInfo);
	}
}