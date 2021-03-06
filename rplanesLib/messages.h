#pragma once
#include "profile.h"
#include "configuration.h"
#include "messagesRegistration.h"

namespace boost {
	namespace serialization {
		template<class Archive>
		void serialize(Archive & ar, rplanes::playerdata::Statistics & Stat, const unsigned int version)
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
		void serialize(Archive & ar, rplanes::playerdata::Plane & pt, const unsigned int version)
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
		void serialize(Archive & ar, rplanes::playerdata::Profile & Profile, const unsigned int version)
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

	} // namespace serialization
} // namespace boost

namespace rplanes
{
	namespace network
	{

		//messages coming from the client side
		//client status request
		class MStatusRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
			}
			RPLANES_MESSAGE_ID(0);
		};
		RPLANES_REGISTER_MESSAGE(MStatusRequest);

		class MLogin : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & name;
				ar & encryptedPassword;
			}
			std::string name;
			std::string encryptedPassword;
			RPLANES_MESSAGE_ID(1);
		};
		RPLANES_REGISTER_MESSAGE(MLogin);

		class MRegistry : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & name;
				ar & password;
			}

			std::string name, password;
			RPLANES_MESSAGE_ID(2)
		};
		RPLANES_REGISTER_MESSAGE(MRegistry);


		class MRoomListRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version){}
			RPLANES_MESSAGE_ID(3)
		};
		RPLANES_REGISTER_MESSAGE(MRoomListRequest);

		class MCreateRoomRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & mapName;
				ar & description;
			}
			std::string description;
			std::string mapName;
			RPLANES_MESSAGE_ID(4)
		};
		RPLANES_REGISTER_MESSAGE(MCreateRoomRequest);

		class MDestroyRoomRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{}
			RPLANES_MESSAGE_ID(5)
		};
		RPLANES_REGISTER_MESSAGE(MDestroyRoomRequest);


		class MJoinRoomRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & planeNo;
				ar & ownerName;
			}

			size_t planeNo;
			std::string ownerName;
			RPLANES_MESSAGE_ID(6)
		};
		RPLANES_REGISTER_MESSAGE(MJoinRoomRequest);

		class MProfileRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
			}
			RPLANES_MESSAGE_ID(7)
		};
		RPLANES_REGISTER_MESSAGE(MProfileRequest);

		//get random player profile data
		class MPlayerProfileRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & playerName;
			}

			std::string playerName;
			RPLANES_MESSAGE_ID(8)
		};
		RPLANES_REGISTER_MESSAGE(MPlayerProfileRequest);

		class MBuyPlaneRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & planeName;
			}

			std::string planeName;
			RPLANES_MESSAGE_ID(9)
		};
		RPLANES_REGISTER_MESSAGE(MBuyPlaneRequest);

		//If such a module isn't persisting in the store this action will buy the module
		class MBuyModuleRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & moduleName;
				ar & planeName;
				ar & setToAllSlots;
				ar & moduleNo;
				ar & moduleType;
			}

			//a plane to store the module
			std::string planeName;
			std::string moduleName;
			bool setToAllSlots;
			//if setToAllSlots is true the moduleNo is ingored
			size_t moduleNo;
			rplanes::ModuleType moduleType;
			RPLANES_MESSAGE_ID(10);
		};
		RPLANES_REGISTER_MESSAGE(MBuyModuleRequest);

		//sell a plane with all mounted modules
		class MSellPlaneRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & planeName;
			}

			std::string planeName;
			RPLANES_MESSAGE_ID(11);
		};
		RPLANES_REGISTER_MESSAGE(MSellPlaneRequest);

		//sell a module from the store
		class MSellModuleRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & moduleName;
				ar & nModulesToSell;
			}

			std::string moduleName;
			size_t nModulesToSell;
			RPLANES_MESSAGE_ID(12)
		};
		RPLANES_REGISTER_MESSAGE(MSellModuleRequest);

		class MUpSkillRequest : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & skill;
				ar & experienceToSpend;
			}
			enum Skill{ FLIGHT, ENDURANCE, SHOOTING, ENGINE } skill;
			size_t experienceToSpend;
			RPLANES_MESSAGE_ID(13)
		};
		RPLANES_REGISTER_MESSAGE(MUpSkillRequest);

		enum  ClientStatus
		{
			//if a client has this status for too long the connection would be closed
			UNLOGGED,
			HANGAR,
			ROOM
		};
		//////////////////////////////////////////////////////////////////////////
		//messages sending by the server only
		//////////////////////////////////////////////////////////////////////////
		class MStatus : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & status;
			}
			ClientStatus status;
			RPLANES_MESSAGE_ID(14);
		};
		RPLANES_REGISTER_MESSAGE(MStatus);



		class MRoomList : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & rooms;
			}
			class RoomInfo
			{
			public:
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					ar & mapName;
					ar & slots;
					ar & creatorName;
					ar & description;
				}

				std::string creatorName;
				std::string mapName;
				std::string description;

				class SlotInfo
				{
				public:
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version)
					{
						ar & nPlayers;
						ar & nPlayersMax;
					}
					size_t nPlayers = 0,
						nPlayersMax = 0;
				};
				std::map< Nation, SlotInfo > slots;

			};
			std::vector<RoomInfo> rooms;
			RPLANES_MESSAGE_ID(15);
		};
		RPLANES_REGISTER_MESSAGE(MRoomList);

		class MProfile : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & profile;
			}
			rplanes::playerdata::Profile profile;
			MProfile(const rplanes::playerdata::Profile & p) : profile(p) {};
			MProfile() = default;
			RPLANES_MESSAGE_ID(16);
		};
		RPLANES_REGISTER_MESSAGE(MProfile);

		class MServerConfiguration : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & conf;
			}
			Configuration conf;
			MServerConfiguration(const Configuration & configuration) : conf(configuration) {};
			MServerConfiguration() = default;
			RPLANES_MESSAGE_ID(17);
		};
		RPLANES_REGISTER_MESSAGE(MServerConfiguration);

		//////////////////////////////////////////////////////////////////////////
		//messages sending to both directions
		//////////////////////////////////////////////////////////////////////////
		class MText : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & text;
			}

			std::string text;
			RPLANES_MESSAGE_ID(18);
		};
		RPLANES_REGISTER_MESSAGE(MText);

		class MExitRoom : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{

			}

			RPLANES_MESSAGE_ID(19);
		};
		RPLANES_REGISTER_MESSAGE(MExitRoom);

		class MResourceString : public Message
		{
		public:
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & string;
			}
			MResourceString(const rstring::_rstrw_t &str) : string(str) {};
			MResourceString() = default;
			rstring::_rstrw_t string;
			RPLANES_MESSAGE_ID(20);
		};
		RPLANES_REGISTER_MESSAGE(MResourceString);
	}
}
