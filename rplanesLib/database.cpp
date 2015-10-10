#include "database.h"
#include <odb/sqlite/database.hxx>
std::shared_ptr<odb::database> rplanes::create_database(std::string filename, std::string schemaName/*=""*/)
{
	using namespace std;
	using namespace odb::core;

	shared_ptr<database> db(
		new odb::sqlite::database(filename, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
	{
		connection_ptr c(db->connection());

		c->execute("PRAGMA foreign_keys=OFF");

		transaction t(c->begin());
		schema_catalog::create_schema(*db, schemaName);
		t.commit();

		c->execute("PRAGMA foreign_keys=ON");
	}
	return db;
}

std::shared_ptr<odb::database> rplanes::loadDatabase(std::string filename)
{
	using namespace std;
	using namespace odb::core;
	shared_ptr<database> db(
		new odb::sqlite::database(filename, SQLITE_OPEN_READWRITE));
	return db;
}
