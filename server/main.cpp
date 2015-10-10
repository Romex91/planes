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
			void StatusRequest::handle()
			{
				network::servermessages::StatusMessage mess;
				auto & Client = server.getClient(clientID);
				mess.status =  Client.getStatus();
				Client.sendMessage(mess);
			}
			namespace room
			{
				void SendControllable::handle()
				{
					auto & Client = server.getClient(clientID);
					Client.setControllable(params);
				}

				void ServerTimeRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					servermessages::room::ServerTime mess;
					mess.time = server.getTime();
					Client.sendMessage(mess);
				}

				void AdministerRoom::handle()
				{
					try
					{
						server.administerRoom(clientID, operation, options);
					}
					catch (PlanesException & e)
					{
						network::bidirectionalmessages::ResourceStringMessage mess;
						mess.string = e.getString();
						auto & Client = server.getClient(clientID);
						Client.sendMessage(mess);
					}

				}
			}
			namespace hangar
			{

				void ProfileRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					network::servermessages::hangar::SendProfile mess;
					mess.profile = Client.profile();
					Client.sendMessage(mess);
				}

				void PlayerProfileRequest::handle()
				{ 
					auto & Client = server.getClient(clientID);
					network::servermessages::hangar::SendProfile profileMessage;
					try
					{
						odb::transaction t(profilesDB->begin());
						profileMessage.profile = *profilesDB->load<rplanes::playerdata::Profile>(playerName);
						t.commit();
					}
					catch (...)
					{
						network::bidirectionalmessages::ResourceStringMessage txt;
						txt.string = _rstrw("Profile {0} is not found in database.", playerName);
						Client.sendMessage(txt);
						return;
					}
					Client.sendMessage(profileMessage);
				}

				void SellModuleRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					network::bidirectionalmessages::ResourceStringMessage mess;
					mess.string = Client.profile().sellModule(moduleName, nModulesToSell, planesDB);
					Client.sendMessage(mess);
				}

				void SellPlaneRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					network::bidirectionalmessages::ResourceStringMessage mess;
					mess.string = Client.profile().sellPlane(planeName, planesDB);
					Client.sendMessage(mess);
				}

				void BuyModuleRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					if(setToAllSlots)
					{
						network::bidirectionalmessages::ResourceStringMessage mess;
						mess.string = Client.profile().buyModules(planeName, moduleName,planesDB);
						Client.sendMessage(mess);						
					}
					else
					{
						network::bidirectionalmessages::ResourceStringMessage mess;
						mess.string = Client.profile().buyModule(planeName, moduleNo, moduleName,planesDB);
						Client.sendMessage(mess);
					}
				}

				void BuyPlaneRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					network::bidirectionalmessages::ResourceStringMessage mess;
					mess.string = Client.profile().buyPlane(planeName,planesDB);
					Client.sendMessage(mess);
				};

				void UpSkillRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					auto & pilot = Client.profile().pilot;
					switch (skill)
					{
					case rplanes::network::clientmessages::hangar::UpSkillRequest::FLIGHT:
						pilot.up_flight(experienceToSpend);
						break;
					case rplanes::network::clientmessages::hangar::UpSkillRequest::ENDURANCE:
						pilot.up_endurance(experienceToSpend);
						break;
					case rplanes::network::clientmessages::hangar::UpSkillRequest::SHOOTING:
						pilot.up_shooting(experienceToSpend);
						break;
					case rplanes::network::clientmessages::hangar::UpSkillRequest::ENGINE:
						pilot.up_engine(experienceToSpend);
						break;
					}
				}

				void JoinRoomRequest::handle()
				{
					try
					{
						server.joinRoom(clientID, playerName, planeNo);
					}
					catch(PlanesException & e)
					{
						network::bidirectionalmessages::ResourceStringMessage mess;
						mess.string = e.getString();
						auto & Client = server.getClient(clientID);
						Client.sendMessage(mess);
					}
				};

				void CreateRoomRequest::handle()
				{
					try
					{
						server.createRoom(clientID, description, mapName);
					}
					catch (PlanesException & e)
					{
						network::bidirectionalmessages::ResourceStringMessage mess;
						mess.string = e.getString();
						auto & Client = server.getClient(clientID);
						Client.sendMessage(mess);
					}
				};

				void DestroyRoomRequest::handle()
				{
					try
					{
						server.destroyRoom(clientID);
					}
					catch (PlanesException & e)
					{
						network::bidirectionalmessages::ResourceStringMessage mess;
						mess.string = e.getString();
						auto & Client = server.getClient(clientID);
						Client.sendMessage(mess);
					}
				}

				void RoomListRequest::handle()
				{
					auto & Client = server.getClient(clientID);
					Client.profile();//just to check the client is in hangar
					network::servermessages::hangar::RoomList message;
					{
						MutexLocker ml(server.roomListMessage.mutex);
						message = server.roomListMessage.message;
					}
					Client.sendMessage(message);
				};

			}
			namespace unlogined
			{

				void Login::handle()
				{
					auto & Client = server.getClient(clientID);
					try
					{
						Client.login(name,encryptedPassword);
						servermessages::hangar::ServerConfiguration confMessage;
						confMessage.conf = configuration();
						Client.sendMessage(confMessage);
					}
					catch( PlanesException & e )
					{
						network::bidirectionalmessages::ResourceStringMessage mess;
						mess.string = e.getString();
						Client.sendMessage(mess);
						throw e;
					}
				}

				void Registry::handle()
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

			void TextMessage::handle()
			{
				std::wcout << _rstrw(":client{0}: {1}", clientID, text).str() << std::endl;
			}

			void ResourceStringMessage::handle()
			{
				std::wcout << _rstrw(":client{0}: {1}", clientID, string).str() << std::endl;
			}

			void ExitRoom::handle()
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