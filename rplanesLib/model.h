#pragma once
#include "stdafx.h"
#include "modules.h"
#include "profile.h"

namespace rplanes
{

	namespace planedata
	{
		//plane model. all the data about each specific plane to store in database
		//Contains number of modules for each type and the list of modules that fit the plane.
#pragma  db  object
		class Model
		{
		public:
			//creates a plane with the cheapest modules set
			playerdata::Plane loadBasicConfiguration( std::shared_ptr<odb::database> planesDB, int & price, std::string profileName );

			//plane name
#pragma db id
			std::string planeName;


			//names of modules fitting this plane
#pragma db unordered\
	id_column("planeName")\
	value_column("module")
			std::vector<std::string> modules;


			//determines number of guns for each gun type
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
			std::string getCheapestModule( int &price, ModuleType MT, std::shared_ptr<odb::database> planesDB);
			std::string getCheapestGun( int &price, GunType GT, std::shared_ptr<odb::database> planesDB );

		};
		std:: shared_ptr<Module> loadModule( std::string moduleName, ModuleType MT, std::shared_ptr<odb::database> db );
		std:: shared_ptr<Module> loadModule( std::string moduleName, std::shared_ptr<odb::database> db );
	}
}