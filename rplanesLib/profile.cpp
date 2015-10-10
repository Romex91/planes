#include "profile.h"
#include "database.h"
#include "odb/profile-odb.hxx"
#include "planesException.h"
#include "model.h"
#include "odb/model-odb.hxx"
#include "configuration.h"
namespace rplanes
{
	namespace playerdata
	{
		serverdata::Plane Plane::buildPlane( Pilot pilot ,std::shared_ptr<odb::database> db )
		{
			serverdata::Plane retval;
			retval.pilot = pilot;
			retval.name = id.planeName;
			retval.nation = nation;
			std::shared_ptr<planedata::Module> module;

#define SINGLE_MODULES( NAME, TYPE, CLASSNAME )\
	module = planedata::loadModule(NAME, TYPE, db);\
	retval.NAME = * dynamic_cast<CLASSNAME *>( module.get() )

			
			SINGLE_MODULES(cabine, CABINE, planedata::Cabine);
			SINGLE_MODULES(framework, FRAMEWORK, planedata::Framework  );
			SINGLE_MODULES( tail, TAIL, planedata::Tail );

#define VECTORED_MODULES( NAME, TYPE, CLASSNAME )\
	for(auto & i : NAME)\
			{\
			module = planedata::loadModule(i, TYPE, db);\
			retval.NAME.push_back( * dynamic_cast<CLASSNAME *>( module.get() ) );\
			}
				VECTORED_MODULES(guns, GUN, planedata::Gun)
				VECTORED_MODULES(ammunitions, AMMUNITION, planedata::Ammunition)
				VECTORED_MODULES(engines, ENGINE, planedata::Engine)
				VECTORED_MODULES(missiles, MISSILE, planedata::Missile )
				VECTORED_MODULES(tanks, TANK, planedata::Tank )
				VECTORED_MODULES(wings, WING, planedata::Wing )
				VECTORED_MODULES(turrets, TURRET, planedata::Turret)
#undef VECTORED_MODULES
#undef SINGLE_MODULES
				for ( auto & turret : retval.turrets )
				{
					turret.gun = dynamic_cast<planedata::Gun&>( *planedata::loadModule( turret.gunName, db )  );
				}
				return retval;
		}

		void Plane::save( std::shared_ptr< odb::database > profilesDB )
		{
			try
			{
				odb::transaction t(profilesDB->begin());
				profilesDB->persist(*this);
				t.commit();
			}
			catch (odb::object_already_persistent)
			{
				odb::transaction t(profilesDB->begin());
				profilesDB->update(*this);
				t.commit();
			}
		}

		int Plane::getPrice( std::shared_ptr< odb::database > planesDB )
		{
			int price = 0;
			std::shared_ptr<planedata::Module> m;

#define SINGLE_MODULES( NAME, TYPE)\
	m = planedata::loadModule(NAME, TYPE, planesDB);\
	price += m->price;

#define VECTORED_MODULES( NAME, TYPE)\
	for(auto & i : NAME)\
			{\
			auto m = planedata::loadModule(i, TYPE, planesDB);\
			price+= m->price;\
			}
			SINGLE_MODULES( cabine, CABINE );
			SINGLE_MODULES( framework, FRAMEWORK);
			SINGLE_MODULES( tail, TAIL );
			VECTORED_MODULES(guns, GUN);
			VECTORED_MODULES( wings, WING );
			VECTORED_MODULES(tanks, TANK );
			VECTORED_MODULES(ammunitions, AMMUNITION );
			VECTORED_MODULES(missiles, MISSILE );
			VECTORED_MODULES(engines, ENGINE );
			VECTORED_MODULES(turrets, TURRET);
#undef VECTORED_MODULES
#undef SINGLE_MODULES	
			return price;
		}

		bool Plane::isReadyForJoinRoom()
		{
#define CHECK_SYMMETRY(vector)\
			for (size_t i = 0; i < vector.size() / 2; i++)\
			{\
			size_t k = vector.size() - i - 1; \
			if (vector[i] != vector[k])\
			{\
			return false; \
		}\
		}

			CHECK_SYMMETRY(engines);
			CHECK_SYMMETRY(wings);
			CHECK_SYMMETRY(missiles);
#undef CHECK_SYMMETRY
			return true;
		}


		void Profile::loadPlanes( std::shared_ptr< odb::database > profilesDB )
		{
			planes.clear();
			odb::transaction t (profilesDB->begin());
			typedef odb::result<Plane> result;
			typedef odb::query<Plane> query;
			result r( profilesDB->query<Plane>( query::id.profileName == query::_ref(login) ) );
			for ( auto & i: r )
			{
				planes.push_back(i);
			}
			t.commit();
		}
		bool operator==( const profile_plane::id & first, const profile_plane::id & second )
		{
			return first.planeName == second.planeName && first.profileName == second.profileName;
		}

		void Profile::save( std::shared_ptr< odb::database > profilesDB )
		{
			//saving the profile
			try
			{
				odb::transaction t(profilesDB->begin());
				profilesDB->persist(*this);
				t.commit();
			}
			catch (odb::object_already_persistent)
			{
				odb::transaction t(profilesDB->begin());
				profilesDB->update(*this);
				t.commit();
			}
			//saving the planes
			for (auto & i: planes)
			{
				i.save( profilesDB );
			}
			//searching the database for the planes absent in the profile
			//if such a plane found it means that it was sold in this session
			typedef odb::result<Plane> result;
			typedef odb::query<Plane> query;

			odb::transaction t(profilesDB->begin());
			result r = profilesDB->query<Plane>( query::id.profileName == login );
			std::vector< profile_plane::id > idToDelete;
			for( auto & pt: r )
			{
				auto Plane = planes.begin();
				for ( ; Plane!=planes.end(); Plane++ )
				{
					if ( Plane->id == pt.id )
					{
						break;
					}
				}
				if ( Plane == planes.end() )
				{
					idToDelete.push_back(pt.id);
				}
			}
			//deleting found planes from the database
			for( auto & ID: idToDelete )
			{
				profilesDB->erase<Plane>( ID );
			}
			t.commit();

		}

		rstring::_rstrw_t Profile::buyPlane(std::string planeName, std::shared_ptr<odb::database> planesDB)
		{
			int price;
			std::shared_ptr <planedata::Model> model;
			Plane newPlane;
			try
			{
				odb::transaction t (planesDB->begin());
				model = planesDB->load<planedata::Model>( planeName );
				t.commit();
			}
			catch (odb::object_not_persistent)
			{
				return _rstrw("Plane {0} does not exist.", planeName);
			}
			try
			{
				newPlane = model->loadBasicConfiguration(planesDB, price, login);
			}
			catch( PlanesException & e)
			{
				return e.getString();
			}
			if ( openedPlanes.count( planeName ) == 0 )
			{
				return _rstrw("{0} is closed. Play campaigns to open new planes.", planeName);
			}

			if( price > money )
				return _rstrw("Not enough money.");

			for(auto i = planes.begin(); i != planes.end(); i++ )
			{
				if( i->id.planeName == planeName )
					return _rstrw("You already have {0}.", planeName);
			}
			money -= price;
			planes.push_back( newPlane );
			return _rstrw("{0} is successfully bought.", planeName);
		}

		Profile::BuyModuleResult Profile::buyModuleInternal(std::string planeName, size_t moduleNo, std::string moduleName, std::shared_ptr<odb::database > planesDB)
		{

			//search the plane in the profile
			auto Plane = planes.begin();
			for (; Plane != planes.end(); Plane++)
			{
				if (Plane->id.planeName == planeName)
					break;
			}
			if (Plane == planes.end())
				return BuyModuleResult::NOT_SUITABLE;

			//searching for a module in a database
			std::shared_ptr<planedata::Module> Module;
			try
			{
				odb::transaction t(planesDB->begin());
				Module = planesDB->load<planedata::Module>(moduleName);
				t.commit();
			}
			catch (odb::object_not_persistent)
			{
				return BuyModuleResult::NOT_FOUND;
			}
			//check the module is suitable to install
			auto priceList = modulePriceList(planeName, Module->getType(), moduleNo, planesDB);
			auto i = priceList.begin();
			for (; i != priceList.end(); i++)
			{
				if (i->first == moduleName)
				{
					break;
				}
			}
			if (i == priceList.end())
			{
				return BuyModuleResult::NOT_SUITABLE;
			}


			//check the module persists in the store or player has anough money
			if (std::find(moduleStore.begin(), moduleStore.end(), moduleName) == moduleStore.end())
				if (Module->price > money)
					return BuyModuleResult::NOT_ENOUGH_MONEY;


			// mounting the module


			//type specific macro for mounting vectored modules
#define VECTORED_MODULES( vectorName )\
	if ( moduleNo >= Plane->vectorName.size())\
								{\
			return BuyModuleResult::NOT_SUITABLE;\
								}\
				if( Plane->vectorName[moduleNo] == moduleName )\
				return BuyModuleResult::ALLREADY_MOUNTED;\
				moduleStore.push_back( Plane->vectorName[moduleNo] );\
				Plane->vectorName[moduleNo] = moduleName;

		//type specific macro for mounting single modules
#define SINGLE_MODULES(slot)\
	if( Plane->slot== moduleName )\
	return BuyModuleResult::ALLREADY_MOUNTED;\
	moduleStore.push_back( Plane->slot );\
	Plane->slot = moduleName;

			switch (Module->getType())
			{
				case GUN:
					VECTORED_MODULES(guns);
					break;
				case MISSILE:
					VECTORED_MODULES(missiles);
					break;
				case WING:
					VECTORED_MODULES(wings);
					break;
				case ENGINE:
					VECTORED_MODULES(engines);
					break;
				case AMMUNITION:
					VECTORED_MODULES(ammunitions);
				case TANK:
					VECTORED_MODULES(tanks);
					break;
				case TURRET:
					VECTORED_MODULES(turrets);
					break;
				case CABINE:
					SINGLE_MODULES(cabine);
					break;
				case FRAMEWORK:
					SINGLE_MODULES(framework);
					break;
				case TAIL:
					SINGLE_MODULES(tail);
					break;
			}
#undef VECTORED_MODULES
#undef SINGLE_MODULES	

			//delete the module from the store
			auto m = std::find(moduleStore.begin(), moduleStore.end(), moduleName);
			if (m != moduleStore.end())
			{
				moduleStore.erase(m);
				return BuyModuleResult::MOUNTED_FROM_STORE;
			}

			//withdrawing money
			money -= Module->price;
			return BuyModuleResult::MOUNTED;
		}

		rstring::_rstrw_t Profile::buyModule(std::string planeName, size_t moduleNo, std::string moduleName, std::shared_ptr<odb::database > planesDB)
		{	
			try {
				BuyModuleResult result = buyModuleInternal(planeName, moduleNo, moduleName, planesDB);
				switch (result)
				{
					case rplanes::playerdata::Profile::BuyModuleResult::MOUNTED:
						return _rstrw("Module {0} is bought and mounted.", moduleName);
					case rplanes::playerdata::Profile::BuyModuleResult::MOUNTED_FROM_STORE:
						return _rstrw("Module {0} is mounted.", moduleName);
					case rplanes::playerdata::Profile::BuyModuleResult::NOT_ENOUGH_MONEY:
						return _rstrw("Not enough money.");
					case rplanes::playerdata::Profile::BuyModuleResult::ALLREADY_MOUNTED:
						return _rstrw("Module is already mounted.");
					case rplanes::playerdata::Profile::BuyModuleResult::NOT_FOUND:
						return _rstrw("Module {0} is not found in database", moduleName);
					case rplanes::playerdata::Profile::BuyModuleResult::NOT_SUITABLE:
						return _rstrw("Cannot mount module {0} to plane {1}.", moduleName, planeName);
					default:
						return _rstrw("Unexpected error.");
				}
			} 
			catch (PlanesException & ex) 
			{
				return ex.getString();
			}
		}

		rstring::_rstrw_t Profile::buyModules(std::string planeName, std::string moduleName, std::shared_ptr<odb::database > planesDB)
		{
			//searching the plane in the profile
			auto Plane = planes.begin();
			for( ; Plane!=planes.end(); Plane++ )
			{
				if ( Plane->id.planeName == planeName )
					break;
			}
			if (Plane == planes.end())
				return _rstrw("{0} is not found.", planeName);
			//searching the module in the database
			std::shared_ptr<planedata::Module> Module;
			try
			{
				odb::transaction t(planesDB->begin());
				Module = planesDB->load<planedata::Module>( moduleName );
				t.commit();
			}
			catch( odb::object_not_persistent )
			{
				return _rstrw("{0} is not found.", moduleName);
			}

			auto moduleType = Module->getType();

			size_t nModulesBought = 0;
			size_t nModulesMountedFromStore = 0;

#define  VECTORED_MODULES(vectorName)\
			for (size_t i = 0; i < Plane->vectorName.size(); i++)\
			{\
				auto result = buyModuleInternal(planeName, i, moduleName, planesDB);\
				if (result == BuyModuleResult::MOUNTED)\
					nModulesBought++;\
				if (result == BuyModuleResult::MOUNTED_FROM_STORE)\
					nModulesMountedFromStore++;\
				if (result == BuyModuleResult::NOT_ENOUGH_MONEY)\
					break;\
			}

			switch (moduleType)
			{
			case rplanes::GUN:
				VECTORED_MODULES(guns);
				break;
			case rplanes::MISSILE:
				VECTORED_MODULES(missiles);
				break;
			case rplanes::WING:
				VECTORED_MODULES(wings);
				break;
			case rplanes::TANK:
				VECTORED_MODULES(tanks);
				break;
			case rplanes::ENGINE:
				VECTORED_MODULES(engines);
				break;
			case rplanes::AMMUNITION:
				VECTORED_MODULES(ammunitions);
				break;
			case rplanes::TURRET:
				VECTORED_MODULES(turrets);
				break;
			default:
				return buyModule(planeName, 0, moduleName, planesDB);
				break;
			}
			return _rstrw("{0} modules bought, {1} modules mounted from store.", nModulesBought, nModulesMountedFromStore);
#undef VECTORED_MODULES
		}

		std::vector<std::pair<std::string, int> > Profile::planePriceList( std::shared_ptr<odb::database> planesDB )
		{
			std::vector< std::pair <std::string, int> > retval;
			//loading the planes opened in the profile
			{
				std::vector<planedata::Model> models;
				odb::transaction t(planesDB->begin());
				odb::result<planedata::Model> r( planesDB->query<planedata::Model>() );
				for (auto & i : r)
				{
					if ( openedPlanes.count( i.planeName ) != 0 )
					{
						models.push_back(i);
					}
				}
				t.commit();
				for ( auto &Model: models )
				{
					int price;
					Plane pt = Model.loadBasicConfiguration(planesDB, price, login);
					retval.push_back( std::pair< std::string, int > (Model.planeName, price ) );
				}
			}
			//find the planes persisting in the profile and change the position in the pricelist using its negative price
			for( auto i = planes.begin(); i != planes.end(); i++ )
			{
				for( auto j = retval.begin(); j != retval.end(); j++ )
				{
					if ( i->id.planeName == j->first)
					{
						retval.erase( j );
						break;
					}
				}
				retval.push_back( std::pair <std::string, int> (i->id.planeName, - i->getPrice(planesDB) ) );
			}
			return retval;
		}

		std::vector<std::pair<std::string, int> > Profile::modulePriceList( std::string planeName, ModuleType MT, size_t moduleNo, std::shared_ptr<odb::database> planesDB )
		{

			std::vector< std::pair< std::string, int > > retval;

			//searching the plane in the profile
			auto Plane = planes.begin();
			for( ; Plane!=planes.end(); Plane++ )
			{
				if ( Plane->id.planeName == planeName )
					break;
			}
			if ( Plane == planes.end() )
				return retval;

			//loading the plane model
			std::shared_ptr<planedata::Model> model;
			try
			{
				odb::transaction t(planesDB->begin());
				model = planesDB->load<planedata::Model>( planeName );
				t.commit();
			}
			catch( odb::object_not_persistent )
			{
				throw PlanesException(_rstrw("Model {0} is not found.", planeName));
			}
			//loading modules suitable to this plane
			try
			{
				for( auto & Module: model->modules )
				{
					auto m = planedata::loadModule(Module, planesDB);
					if ( m->getType() == MT )
						retval.push_back( std::pair < std::string, int > ( Module, m->price ) );
				}
			}
			catch( odb::object_not_persistent )
			{
				throw PlanesException(_rstrw("Model {0} is invalid.", planeName));
			}

			//for modules persisting in the store set prices to 0
			for( auto i = moduleStore.begin(); i != moduleStore.end(); i++ )
			{
				for ( auto j = retval.begin(); j != retval.end(); j++ )
				{
					if ( *i == j->first)
					{
						j->second *= 0;
					}
				}
			}

			//for guns delete modules
			try
			{
				if (MT == GUN)
				{
					if ( moduleNo >= model->guns.size() ) //return empty array if moduleNo is incorrect
					{
						retval.clear();
						return retval;
					}
					for( auto i = retval.begin(); i!=retval.end(); i++ )
					{
						auto Gun =  * dynamic_cast< planedata::Gun * >(  planedata::loadModule( i->first, planesDB ).get() );
						if ( !Gun.isSuitable(model->guns[ moduleNo ]) )
						{
							retval.erase(i);
							i = retval.begin() - 1;
						}
					}
				}
			}
			catch( odb::object_not_persistent )
			{
				throw PlanesException(_rstrw("Model {0} is invalid.", planeName));
			}

			//delete modules allready stored on the plane

#define VECTORED_MODULES( vectorName )\
	if ( moduleNo >= Plane->vectorName.size() )\
			{\
			retval.clear();\
			return retval;\
			}\
			for ( auto i = retval.begin(); i != retval.end(); i++ )\
			{\
			if ( i->first == Plane->vectorName[ moduleNo ] )\
			{\
			retval.erase(i);\
			break;\
			}\
			}


#define SINGLE_MODULES(slot)\
	for ( auto i = retval.begin(); i != retval.end(); i++ )\
			{\
			if ( i->first == Plane->slot )\
			{\
			retval.erase(i);\
			break;\
			}\
			}

			switch (MT)
			{
			case GUN:
				VECTORED_MODULES(guns);
				break;
			case MISSILE:
				VECTORED_MODULES(missiles);
				break;
			case AMMUNITION:
				VECTORED_MODULES(ammunitions);
				break;
			case WING:
				VECTORED_MODULES(wings);
				break;
			case TURRET:
				VECTORED_MODULES(turrets);
				break;
			case ENGINE:
				VECTORED_MODULES(engines);
				break;
			case TANK:
				VECTORED_MODULES(tanks);
				break;
			case CABINE:
				SINGLE_MODULES(cabine);
				break;
			case FRAMEWORK:
				SINGLE_MODULES(framework);
				break;
			case TAIL:
				SINGLE_MODULES(tail);
				break;
			}
#undef VECTORED_MODULES
#undef SINGLE_MODULES	
			return retval;
		}

		std::vector<std::pair<std::string, int> > Profile::moduleStorePriceList( std::shared_ptr<odb::database> planesDB )
		{
			std::vector<std::pair<std::string, int> > retval;
			for( auto & Module: moduleStore )
			{
				auto m = planedata::loadModule(Module, planesDB);
				retval.push_back( std::pair < std::string, int > ( Module, m->price ) );
			}
			return retval;
		}

		rstring::_rstrw_t Profile::sellPlane(std::string planeName, std::shared_ptr<odb::database> planesDB)
		{
			try {
				auto Plane = planes.begin();
				for (; Plane != planes.end(); Plane++)
				{
					if (Plane->id.planeName == planeName)
					{
						break;
					}
				}
				if (Plane == planes.end())
				{
					return _rstrw("{0} is not found.", planeName);
				}
				money -= Plane->getPrice(planesDB);
				planes.erase(Plane);
				return _rstrw("Successfully sold {0}.", planeName);
			}
			catch (PlanesException& e)
			{
				return e.getString();
			}
		}

		rstring::_rstrw_t Profile::sellModule(std::string moduleName, size_t nModules, std::shared_ptr<odb::database> planesDB)
		{
			try {
				std::shared_ptr<planedata::Module> m;
				m = planedata::loadModule(moduleName, planesDB);
				size_t nSoldModules = 0;
				while (nModules != nModules)
				{
					auto Module = moduleStore.begin();
					for (; Module != moduleStore.end(); Module++)
					{
						if (*Module == moduleName)
							break;
					}
					if (Module == moduleStore.end())
						break;

					money += m->price;
					moduleStore.erase(Module);

					nSoldModules++;
				}
				return _rstrw("Sold {0} modules {1}", nSoldModules, moduleName);
			}
			catch (PlanesException & e)
			{
				return e.getString();
			}

		}

		Profile::Profile()
		{
			money = configuration().profile.startMoney;
			openedMaps.insert(configuration().profile.startMaps.begin(), 
				configuration().profile.startMaps.end());
			openedPlanes.insert(configuration().profile.startPlanes.begin(),
				configuration().profile.startPlanes.end());
		}


	}
}
