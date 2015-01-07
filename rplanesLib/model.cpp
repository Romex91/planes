#include "database.h"
#include "model.h"
#include "odb/model-odb.hxx"
#include "exceptions.h"
namespace rplanes
{
	using namespace playerdata;
	namespace planedata
	{
		Plane Model::loadBasicConfiguration( std::shared_ptr<odb::database> db, int & price, std::string profileName )
		{
			Plane retVal;

			retVal.id.planeName = planeName;
			retVal.id.profileName = profileName;
			retVal.nation = nation;

			price = 0;

			int minPrice;

			std::string moduleName;
			//----------------------------------------------------guns--------------------------------------
			if( guns.size() > 0 )
			{
				for( auto & Gun: guns  )
				{
					retVal.guns.push_back(getCheapestGun(minPrice, Gun, db));
					price += minPrice;
				}
			}
			//макрос для модулей, содержащихся в векторе
#define VECTORED_MODULES(TYPE, COUNT, NAME)\
	if(COUNT>0)\
	moduleName = getCheapestModule( minPrice, TYPE, db);\
	retVal.NAME = std::vector<std::string>( COUNT, moduleName);\
	price += minPrice * COUNT;
			//макрос для одиночных модулей
#define SINGLE_MODULES( TYPE, NAME )\
	retVal.NAME = getCheapestModule( minPrice, TYPE, db);\
	price += minPrice;	
			VECTORED_MODULES(MISSILE, nMissiles, missiles);
			VECTORED_MODULES( AMMUNITION, nAmmunitions, ammunitions );
			VECTORED_MODULES ( ENGINE, nEngines, engines );
			VECTORED_MODULES ( WING, nWings, wings );
			VECTORED_MODULES ( TANK, nTanks, tanks );
			VECTORED_MODULES(TURRET, nTurrets, turrets);
			SINGLE_MODULES(CABINE,cabine);
			SINGLE_MODULES(FRAMEWORK,framework);
			SINGLE_MODULES(TAIL,tail);
#undef VECTORED_MODULES
#undef SINGLE_MODULES	
			return retVal;

		}


		std::string Model::getCheapestModule( int &price, ModuleType MT, std::shared_ptr<odb::database> db )
		{
			std::string retval;
			bool firstIteration = true;
			for ( auto & moduleName : this->modules )
			{
				float modulePrice;
				{
					//загрузка модуля с указанным именем из базы данных
					auto Module = loadModule(moduleName, db);
					//если модуль не подходит по типу, проверяем список дальше
					if ( Module->getType() != MT )
					{
						continue;
					}
					modulePrice = Module->price;
				}
				if ( firstIteration )
				{
					firstIteration = false;
					price = modulePrice;
					retval = moduleName;
				}else
				{
					if ( price > modulePrice )
					{
						price = modulePrice;
						retval = moduleName;
					}
				}
			}
			if ( retval.size() == 0 )
				throw(eModelTemplateNotFull("Название самолета " + planeName + ". "));
			return retval;
		}

		std::string Model::getCheapestGun( int &price, GunType GT, std::shared_ptr<odb::database> db )
		{
			std::string retval;
			bool firstIteration = true;
			for ( auto & moduleName : this->modules )
			{
				float modulePrice;
				{
					//загрузка модуля с указанным именем из базы данных
					auto Module = loadModule( moduleName, db);
					//если модуль не подходит по типу, проверяем список дальше
					if ( Module->getType() != GUN )
					{
						continue;
					}
					//если пушка не соответствует подвеске, проверяем список дальше
					if ( !dynamic_cast<Gun*> (Module.get())->isSuitable(GT) )
					{
						continue;
					}
					modulePrice = Module->price;
				}
				if ( firstIteration )
				{
					firstIteration = false;
					price = modulePrice;
					retval = moduleName;
				}else
				{
					if ( price > modulePrice )
					{
						price = modulePrice;
						retval = moduleName;
					}
				}
			}
			if ( retval.size() == 0 )
				throw(eModelTemplateNotFull("Название самолета " + planeName + "."));
			return retval;
		}



		std:: shared_ptr<Module> loadModule( std::string moduleName, ModuleType MT, std::shared_ptr<odb::database> db )
		{
			std:: shared_ptr<Module> module;
			try
			{
				odb::transaction t(db->begin());
				module = db->load<Module>(moduleName);
				t.commit();
			}
			catch(odb::object_not_persistent)
			{
				throw (eModuleNotFound("Название модуля " + moduleName + ". "));
			}
			if ( module->getType() != MT)
			{
				throw(eModulType("Название модуля " + moduleName + ". "));
			}
			return module;
		}

		std:: shared_ptr<Module> loadModule( std::string moduleName, std::shared_ptr<odb::database> db )
		{
			std:: shared_ptr<Module> module;
			try
			{
				odb::transaction t(db->begin());
				module = db->load<Module>(moduleName);
				t.commit();
			}
			catch(odb::object_not_persistent)
			{
				throw (eModuleNotFound("Название модуля " + moduleName + ". "));
			}
			return module;
		}
	}
}
