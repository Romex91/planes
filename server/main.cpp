#include "server.h"



Server server;

extern std::shared_ptr<odb::database> profilesDB;

//message handlers
namespace rplanes
{
	namespace network
	{
		namespace clientmessages
		{
			void MStatusRequest::handle()
			{
				network::servermessages::MStatus mess;
				auto & Client = server.getClient(clientID);
				mess.status =  Client.getStatus();
				Client.sendMessage(mess);
			}
			namespace room
			{
				void MSendControllable::handle()
				{
					auto & Client = server.getClient(clientID);
					Client.setControllable(params);
				}

				void MServerTimeRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					MServerTime mess;
					mess.time = server.getTime();
					Client.sendMessage(mess);
				}

				void MAdministerRoom::handle()
				{
					try
					{
						server.administerRoom(clientID, operation, options);
					}
					catch (PlanesException & e)
					{
						network::MResourceString mess;
						mess.string = e.getString();
						auto & Client = server.getClient(clientID);
						Client.sendMessage(mess);
					}

				}
			}
			namespace hangar
			{

				void MProfileRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					network::MProfile mess;
					mess.profile = Client.profile();
					Client.sendMessage(mess);
				}

				void MPlayerProfileRequest::handle()
				{ 
					auto & Client = server.getClient(clientID);
					network::MProfile profileMessage;
					try
					{
						odb::transaction t(profilesDB->begin());
						profileMessage.profile = *profilesDB->load<rplanes::playerdata::Profile>(playerName);
						t.commit();
					}
					catch (...)
					{
						network::MResourceString txt;
						txt.string = _rstrw("Profile {0} is not found in database.", playerName);
						Client.sendMessage(txt);
						return;
					}
					Client.sendMessage(profileMessage);
				}

				void MSellModuleRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					network::MResourceString mess;
					mess.string = Client.profile().sellModule(moduleName, nModulesToSell, planesDB);
					Client.sendMessage(mess);
				}

				void MSellPlaneRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					network::MResourceString mess;
					mess.string = Client.profile().sellPlane(planeName, planesDB);
					Client.sendMessage(mess);
				}

				void MBuyModuleRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					if(setToAllSlots)
					{
						network::MResourceString mess;
						mess.string = Client.profile().buyModules(planeName, moduleName,planesDB);
						Client.sendMessage(mess);						
					}
					else
					{
						network::MResourceString mess;
						mess.string = Client.profile().buyModule(planeName, moduleNo, moduleName,planesDB);
						Client.sendMessage(mess);
					}
				}

				void MBuyPlaneRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					network::MResourceString mess;
					mess.string = Client.profile().buyPlane(planeName,planesDB);
					Client.sendMessage(mess);
				};

				void MUpSkillRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					auto & pilot = Client.profile().pilot;
					switch (skill)
					{
					case rplanes::network::clientmessages::hangar::MUpSkillRequest::FLIGHT:
						pilot.up_flight(experienceToSpend);
						break;
					case rplanes::network::clientmessages::hangar::MUpSkillRequest::ENDURANCE:
						pilot.up_endurance(experienceToSpend);
						break;
					case rplanes::network::clientmessages::hangar::MUpSkillRequest::SHOOTING:
						pilot.up_shooting(experienceToSpend);
						break;
					case rplanes::network::clientmessages::hangar::MUpSkillRequest::ENGINE:
						pilot.up_engine(experienceToSpend);
						break;
					}
				}

				void MJoinRoomRequest::handle()
				{
					try
					{
						server.joinRoom(clientID, playerName, planeNo);
					}
					catch(PlanesException & e)
					{
						network::MResourceString mess;
						mess.string = e.getString();
						auto & Client = server.getClient(clientID);
						Client.sendMessage(mess);
					}
				};

				void MCreateRoomRequest::handle()
				{
					try
					{
						server.createRoom(clientID, description, mapName);
					}
					catch (PlanesException & e)
					{
						network::MResourceString mess;
						mess.string = e.getString();
						auto & Client = server.getClient(clientID);
						Client.sendMessage(mess);
					}
				};

				void MDestroyRoomRequest::handle()
				{
					try
					{
						server.destroyRoom(clientID);
					}
					catch (PlanesException & e)
					{
						network::MResourceString mess;
						mess.string = e.getString();
						auto & Client = server.getClient(clientID);
						Client.sendMessage(mess);
					}
				}

				void MRoomListRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					Client.profile();//just to check the client is in hangar
					network::MRoomList message;
					{
						MutexLocker ml(server.roomListMessage.mutex);
						message = server.roomListMessage.message;
					}
					Client.sendMessage(message);
				};

			}
			namespace unlogined
			{

				void MLogin::handle()
				{
					auto & Client = server.getClient(clientID);
					try
					{
						Client.login(name,encryptedPassword);
						MServerConfiguration confMessage;
						confMessage.conf = configuration();
						Client.sendMessage(confMessage);
					}
					catch( PlanesException & e )
					{
						network::MResourceString mess;
						mess.string = e.getString();
						Client.sendMessage(mess);
						throw e;
					}
				}

				void MRegistry::handle()
				{
					auto & Client = server.getClient(clientID);
					playerdata::Profile Profile;
					Profile.login = name;
					Profile.password = password;
					odb::transaction t(profilesDB->begin());
					profilesDB->persist(Profile);
					t.commit();
				}
			}
		}

		namespace bidirectionalmessages
		{

			void MText::handle()
			{
				std::wcout << _rstrw(":client{0}: {1}", clientID, text).str() << std::endl;
			}

			void MResourceString::handle()
			{
				std::wcout << _rstrw(":client{0}: {1}", clientID, string).str() << std::endl;
			}

			void MExitRoom::handle()
			{
				auto & client = server.getClient(clientID);
				client.prepareRoomExit();
			}
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
				groups.addContainer(room.second.map_.humanGroups);
				groups.addContainer(room.second.map_.botGroups);
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