#include "profile.h"
#include "database.h"
#include "odb/profile-odb.hxx"
#include "exceptions.h"
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

			//макрос для одиночных модулей
#define SINGLE_MODULES( NAME, TYPE, CLASSNAME )\
	module = planedata::loadModule(NAME, TYPE, db);\
	retval.NAME = * dynamic_cast<CLASSNAME *>( module.get() )

			
			SINGLE_MODULES(cabine, CABINE, planedata::Cabine);
			SINGLE_MODULES(framework, FRAMEWORK, planedata::Framework  );
			SINGLE_MODULES( tail, TAIL, planedata::Tail );

				//макрос для модулей, содержащихся в векторе
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
				//загружаем ПУШКИ для турелей
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
			//макрос для одиночных модулей
#define SINGLE_MODULES( NAME, TYPE)\
	m = planedata::loadModule(NAME, TYPE, planesDB);\
	price += m->price;
			//макрос для модулей, содержащихся в векторе
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
			//проверяем симметричность некоторых модулей
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
			//сохранение профиля
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
			//сохранение самолетов профиля
			for (auto & i: planes)
			{
				i.save( profilesDB );
			}
			//поиск в базе данных самолетов, остутствующих в профиле
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
			//удаление найденных самолетов из базы данных
			for( auto & ID: idToDelete )
			{
				profilesDB->erase<Plane>( ID );
			}
			t.commit();

		}

		std::string Profile::buyPlane( std::string planeName, std::shared_ptr<odb::database> planesDB )
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
				return "Самолета с таким именем не существует " + planeName + ".";
			}
			try
			{
				newPlane = model->loadBasicConfiguration(planesDB, price, login);
			}
			catch( planesException & e)
			{
				return e.what();
			}
			if ( openedPlanes.count( planeName ) == 0 )
			{
				return "Самолет не открыт. Проходите миссии, чтобы открыть самолеты.";
			}

			if( price > money )
				return "Недостаточно денег.";

			for(auto i = planes.begin(); i != planes.end(); i++ )
			{
				if( i->id.planeName == planeName )
					return std::string("У вас уже есть ") + planeName + ".";
			}
			money -= price;
			planes.push_back( newPlane );
			return planeName + " куплен.";
		}

		std::string Profile::buyModule( std::string planeName, size_t moduleNo, std::string moduleName, std::shared_ptr<odb::database > planesDB )
		{	
			//поиск самолета в профиле
			auto Plane = planes.begin();
			for( ; Plane!=planes.end(); Plane++ )
			{
				if ( Plane->id.planeName == planeName )
					break;
			}
			if ( Plane == planes.end() )
				return std::string("В ангаре нет самолета ") + planeName + " profile::buyModule";
			//поиск модуля в базе данных
			std::shared_ptr<planedata::Module> Module;
			try
			{
				odb::transaction t(planesDB->begin());
				Module = planesDB->load<planedata::Module>( moduleName );
				t.commit();
			}
			catch( odb::object_not_persistent )
			{
				return "Модуль с именем " + moduleName + " не найден в базе данных.";
			}
			//проверка возможности установки
			try
			{
				auto priceList = modulePriceList(planeName, Module->getType(), moduleNo, planesDB);
				auto i = priceList.begin();
				for ( ; i != priceList.end(); i++ )
				{
					if ( i->first == moduleName )
					{
						break;
					}
				}
				if ( i == priceList.end() )
				{
					return "На самолет " + planeName + " нельзя поставить модуль " + moduleName + "." ;
				}
			}
			catch( planesException & e )
			{
				return e.what();
			}

			//проверка возможности покупки или установки со склада
			if ( std::find( moduleStore.begin(), moduleStore.end(), moduleName ) == moduleStore.end() )
				if ( Module->price > money )
					return "Не хватает денег.";

			// установка модуля на самолет


			//макросы установки модуля конкретного типа
#define VECTORED_MODULES( vectorName )\
	if ( moduleNo >= Plane->vectorName.size())\
			{\
			return "Параметр moduleNo задан не неверно.";\
				}\
				if( Plane->vectorName[moduleNo] == moduleName )\
				return "Данный мудуль уже установлен.";\
				moduleStore.push_back( Plane->vectorName[moduleNo] );\
				Plane->vectorName[moduleNo] = moduleName;


#define SINGLE_MODULES(slot)\
	if( Plane->slot== moduleName )\
	return "модуль уже установлен";\
	moduleStore.push_back( Plane->slot );\
	Plane->slot = moduleName;

			switch ( Module->getType() )
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
				VECTORED_MODULES( ammunitions );
			case TANK:
				VECTORED_MODULES( tanks );
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

			//удаление модуля со склада
			auto m = std::find( moduleStore.begin(), moduleStore.end(), moduleName );
			if ( m != moduleStore.end() )
			{
				moduleStore.erase(m);
				return "Модуль установлен со склада.";
			}

			//снятие денег
			money-= Module->price;
			return "Модуль приобретен и установлен.";
		}

		std::string Profile::buyModules( std::string planeName, std::string moduleName, std::shared_ptr<odb::database > planesDB )
		{
			std::string retval;

			//поиск самолета в профиле
			auto Plane = planes.begin();
			for( ; Plane!=planes.end(); Plane++ )
			{
				if ( Plane->id.planeName == planeName )
					break;
			}
			if ( Plane == planes.end() )
				return std::string("В ангаре нет самолета ") + planeName + " profile::buyModule";
			//поиск модуля в базе данных
			std::shared_ptr<planedata::Module> Module;
			try
			{
				odb::transaction t(planesDB->begin());
				Module = planesDB->load<planedata::Module>( moduleName );
				t.commit();
			}
			catch( odb::object_not_persistent )
			{
				return "Модуль с именем " + moduleName + " не найден в базе данных.";
			}

			auto moduleType = Module->getType();
			switch (moduleType)
			{
			case rplanes::GUN:
				for (  size_t i = 0; i < Plane->guns.size(); i++ )
				{
					retval+= buyModule(planeName, i, moduleName, planesDB) + "\n";
				}
				break;
			case rplanes::MISSILE:
				for (  size_t i = 0; i < Plane->missiles.size(); i++ )
				{
					retval+= buyModule(planeName, i, moduleName, planesDB) + "\n";
				}
				break;
			case rplanes::WING:
				for (  size_t i = 0; i < Plane->wings.size(); i++ )
				{
					retval+= buyModule(planeName, i, moduleName, planesDB) + "\n";
				}
				break;
			case rplanes::TANK:
				for (  size_t i = 0; i < Plane->tanks.size(); i++ )
				{
					retval+= buyModule(planeName, i, moduleName, planesDB) + "\n";
				}
				break;
			case rplanes::ENGINE:
				for (  size_t i = 0; i < Plane->engines.size(); i++ )
				{
					retval+= buyModule(planeName, i, moduleName, planesDB) + "\n";
				}
				break;
			case rplanes::AMMUNITION:
				for (  size_t i = 0; i < Plane->ammunitions.size(); i++ )
				{
					retval+= buyModule(planeName, i, moduleName, planesDB) + "\n";
				}
				break;
			case rplanes::TURRET:
				for (size_t i = 0; i < Plane->turrets.size(); i++)
				{
					retval += buyModule(planeName, i, moduleName, planesDB) + "\n";
				}
				break;
			default:
				retval = buyModule(planeName, 0, moduleName, planesDB) + "\n";
				break;
			}
			return retval;
		}

		std::vector<std::pair<std::string, int> > Profile::planePriceList( std::shared_ptr<odb::database> planesDB )
		{
			std::vector< std::pair <std::string, int> > retval;
			//загружаем модели открытых самолетов
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
			//заменяем цену купленных самолетов на отрицательную
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

			//поиск самолета в профиле
			auto Plane = planes.begin();
			for( ; Plane!=planes.end(); Plane++ )
			{
				if ( Plane->id.planeName == planeName )
					break;
			}
			if ( Plane == planes.end() )
				return retval;

			//загрузка модели
			std::shared_ptr<planedata::Model> model;
			try
			{
				odb::transaction t(planesDB->begin());
				model = planesDB->load<planedata::Model>( planeName );
				t.commit();
			}
			catch( odb::object_not_persistent )
			{
				throw eModelTemplateNotFound( "Название самолета " + planeName + ". ");
			}
			//загрузка устанавливаемых на самолет модулей данного типа
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
				throw eModelTemplateNotFull("Название самолета " + planeName + ". ");
			}

			//пометить имеющиеся в ангаре
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

			//Убрать не подходящие по типу подвески
			try
			{
				if (MT == GUN)
				{
					if ( moduleNo >= model->guns.size() ) //если pos не корректна, вернуть пустой массив
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
				throw eModelTemplateNotFull("Название самолета " + planeName + ". ");
			}


			//убрать уже установленные модули

			//макросы установки модуля конкретного типа
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

		std::string Profile::sellPlane( std::string planeName, std::shared_ptr<odb::database> planesDB )
		{
			auto Plane = planes.begin();
			for ( ; Plane!= planes.end(); Plane++)
			{
				if ( Plane->id.planeName == planeName )
				{
					break;
				}
			}
			if ( Plane == planes.end() )
			{
				return "Самолет с именем " + planeName + " отсутствует.";
			}
			try
			{
				money -= Plane->getPrice(planesDB); 
			}
			catch( planesException& e )
			{
				return e.what();
			}
			planes.erase(Plane);
			return planeName + " продан.";
		}

		std::string Profile::sellModule( std::string moduleName, size_t nModules , std::shared_ptr<odb::database> planesDB )
		{
			//загружаем указанный модуль из базы данных
			std::shared_ptr<planedata::Module> m;
			try
			{
				m = planedata::loadModule(moduleName, planesDB);
			}
			catch(planesException & e)
			{
				return e.what();
			}
			std::string retval = "продано ";
			size_t nSoldModules = 0;
			//пока не продано достаточное количество модулей
			while ( nModules != nModules )
			{
				//ищем модуль в хранилище
				auto Module = moduleStore.begin();
				for (; Module!=moduleStore.end(); Module++)
				{
					if( *Module == moduleName )
						break;
				}
				if( Module == moduleStore.end() )
					break;

				//прибавляем деньги
				money += m->price;
				//удаляем модуль из хранилища
				moduleStore.erase( Module );

				nSoldModules++;
			}
			return retval + " модулей " + moduleName;
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
