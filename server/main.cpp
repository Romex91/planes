#include "server.h"



Server server;

extern std::shared_ptr<odb::database> profilesDB;

//message handlers
namespace rplanes
{
	namespace network
	{
		namespace bidirectionalmessages
		{

		}
	}
}


class consoleHandler
{
	std::string line;
	std::string command;
	std::vector<std::string> options;
	void help()
	{
		std::cout 
			<< "usage:" << std::endl
			<< "show [planes, clients, players, rooms]" << std::endl
			<< "" << std::endl
			<< "" << std::endl;
	}
	void show()
	{
		if ( options.size() != 1)
		{
			help();
			return;
		}
		if ( options[0] == "planes" )
		{
			MutexLocker ml(server.roomClients_.mutex);
			for ( auto & room : server.rooms_)
			{
				std::vector< std::shared_ptr<Player > > players;

				ContainersMerger<PlayerGroup> groups;
				groups.addContainer(room.second->map_.humanGroups);
				groups.addContainer(room.second->map_.botGroups);
				groups.for_each([ &players ]( PlayerGroup & group )
				{
					auto playersToAdd = group.getPlayers();
					players.insert(players.begin(), playersToAdd.begin(), playersToAdd.end());
				});

				for (auto & player : players)
				{
					std::cout << player->name << std::endl;
					if ( player->plane_.isDestroyed() )
					{
						std::wcout << _rstrw("plane is destroyed").str() << std::endl; 
					}
					player->plane_.showParams();
				}
			}
		}
		else
		{
			help();
		}
	}
public:
	void loop()
	{
		std::stringstream ss;
		while (true)
		{
			std::getline( std::cin, line );
			if ( line.size() == 0 )
			{
				continue;
			}
			ss.str("");
			ss.clear();
			ss << line;
			ss >> command;
			options.clear();
			std::string option;
			ss >> option;
			while ( ss )
			{
				options.push_back(option);
				ss >> option;
			}

#define CHECK_COMMAND(cmd)\
	if( #cmd == command )\
			{\
			cmd();\
			continue;\
			}
			CHECK_COMMAND(show);
			CHECK_COMMAND(help);
			help();
#undef CHECK_COMMAND
		}

	}
};


int main()
{
#ifdef _MSC_VER
	system("chcp 65001");
#endif
	boost::locale::generator gen;
	std::locale::global(gen.generate(std::locale(), ""));
	std::wcout.imbue(std::locale());

	std::wifstream xmlFs(rplanes::configuration().server.languageXml);
	boost::archive::xml_wiarchive archive(xmlFs, boost::archive::no_header);
	archive >> boost::serialization::make_nvp("strings", rstring::_rstrw_t::resource());


	planesDB = rplanes::loadDatabase("../Resources/planes.db");
	profilesDB = rplanes::loadDatabase("../Resources/profiles.db");
	omp_set_num_threads(3);
	omp_set_nested(true);

#pragma omp parallel sections
	{
#pragma omp section
		{
			server.hangarLoop();
		}
#pragma omp section
		{
			server.roomLoop();
		}
#pragma omp section
		{
			consoleHandler ch;
			ch.loop();
		}
	}
}