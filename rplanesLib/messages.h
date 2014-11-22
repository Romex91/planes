#pragma once
#define MACROSES
#include "network.h"
#include "plane.h"
#include "profile.h"
#include "configuration.h"

#if defined(PLANES_CLIENT) && defined( PLANES_SERVER )
#error Both PLANES_CLIENT and PLANES_SERVER was defined
#endif

namespace boost {
	namespace serialization {
		template<class Archive>
		void serialize(Archive & ar, rplanes::playerdata::Statistics & Stat, const unsigned int version);
		template<class Archive>
		void serialize(Archive & ar, rplanes::playerdata::Plane & pt, const unsigned int version);
		template<class Archive>
		void serialize(Archive & ar, rplanes::playerdata::Profile & Profile, const unsigned int version);
		template<class Archive>
		void serialize(Archive & ar, rplanes::serverdata::LaunchedMissile & Missile, const unsigned int version);
		template<class Archive>
		void serialize(Archive & ar, rplanes::serverdata::Bullet & Bullet, const unsigned int version);

	} // namespace serialization
} // namespace boost

#ifdef MACROSES
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////////////////////������, ��� ���������� � ���� ���������///////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define MESSAGE_BODY(className, classId)\
private:\
	void writeData(boost::archive::binary_oarchive & archive)\
	{\
		archive << *this;\
	}\
	void readData(boost::archive::binary_iarchive & archive)\
	{\
		archive >> *this;\
	}\
	friend boost::serialization::access;\
	std::shared_ptr<Message> copy()\
	{\
		auto ptr = new className;\
		*ptr = *this;\
		return std::shared_ptr<className>(ptr);\
	}\
public:\
	unsigned short getId()const\
	{\
		return classId; \
	}\
	MESSAGE_HANDLER
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////������, ��� ����������� ���������. ����������� ����� ������///////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#define MESSAGE_REG(className)\
	namespace objects\
{\
	static MessageRegistrar<className> className##Registration;\
};


#endif // MACROSES

#ifdef PLANES_CLIENT
#define MESSAGE_HANDLER void handle(){ std::cout << "�������� ������������ ���������"<<std::endl; };
#else
#define MESSAGE_HANDLER void handle();
#endif 

namespace rplanes
{
	namespace network
	{

		//���������, ������������ ������ ��������
		namespace clientmessages
		{

			//��������� ������ �� ��������� ������� �������
			class StatusRequest: public Message
			{
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
				}
			public:
				MESSAGE_BODY(StatusRequest,0);
			};
			MESSAGE_REG(StatusRequest);

			//��������� ������������ �� �����������
			namespace unlogined
			{
				//���� � ����.
				class Login: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string name;
					std::string encryptedPassword;
					MESSAGE_BODY(Login,1)			
				};
				MESSAGE_REG(Login);

				//����������� ������ ������
				class Registry: public Message
				{
				private:
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string name, password;			
					MESSAGE_BODY(Registry,2)
				};
				MESSAGE_REG(Registry);
			}

			//��������� ������������ ������ �� ������
			namespace hangar
			{

				//������ �� �������� ������ ��������� ������
				class RoomListRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
					MESSAGE_BODY(RoomListRequest, 3)
				};
				MESSAGE_REG(RoomListRequest);

				//������ �� �������� �������
				class CreateRoomRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string description;
					std::string mapName;
					MESSAGE_BODY(CreateRoomRequest, 4)
				};
				MESSAGE_REG(CreateRoomRequest);

				//������ �� �������� �������
				class DestroyRoomRequest : public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					MESSAGE_BODY(DestroyRoomRequest, 5)
				};
				MESSAGE_REG(DestroyRoomRequest);


				//������ �� ������������� � �������
				class JoinRoomRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					size_t planeNo;
					std::string playerName;
					MESSAGE_BODY(JoinRoomRequest, 6)
				};
				MESSAGE_REG(JoinRoomRequest);

				//������ ������ ������
				class ProfileRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:			
					MESSAGE_BODY(ProfileRequest, 7)
				};
				MESSAGE_REG(ProfileRequest);

				//������ ������ ������� ������. ��� ����������� �����������
				class PlayerProfileRequest : public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string playerName;
					MESSAGE_BODY(PlayerProfileRequest, 8)
				};
				MESSAGE_REG(PlayerProfileRequest);

				//������ ������� ��������.
				//� ����� ������ ������� textMessage �� �������� ����������
				class BuyPlaneRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string planeName;			
					MESSAGE_BODY(BuyPlaneRequest, 9)
				};
				MESSAGE_REG(BuyPlaneRequest);

				//������ ��������� ������. ���� ������ ����������� �� ������, �� ����� ������.
				class BuyModuleRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					//�������, �� ������� ��������������� ������
					std::string planeName;
					//�������� ������
					std::string moduleName;
					//���� true, ������ ����� ���������� ����� ��� �����, ����� ������ � moduleNo
					bool setToAllSlots;
					//����� ����������� ������.
					size_t moduleNo;
					//��� ������
					rplanes::ModuleType moduleType;
					MESSAGE_BODY(BuyModuleRequest, 10);
				};
				MESSAGE_REG(BuyModuleRequest);

				//������ ������� ��������.
				//� ����� ������ ������� textMessage �� �������� ����������
				class SellPlaneRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string planeName;			
					MESSAGE_BODY(SellPlaneRequest, 11);
				};
				MESSAGE_REG(SellPlaneRequest);

				//������ ������� ������ �� ������.
				//� ����� ������ ������� textMessage �� �������� ���������� � ���������� ������� sendProfile					
				class SellModuleRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string moduleName;
					//���������� ����������� �������
					size_t nModulesToSell;
					MESSAGE_BODY(SellModuleRequest, 12)
				};
				MESSAGE_REG(SellModuleRequest);

				//������ ��������� ������
				class UpSkillRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					enum Skill{FLIGHT, ENDURANCE, SHOOTING, ENGINE } skill;
					size_t experienceToSpend;
					MESSAGE_BODY(UpSkillRequest, 13)
				};
				MESSAGE_REG(UpSkillRequest);
			}

			//��������� ������������ ������ �� �������
			namespace room
			{
				class ServerTimeRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version){}
					MESSAGE_BODY(ServerTimeRequest, 14);
				};
				MESSAGE_REG(ServerTimeRequest);

				//��������� ������� ������ ����������
				class SendControllable: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					serverdata::Plane::ControllableParameters params;
					MESSAGE_BODY(SendControllable, 15);
				};
				MESSAGE_REG(SendControllable);

				//����������������� �������. ���������� ���� �� ����������
				class AdministerRoom : public Message
				{
				private:
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					enum Operation
					{
						//� ������ ����� ������������� �������
						KICK_PLAYERS,
						//� ������ ����� �������, ����������� � ������ ������
						BAN_PLAYERS,
						//� ������ �������� �����
						CHANGE_MAP,
						//
						RESTART,
						//� ������ ����� �������
						KILL_PLAYERS,
						//� ������ ����� �������
						UNBAN_PLAYERS
					};
					Operation operation;
					std::vector<std::string> options;
					MESSAGE_BODY(AdministerRoom, 16);
				};
				MESSAGE_REG(AdministerRoom);
			}
		}
	}
}

#undef  MESSAGE_HANDLER
#ifdef PLANES_SERVER
#define MESSAGE_HANDLER void handle(){ std::cout << "�������� ������������ ���������"<<std::endl; };
#else
#define MESSAGE_HANDLER void handle();
#endif 

namespace rplanes
{
	namespace network
	{
		enum  ClientStatus
		{
			//������ �� �����������. ����� �������� ���� �� ������������
			UNLOGINED,
			//������ �����������.
			HANGAR,
			//������ � �������.
			ROOM		
		};

		//���������, ������������ ������ ��������
		namespace servermessages
		{

			//������ �������
			class StatusMessage: public Message
			{
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version);
			public:			
				ClientStatus status;
				MESSAGE_BODY(StatusMessage, 17);
			};
			MESSAGE_REG(StatusMessage);

			namespace room
			{
				//��������� �������� ����
				class CreateBullets: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					
					std::vector <serverdata::Bullet> bullets;
					float time;
					MESSAGE_BODY(CreateBullets, 18);
				};
				MESSAGE_REG(CreateBullets);

				//������  ������������ �������� ��� ������������
				class InterfaceData: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					
					unsigned short
						//- ������� ���������� ������ ��� ����������� [0,100]
						faintVal,
						//- ������ ���������� �����. � ������
						aimSize,
						//- ��������� ���������� ����� ������������ ����� (�� ��������). � ������
						shootingDistance,
						//- ������������ �������� �/�
						Vmax,
						//- ����������� �������� �/�
						Vmin,
						//- �������� �/�
						V;
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
							//����������� ��� ������� ��������� ������������. 0.1 ������ �������
							criticalTemperature,
							//- ����������� ���������. 0.1 ������ �������
							temperature,
							//- ������� ����������� ��������� ��� ������� ��������� ������������. 0.1 ������ �������
							dTmax,
							//- ������� �����������. 0.1 ������ �������
							dT;						
					};
					//��������� �������� ��� ������� �� ����������
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
							//������. 0,01 ��. � �������, 7.62 ����� ����� �������� 762
							caliber,
							//��������. �����
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
							//����� � ������
							fuel, 
							//������� � ������
							capacity;
					} gasTank;

					InterfaceData()
					{
						faintVal = 0;
						aimSize = 0;
						shootingDistance = 0;
						Vmax  = 0;
						Vmin = 0;
						V = 0;
					}


					//��������� �����
					void update( const serverdata::Plane & Plane )
					{
						faintVal = Plane.target.faintVal * 100;
						aimSize = Plane.target.aimSize;
						shootingDistance = Plane.target.clientShootingDistance;
						Vmax = Plane.statical.Vmax;
						Vmin = Plane.statical.Vmin;
						V = Plane.target.V;

						ammunitions.clear();
						for(auto &serverAmmo : Plane.ammunitions )
						{
							Ammo ammo;
							ammo.caliber = serverAmmo.caliber;
							ammo.capacity = serverAmmo.capacity;
							ammunitions.push_back(ammo);
						}
						thermometers.clear();
						for ( auto & engine : Plane.engines )
						{
							Thermometer newTherm;
							newTherm.criticalTemperature =  engine.criticalTemperature * 10;
							newTherm.dT = engine.dt * 10;
							newTherm.dTmax = engine.maxDt * 10;
							newTherm.temperature = engine.temperature * 10;
							thermometers.push_back(newTherm);
						}
						gasTank.fuel = 0;
						gasTank.capacity = 0;
						for (  auto & Tank : Plane.tanks)
						{
							gasTank.fuel+=Tank.fuel;
							gasTank.capacity+=Tank.capacity;
						}
					}

					MESSAGE_BODY(InterfaceData, 19);	
				};
				MESSAGE_REG(InterfaceData);


				//��������� ��������� ���������� CreateBullets
				class �reateRicochetes: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:			
					std::vector <serverdata::Bullet> bullets;
					float time;
					MESSAGE_BODY(�reateRicochetes, 20);
				};
				MESSAGE_REG(�reateRicochetes);
				
				//��������� �������� �����
				class �reateMissiles: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::vector<serverdata::LaunchedMissile> missiles;
					float time;
					MESSAGE_BODY(�reateMissiles, 21);
				};
				MESSAGE_REG(�reateMissiles);
				
				//��������� ����������� ����. ���������� ������ ��� ����������. � ������ ������ ������ ������ ��� ������� ����
				class DestroyBullets: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					class BulletInfo
					{
					public:
						template <typename Archive>
						void serialize(Archive& ar, const unsigned int version);
						
						size_t bulletID;
						enum Reason
						{
							HIT,
							RICOCHET
						} reason;
					};
					std::vector<BulletInfo> bullets;
					MESSAGE_BODY(DestroyBullets, 22)
				};
				MESSAGE_REG(DestroyBullets);

				//��������� ����������� �����
				class DestroyMissiles: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::vector<size_t> ids;
					MESSAGE_BODY(DestroyMissiles, 23)
				};
				MESSAGE_REG(DestroyMissiles);
				
				//����������� � ����� �����
				class ChangeMap : public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string mapName;
					MESSAGE_BODY(ChangeMap, 24);
				};
				MESSAGE_REG(ChangeMap);


				//������ �������� ������������ �������
				class Plane
				{
				public:
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);

					std::string planeName,
						playerName;
					Nation nation;
					size_t id;

					class Position
					{
					public:
						template <typename Archive>
						void serialize(Archive& ar, const unsigned int version);
						float x,
							y,
							angle,
							roll;
					} pos;

					class  ExtrapolationData
					{
					public:
						template <typename Archive>
						void serialize(Archive& ar, const unsigned int version);
						float speed,
							angleVelocity,
							acceleration;
					} extrapolationData;

					class Module
					{
					public:
						template <typename Archive>
						void serialize(Archive& ar, const unsigned int version);
						unsigned short hp, hpMax;
						serverdata::HitZone hitZone;
						bool defected;
						ModuleType type;
					};
					std::vector<Module> modules;

					Plane(){}
					//����������� �� �������
					void init( const serverdata::Plane & Plane , std::string PlayerName, size_t ID );
					//����������� �� �������
					void extrapolate( float frameTime );
				};

				//�������� ������ ��������. ������������� ����� ������ ������. �������� ���������� � ��� ��������� � ������� ���������
				class CreatePlanes : public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::vector< Plane > Planes;
					MESSAGE_BODY(CreatePlanes, 25);
				};
				MESSAGE_REG(CreatePlanes);

				//����������� ��������
				class DestroyPlanes : public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
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
						void serialize(Archive& ar, const unsigned int version);
						//����� ������ � �������
						size_t moduleNo;
						size_t planeID;
						size_t killerId;
						Reason reason;
						//�� ����� �����
						rplanes::Nation nation;
					};
					std::vector<DestroyedPlane> planes;
					MESSAGE_BODY(DestroyPlanes, 26);
				};
				MESSAGE_REG(DestroyPlanes);
						
				//������� ���� ������������ ���������					
				class SetPlanesPositions: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					class PlanePos
					{
					public:
						template <typename Archive>
						void serialize(Archive& ar, const unsigned int version);
						size_t planeID;
						Plane::Position pos;
						Plane::ExtrapolationData extrapolationData;
					};
					std::vector< PlanePos> positions;
					float time;
					MESSAGE_BODY(SetPlanesPositions, 27);
				};
				MESSAGE_REG(SetPlanesPositions);

				//�������� ��������� ������
				class UpdateModules: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					class Module
					{
					public:
						template <typename Archive>
						void serialize(Archive& ar, const unsigned int version);
						size_t planeID, moduleNo;
						unsigned short hp;
						bool defect;
						planedata::ModuleHP::ChangeReason reason;
					};
					std::vector< Module > modules;
					MESSAGE_BODY(UpdateModules, 28);
				};
				MESSAGE_REG(UpdateModules);

				//�������� ������� ����� �������. ���������� ��� ���������� �� ���������
				class ServerTime: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					float time;
					MESSAGE_BODY(ServerTime, 29);
				};
				MESSAGE_REG(ServerTime);

				//�������� ���������� � ������� � ������� �������. ��������� ������������ ����� ���������� ������.
				class RoomInfo: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					
					class NewPlayerInfo
					{
					public:
						template <typename Archive>
						void serialize(Archive& ar, const unsigned int version);
						std::string name;
						std::string planeName;
						Nation nation;
					};
					class Goal
					{
					public:
						template <typename Archive>
						void serialize(Archive& ar, const unsigned int version);
						rplanes::PointXY position;
						std::string description;
						short recommendedSpeed;
					};
					class Statistics
					{
					public:
						template <typename Archive>
						void serialize(Archive& ar, const unsigned int version);
						int destroyed,
							friendsDestroyed,
							crashes;
					};

					void clear();

					//������ �������� � �������
					std::vector<NewPlayerInfo> newPlayers;
					
					//������, �������� �� �������
					std::vector<std::string> disconnectedPlayers;
					
					//���� - ��� ������, ���������� �������� ����������
					std::map< std::string, Statistics > updatedStatistics;
					
					//���� ������ � ��� ������
					Goal goal;

					MESSAGE_BODY(RoomInfo, 30);
				};
				MESSAGE_REG(RoomInfo);
			}

			namespace hangar
			{
				//������ ��������� ������
				class RoomList: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					class RoomInfo
					{
					public:
						template <typename Archive>
						void serialize(Archive& ar, const unsigned int version);
						std::string creatorName;
						std::string mapName;
						std::string description;

						class SlotInfo
						{
						public:
							SlotInfo();
							template <typename Archive>
							void serialize(Archive& ar, const unsigned int version);
							size_t nPlayers,
								nPlayersMax;
						};
						std::map< Nation, SlotInfo > slots;

					};
					std::vector<RoomInfo> rooms;			
					MESSAGE_BODY(RoomList, 31);
				};
				MESSAGE_REG(RoomList);

				//��������� ������ �����. profile
				class SendProfile: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					rplanes::playerdata::Profile profile;
					MESSAGE_BODY(SendProfile, 32);
				};
				MESSAGE_REG(SendProfile);

				//������������ �������. ���������� ��� ���������� ����������� � �������� ��������
				class ServerConfiguration: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					Configuration conf;
					MESSAGE_BODY(ServerConfiguration, 33);
				};
				MESSAGE_REG(ServerConfiguration);
			}
		}
	}
}

#undef MESSAGE_HANDLER
#define MESSAGE_HANDLER void handle();

namespace rplanes
{
	namespace network
	{
		//���������, ������������ � �������� � �������� 
		namespace bidirectionalmessages
		{
			//��������� ��������� ���������. text
			class TextMessage: public Message 
			{
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version);
			public:
				std::string text;
				MESSAGE_BODY( TextMessage, 34 );
			};
			MESSAGE_REG(TextMessage);

			//��������� � ������ �� �������
			class ExitRoom: public Message
			{
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version);
				MESSAGE_BODY(ExitRoom, 35);
			};
			MESSAGE_REG(ExitRoom);
		}
	}
}

#undef MACROSES
#undef MESSAGE_BODY
#undef MESSAGE_REG
#undef MESSAGE_HANDLER
