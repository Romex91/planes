
#include "stdafx.h"
using namespace odb::core;
using namespace rplanes;

int main()
{
	std::cout << "Select database: plane prof " << std::endl;
	std::string dbName;
	std::cin >>dbName;
	if ( dbName == "plane" )
	{
		std::string yes;
		std::cout << "Are you sure you want to recreate planes.db? All data will be lost." <<std::endl << "Print \"yes\" to continue"<< std::endl;
		std::cin >> yes;
		if ( yes != "yes" )
		{
			return 0;
		}
		auto db( create_database("planes.db", "planesDB") );
		transaction t(db->begin());
		{
			using namespace planedata;
			Gun();
			Wing();
			Tail();
			Tank();
			Framework();
			Ammunition();
			Missile();
			Cabine();
			Engine();
			Model();
		}
		t.commit();

	}
	if ( dbName == "prof" )
	{
		std::string yes;
		std::cout << "Are you sure you want to recreate profiles.db? All data will be lost." <<std::endl << "Print \"yes\" to continue"<< std::endl;
		std::cin >> yes;
		if ( yes != "yes" )
		{
			return 0;
		}
		auto db( create_database("profiles.db", "profilesDB") );

		try
		{
			transaction t(db->begin());
			auto Profile = db->load<playerdata::Profile>("1");
			t.commit();
		}
		catch(...)
		{}
	}
	system("pause");
}
