#pragma once
//������ ���� �������� ����� ������ ��������
#include "stdafx.h"
#include "modules.h"
#include "profile.h"

namespace rplanes
{

	namespace planedata
	{
		//������ ��������. �������� ������ ��������������� ������� � ���������� ������� ������ �����
#pragma  db  object
		class Model
		{
		public:
			//��������� �� ���� ������ �������� ������������
			playerdata::Plane loadBasicConfiguration( std::shared_ptr<odb::database> planesDB, int & price, std::string profileName );

			//�������� ������ ��������
#pragma db id
			std::string planeName;


			//������ �������� ��������������� �������
#pragma db unordered\
	id_column("planeName")\
	value_column("module")
			std::vector<std::string> modules;


			//������ ����� ������. ���������� ���������� ��������������� ��������� � �����
#pragma db unordered\
	id_column("planeName")\
	value_column("gun")
			std::vector< GunType > guns;
			
			int nMissiles;
			int nAmmunitions;
			int nTanks;
			int nEngines;
			int nWings;
			int nTurrets;

			Nation nation;

		private:
			//����� ����� ������� ������ ������������� ����
			std::string getCheapestModule( int &price, ModuleType MT, std::shared_ptr<odb::database> planesDB);
			//����� ����� ������� ����� ������������� ����
			std::string getCheapestGun( int &price, GunType GT, std::shared_ptr<odb::database> planesDB );

		};
		//��������� �� ���� ������ ������ � ��������� ��� ���
		std:: shared_ptr<Module> loadModule( std::string moduleName, ModuleType MT, std::shared_ptr<odb::database> db );
		//��������� �� ���� ������ ������
		std:: shared_ptr<Module> loadModule( std::string moduleName, std::shared_ptr<odb::database> db );
	}
}