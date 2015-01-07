#pragma once
//данный файл содержит класс модели самолета
#include "stdafx.h"
#include "modules.h"
#include "profile.h"

namespace rplanes
{

	namespace planedata
	{
		//модель самолета. Содержит список устанавливаемых модулей и количество модулей разных типов 
#pragma  db  object
		class Model
		{
		public:
			//загрузить из базы данных стоковую конфигурацию
			playerdata::Plane loadBasicConfiguration( std::shared_ptr<odb::database> planesDB, int & price, std::string profileName );

			//название модели самолета
#pragma db id
			std::string planeName;


			//список названий устанавливаемых модулей
#pragma db unordered\
	id_column("planeName")\
	value_column("module")
			std::vector<std::string> modules;


			//список типов орудий. Определяет количество устанавливаемых пулеметов и пушек
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
			//найти самый дешевый модуль определенного типа
			std::string getCheapestModule( int &price, ModuleType MT, std::shared_ptr<odb::database> planesDB);
			//найти самую дешевую пушку определенного типа
			std::string getCheapestGun( int &price, GunType GT, std::shared_ptr<odb::database> planesDB );

		};
		//загрузить из базы данных модуль и проверить его тип
		std:: shared_ptr<Module> loadModule( std::string moduleName, ModuleType MT, std::shared_ptr<odb::database> db );
		//загрузить из базы данных модуль
		std:: shared_ptr<Module> loadModule( std::string moduleName, std::shared_ptr<odb::database> db );
	}
}