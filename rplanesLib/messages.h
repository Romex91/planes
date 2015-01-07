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
////////////////////////Макрос, для размещения в теле сообщений///////////
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
////Макрос, для регистрации сообщения. Размещается после класса///////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#define MESSAGE_REG(className)\
	namespace objects\
{\
	static MessageRegistrar<className> className##Registration;\
};


#endif // MACROSES

#ifdef PLANES_CLIENT
#define MESSAGE_HANDLER void handle(){ std::cout << "Получено недопустимое сообщение"<<std::endl; };
#else
#define MESSAGE_HANDLER void handle();
#endif 

namespace rplanes
{
	namespace network
	{

		//сообщения, передаваемые только клиентом
		namespace clientmessages
		{

			//отправить запрос на получение статуса клиента
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

			//сообщения отправляемые до авторизации
			namespace unlogined
			{
				//вход в игру.
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

				//регистрация нового игрока
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

			//сообщения отправляемые только из ангара
			namespace hangar
			{

				//запрос на отправку списка имеющихся комнат
				class RoomListRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
					MESSAGE_BODY(RoomListRequest, 3)
				};
				MESSAGE_REG(RoomListRequest);

				//запрос на создание комнаты
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

				//запрос на удаление комнаты
				class DestroyRoomRequest : public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					MESSAGE_BODY(DestroyRoomRequest, 5)
				};
				MESSAGE_REG(DestroyRoomRequest);


				//запрос на присоединение к комнате
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

				//запрос данных игрока
				class ProfileRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:			
					MESSAGE_BODY(ProfileRequest, 7)
				};
				MESSAGE_REG(ProfileRequest);

				//запрос данных другого игрока. Для возможности фаллометрии
				class PlayerProfileRequest : public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string playerName;
					MESSAGE_BODY(PlayerProfileRequest, 8)
				};
				MESSAGE_REG(PlayerProfileRequest);

				//Запрос покупки самолета.
				//в ответ сервер отошлет textMessage со статусом выполнения
				class BuyPlaneRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string planeName;			
					MESSAGE_BODY(BuyPlaneRequest, 9)
				};
				MESSAGE_REG(BuyPlaneRequest);

				//Запрос установки модуля. Если модуль отсутствует на складе, он будет куплен.
				class BuyModuleRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					//самолет, на который устанавливается модуль
					std::string planeName;
					//название модуля
					std::string moduleName;
					//если true, модуль будет установлен везде где можно, иначе только в moduleNo
					bool setToAllSlots;
					//номер заменяемого модуля.
					size_t moduleNo;
					//тип модуля
					rplanes::ModuleType moduleType;
					MESSAGE_BODY(BuyModuleRequest, 10);
				};
				MESSAGE_REG(BuyModuleRequest);

				//Запрос продажи самолета.
				//в ответ сервер отошлет textMessage со статусом выполнения
				class SellPlaneRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string planeName;			
					MESSAGE_BODY(SellPlaneRequest, 11);
				};
				MESSAGE_REG(SellPlaneRequest);

				//Запрос продажи модуля со склада.
				//в ответ сервер отошлет textMessage со статусом выполнения и измененный профиль sendProfile					
				class SellModuleRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string moduleName;
					//количество продаваемых модулей
					size_t nModulesToSell;
					MESSAGE_BODY(SellModuleRequest, 12)
				};
				MESSAGE_REG(SellModuleRequest);

				//запрос повышения навыка
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

			//сообщения отправляемые только из комнаты
			namespace room
			{
				class ServerTimeRequest: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version){}
					MESSAGE_BODY(ServerTimeRequest, 14);
				};
				MESSAGE_REG(ServerTimeRequest);

				//отправить серверу данные управления
				class SendControllable: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					serverdata::Plane::ControllableParameters params;
					MESSAGE_BODY(SendControllable, 15);
				};
				MESSAGE_REG(SendControllable);

				//администрирование комнаты. Необходимо быть ее владельцем
				class AdministerRoom : public Message
				{
				private:
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					enum Operation
					{
						//в опциях имена выбрасываемых игроков
						KICK_PLAYERS,
						//в опциях имена игроков, добавляемых в черный список
						BAN_PLAYERS,
						//в опциях название карты
						CHANGE_MAP,
						//
						RESTART,
						//в опциях имена игроков
						KILL_PLAYERS,
						//в опциях имена игроков
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
#define MESSAGE_HANDLER void handle(){ std::cout << "Получено недопустимое сообщение"<<std::endl; };
#else
#define MESSAGE_HANDLER void handle();
#endif 

namespace rplanes
{
	namespace network
	{
		enum  ClientStatus
		{
			//клиент не авторизован. Будет отключен если не авторизуется
			UNLOGINED,
			//клиент авторизован.
			HANGAR,
			//клиент в комнате.
			ROOM		
		};

		//сообщения, передаваемые только сервером
		namespace servermessages
		{

			//статус клиента
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
				//сообщения создания пуль
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

				//данные  отправляемые сервером для визуализации
				class InterfaceData: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					
					unsigned short
						//- уровень затемнения экрана при перегрузках [0,100]
						faintVal,
						//- радиус прицельной рамки. В метрах
						aimSize,
						//- положение прицельной рамки относительно пушек (не самолета). В метрах
						shootingDistance,
						//- максимальная скорость м/с
						Vmax,
						//- минимальная скорость м/с
						Vmin,
						//- скорость м/с
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
							//температура при которой двигатель повреждается. 0.1 градус цельсия
							criticalTemperature,
							//- температура двигателя. 0.1 градус цельсия
							temperature,
							//- прирост температуры двигателя при котором двигатель повреждается. 0.1 градус цельсия
							dTmax,
							//- прирост температуры. 0.1 градус цельсия
							dT;						
					};
					//показания приборов для каждого из двигателей
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
							//калибр. 0,01 мм. К примеру, 7.62 будет иметь значение 762
							caliber,
							//боезапас. штука
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
							//запас в литрах
							fuel, 
							//емкость в литрах
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


					//серверный метод
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
							ammo.caliber = static_cast<unsigned short>(serverAmmo.caliber);
							ammo.capacity = static_cast<unsigned short>(serverAmmo.capacity);
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


				//сообщение логически идентичное CreateBullets 
				class CreateRicochetes: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:			
					std::vector <serverdata::Bullet> bullets;
					float time;
					MESSAGE_BODY(CreateRicochetes, 20);
				};
				MESSAGE_REG(CreateRicochetes);
				
				//сообщение создания ракет
				class CreateMissiles: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::vector<serverdata::LaunchedMissile> missiles;
					float time;
					MESSAGE_BODY(CreateMissiles, 21);
				};
				MESSAGE_REG(CreateMissiles);
				
				//сообщение уничтожения пуль. Передается только при попаданиях. В случае излета клиент должен сам удалить пулю
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

				//сообщения уничтожения ракет
				class DestroyMissiles: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::vector<size_t> ids;
					MESSAGE_BODY(DestroyMissiles, 23)
				};
				MESSAGE_REG(DestroyMissiles);
				
				//уведомление о смене карты
				class ChangeMap : public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::string mapName;
					MESSAGE_BODY(ChangeMap, 24);
				};
				MESSAGE_REG(ChangeMap);


				//данные самолета передаваемые клиенту
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
					//запускается на сервере
					void init( const serverdata::Plane & Plane , std::string PlayerName, size_t ID );
					//запускается на клиенте
					void extrapolate( float frameTime );
				};

				//создание нового самолета. Необязательно спаун нового игрока. Самолеты появляются и при вхождении в область видимости
				class CreatePlanes : public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					std::vector< Plane > Planes;
					MESSAGE_BODY(CreatePlanes, 25);
				};
				MESSAGE_REG(CreatePlanes);

				//уничтожение самолета
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
						//номер модуля в векторе
						size_t moduleNo;
						size_t planeID;
						size_t killerId;
						Reason reason;
						//На нужды ботов
						rplanes::Nation nation;
					};
					std::vector<DestroyedPlane> planes;
					MESSAGE_BODY(DestroyPlanes, 26);
				};
				MESSAGE_REG(DestroyPlanes);
						
				//позиции всех отображаемых самолетов					
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

				//обновить состояние модуля
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

				//передать текущее время сервера. Необходимо для избавления от дрожжания
				class ServerTime: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					float time;
					MESSAGE_BODY(ServerTime, 29);
				};
				MESSAGE_REG(ServerTime);

				//передача информации о игроках в текущей комнате. Сообщение отправляется после обновления данных.
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

					//игроки зашедшие в комнату
					std::vector<NewPlayerInfo> newPlayers;
					
					//игроки, вышедшие из комнаты
					std::vector<std::string> disconnectedPlayers;
					
					//ключ - имя игрока, статистика которого изменилась
					std::map< std::string, Statistics > updatedStatistics;
					
					//куда лететь и что делать
					Goal goal;

					MESSAGE_BODY(RoomInfo, 30);
				};
				MESSAGE_REG(RoomInfo);
			}

			namespace hangar
			{
				//список доступных комнат
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

				//отправить данные ирока. profile
				class SendProfile: public Message
				{
					template <typename Archive>
					void serialize(Archive& ar, const unsigned int version);
				public:
					rplanes::playerdata::Profile profile;
					MESSAGE_BODY(SendProfile, 32);
				};
				MESSAGE_REG(SendProfile);

				//конфигурация сервера. Необходима для выполнения синохронных с сервером действий
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
		//сообщения, передаваемые и клиентом и сервером 
		namespace bidirectionalmessages
		{
			//отправить текстовое сообщение. text
			class TextMessage: public Message 
			{
				template <typename Archive>
				void serialize(Archive& ar, const unsigned int version);
			public:
				std::string text;
				MESSAGE_BODY( TextMessage, 34 );
			};
			MESSAGE_REG(TextMessage);

			//сообщение о выходе из комнаты
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
