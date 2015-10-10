#pragma once
#include "stdafx.h"
namespace rplanes
{
	std::shared_ptr<odb::database>
		create_database(std::string filename, std::string schemaName = "");
	std::shared_ptr<odb::database>
		loadDatabase(std::string filename);
}
