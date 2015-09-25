#pragma once
#include "stdafx.h"

#include "pilot.h"
#include "plane.h"


namespace rplanes
{
	namespace playerdata
	{
		namespace profile_plane
		{
#pragma db value
			struct id
			{
				std::string planeName;
				std::string profileName;
			};
		}
		//шаблон самолета. Содержит названия всех модулей, установленных на данном самолете профиля
#pragma  db object
		class Plane
		{
		public:
#pragma db\
	id_column("id")\
	value_column("gun")
			std::vector< std::string > guns;
#pragma db\
	id_column("id")\
	value_column("missile")
			std::vector< std::string > missiles;

#pragma  db id
			profile_plane::id id;
			std::string tail;
#pragma db\
	id_column("id")\
	value_column("ammunition")
			std::vector<std::string> ammunitions;

#pragma db\
	id_column("id")\
	value_column("tank")
			std::vector<std::string> tanks;

#pragma db\
	id_column("id")\
	value_column("engine")
			std::vector<std::string> engines;

#pragma db\
	id_column("id")\
	value_column("wing")
			std::vector<std::string> wings;

#pragma db\
	id_column("id")\
	value_column("wing")
			std::vector<std::string> turrets;


			std::string framework;
			std::string cabine;

			void save(std::shared_ptr< odb::database > profilesDB);

			serverdata::Plane buildPlane(Pilot Pilot, std::shared_ptr<odb::database>  planesDB);

			bool isReadyForJoinRoom();

			int getPrice(std::shared_ptr< odb::database > planesDB);

			Nation nation;
		};

#pragma db value
		struct  Statistics
		{
			//Statistics() :
			//	friendlyDamage(0),
			//	damage(0),
			//	damageReceived(0),
			//	friensDestroyed(0),
			//	enemyDestroyed(0),
			//	shots(0),
			//	hits(0),
			//	hitsReceived(0),
			//	crashes(0),
			//	exp(0),
			//	money(0)
			//{}
			int
				friendlyDamage = 0,
				damage = 0,
				damageReceived = 0,

				friensDestroyed = 0,
				enemyDestroyed = 0,

				shots = 0,

				hits = 0,
				hitsReceived = 0,

				crashes = 0,

				exp = 0,
				money = 0;
			Statistics & operator+=(const Statistics & statistics)
			{
				friendlyDamage += statistics.friendlyDamage;
				damage += statistics.damage;
				damageReceived += statistics.damageReceived;
				friensDestroyed += statistics.friensDestroyed;
				enemyDestroyed += statistics.enemyDestroyed;
				shots += statistics.shots;
				hits += statistics.hits;
				hitsReceived += statistics.hitsReceived;
				crashes += statistics.crashes;
				exp += statistics.exp;
				money += statistics.money;
				return *this;
			}
		};
		//профиль игрока. Содержит данные пилота, список названий модулей, содержащихся на складе. И конфигурации имеющихся самолетов.
#pragma db object
		class Profile
		{
		public:
			Pilot pilot;

#pragma db id
			std::string login;
			std::string password;

#pragma db unordered\
	id_column("profile")\
	value_column("module")
			std::vector< std::string > moduleStore;

#pragma db transient
			std::vector< Plane > planes;
			int money;

#pragma db id_column("id")\
	value_column("openedPlanes")
			std::set<std::string> openedPlanes;

#pragma db id_column("id")\
	value_column("openedMaps")
			std::set<std::string> openedMaps;

#pragma db id_column("id")\
	value_column("banlist")
			std::set < std::string > banlist;

			//статистика. общая и по самолетам.  ключ - название самолета или total для общей
			std::map< std::string, Statistics> statistics;

			Profile();

			void loadPlanes(std::shared_ptr< odb::database > profilesDB);

			//сохранение профиля включая самолеты
			void save(std::shared_ptr< odb::database > profilesDB);

			//возвращает строки типа "самолет куплен" и т.п.
			PlanesString buyPlane(
				std::string planeName, std::shared_ptr<odb::database> planesDB);

			// Покупка и установка модуля в позицию moduleNo. возвращает строки типа "модуль куплен" и т.п. 
			//если модуль есть на складе,  деньги потрачены не будут. Если moduleNo не корректно, будет возвращено сообщение ошибке.
			PlanesString buyModule(
				std::string planeName,
				size_t moduleNo,
				std::string moduleName,
				std::shared_ptr<odb::database > planesDB);

			//покупка и установка модулей во все подходящие слоты
			PlanesString buyModules(
				std::string planeName,
				std::string moduleName,
				std::shared_ptr<odb::database > planesDB
				);

			// если цена < 0, значит самолет уже есть, а abs(цена) соответствует цене самолета
			std::vector<std::pair<std::string, int> > planePriceList(std::shared_ptr<odb::database> planesDB);

			//список модулей конкретного типа, устанавливаемых на самолет в конкретную позицию. 
			//Если модуль есть на складе, цена будет равна 0. Если pos не коректно, будет возвращен пустой массив.
			std::vector<std::pair<std::string, int> > modulePriceList(
				std::string  planeName,
				ModuleType moduleType,
				size_t moduleNo,
				std::shared_ptr<odb::database> planesDB);
			// Список модулей, содержащихся на складе.
			std::vector<std::pair<std::string, int> > moduleStorePriceList(std::shared_ptr<odb::database> planesDB);
			// возвращает строки типа "самолет продан"
			PlanesString sellPlane(
				std::string planeName,
				std::shared_ptr<odb::database> planesDB);
			// Продавать можно только модули, хранящиеся на складе. Возвращает строки типа "модуль продан"
			PlanesString sellModule(
				std::string moduleName,
				size_t nModules,
				std::shared_ptr<odb::database> planesDB);

		private:
			enum class BuyModuleResult {MOUNTED, MOUNTED_FROM_STORE, NOT_ENOUGH_MONEY, ALLREADY_MOUNTED, NOT_FOUND, NOT_SUITABLE};
			BuyModuleResult buyModuleInternal(
				std::string planeName,
				size_t moduleNo,
				std::string moduleName,
				std::shared_ptr<odb::database > planesDB);

		};
	}
}

