#pragma once
#include "stdafx.h"

#include "pilot.h"
#include "plane.h"


namespace rplanes
{
	namespace playerdata
	{
		//���� odb. (odb �� ������������ ��������� �������)
		namespace profile_plane
		{
#pragma db value
			struct id
			{
				std::string planeName;
				std::string profileName;
			};
		}
		//������ ��������. �������� �������� ���� �������, ������������� �� ������ �������� �������
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
			Statistics() :
				friendlyDamage(0),
				damage(0),
				damageReceived(0),
				friensDestroyed(0),
				enemyDestroyed(0),
				shots(0),
				hits(0),
				hitsReceived(0),
				crashes(0),
				exp(0),
				money(0)
			{}
			int
				friendlyDamage,
				damage,
				damageReceived,

				friensDestroyed,
				enemyDestroyed,

				shots,

				hits,
				hitsReceived,

				crashes,

				exp,
				money;
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
		//������� ������. �������� ������ ������, ������ �������� �������, ������������ �� ������. � ������������ ��������� ���������.
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

			//����������. ����� � �� ���������.  ���� - �������� �������� ��� total ��� �����
			std::map< std::string, Statistics> statistics;

			Profile();

			void loadPlanes(std::shared_ptr< odb::database > profilesDB);

			//���������� ������� ������� ��������
			void save(std::shared_ptr< odb::database > profilesDB);

			//���������� ������ ���� "������� ������" � �.�.
			std::string buyPlane(
				std::string planeName, std::shared_ptr<odb::database> planesDB);

			// ������� � ��������� ������ � ������� moduleNo. ���������� ������ ���� "������ ������" � �.�. 
			//���� ������ ���� �� ������,  ������ ��������� �� �����. ���� moduleNo �� ���������, ����� ���������� ��������� ������.
			std::string buyModule(
				std::string planeName,
				size_t moduleNo,
				std::string moduleName,
				std::shared_ptr<odb::database > planesDB);

			//������� � ��������� ������� �� ��� ���������� �����
			std::string buyModules(
				std::string planeName,
				std::string moduleName,
				std::shared_ptr<odb::database > planesDB
				);

			// ���� ���� < 0, ������ ������� ��� ����, � abs(����) ������������� ���� ��������
			std::vector<std::pair<std::string, int> > planePriceList(std::shared_ptr<odb::database> planesDB);

			//������ ������� ����������� ����, ��������������� �� ������� � ���������� �������. 
			//���� ������ ���� �� ������, ���� ����� ����� 0. ���� pos �� ��������, ����� ��������� ������ ������.
			std::vector<std::pair<std::string, int> > modulePriceList(
				std::string  planeName,
				ModuleType moduleType,
				size_t moduleNo,
				std::shared_ptr<odb::database> planesDB);
			// ������ �������, ������������ �� ������.
			std::vector<std::pair<std::string, int> > moduleStorePriceList(std::shared_ptr<odb::database> planesDB);
			// ���������� ������ ���� "������� ������"
			std::string sellPlane(
				std::string planeName,
				std::shared_ptr<odb::database> planesDB);
			// ��������� ����� ������ ������, ���������� �� ������. ���������� ������ ���� "������ ������"
			std::string sellModule(
				std::string moduleName,
				size_t nModules,
				std::shared_ptr<odb::database> planesDB);
		};
	}
}

