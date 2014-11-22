#pragma once
#include "stdafx.h"
#include <odb/sqlite/database.hxx>
namespace rplanes
{

	inline std::shared_ptr<odb::database>
		create_database (std::string filename, std::string schemaName="")
	{
		using namespace std;
		using namespace odb::core;

		shared_ptr<database> db (
			new odb::sqlite::database( filename, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
		{
			connection_ptr c (db->connection ());

			c->execute ("PRAGMA foreign_keys=OFF");

			transaction t (c->begin ());
			schema_catalog::create_schema (*db, schemaName);
			t.commit ();

			c->execute ("PRAGMA foreign_keys=ON");
		}
		return db;
	}
	inline std::shared_ptr<odb::database>
		loadDatabase( std::string filename )
	{
		using namespace std;
		using namespace odb::core;
		shared_ptr<database> db (
			new odb::sqlite::database( filename, SQLITE_OPEN_READWRITE));
		return db;
	}
}
