#define PLANES_CLIENT //������ ������ �� messages.h
#include "stdafx.h"
using boost::asio::ip::tcp;

std::string server_ip = "127.0.0.1";

float scale = 2.f; //����������� ��������� ������ ��� ���������

boost::asio::io_service io_service;

//����������� ������� � ��������
//���� � �������� ������� � �������� ���-�� ���� �� ���, ������ �������� ���������� ��� �������� �������������� ����������
//���������� ����� ���� ��������:

//1)���� ������ ��������� � ������ � �������� ������ ��� configuration().server.hangarMessagesPerFrame ��������� �� configuration().server.hangarFrameTime
//2)���� ������ ��������� � �������, �  ���������� ������ ��� configuration().server.roomMessagesPerFrame ��������� �� configuration().server.roomFrameTime;
//3)���� ������ �������� ���������, ���� ����������������� ������
//4)� ������ ������������� ����� ������ � �������, ������ ���������


class Client
{
public:

	//���������, ������������ ������� ������ ��������� ����
	rplanes::network::clientmessages::room::SendControllable controllable;

	rplanes::network::Connection connection;
	Client() : connection(io_service)
	{}

	void connect()
	{
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(server_ip, "40000");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		connection.connect(endpoint_iterator);
	}

	//��� ��������� ����� �� �������� �� �������

	class plane : public sf::Drawable, public rplanes::network::servermessages::room::Plane
	{
	public:
		plane() :
			rplanes::network::servermessages::room::Plane()
		{}
		plane(const rplanes::network::servermessages::room::Plane  & Plane) :
			rplanes::network::servermessages::room::Plane(Plane)
		{}
		void draw(sf::RenderTarget & target, sf::RenderStates states) const
		{
			sf::ConvexShape shape;
			for (auto & module : modules)
			{
				shape.setPointCount(static_cast<unsigned int>(module.hitZone.shape.points.size()));
				unsigned int i = 0;
				for (auto & Point : module.hitZone.shape.points)
				{
					shape.setPoint(i, sf::Vector2f(Point.x *scale, Point.y * scale));
					i++;
				}
				shape.setOutlineThickness(1);
				//if (module.defected)
				//{
				//	shape.setOutlineColor(sf::Color::Red);
				//}
				//else
				//{
				//	shape.setOutlineColor(sf::Color::Green);
				//}
				
				switch ( rplanes::network::servermessages::room::Plane::nation)
				{
				case rplanes::ALLIES:
					shape.setOutlineColor(sf::Color::Green);
					break;
				case rplanes::AXIS:
					shape.setOutlineColor(sf::Color::Red);
					break;
				default:
					break;
				}
				shape.setFillColor(sf::Color(255 * (1 - module.hp / module.hpMax), 255 * module.hp / module.hpMax, 0, 50));
				shape.setPosition(pos.x * scale, pos.y * scale);
				target.draw(shape);
			}
		}
	};
	class bullet : public sf::Drawable, public rplanes::serverdata::Bullet
	{
	public:
		bullet() :
			rplanes::serverdata::Bullet()
		{

			}
		bullet(const rplanes::serverdata::Bullet & bullet) :
			rplanes::serverdata::Bullet(bullet)
		{}
		void draw(sf::RenderTarget & target, sf::RenderStates states) const
		{
			sf::CircleShape circle;


			float size = 2.f;

			circle.setRadius(size);
			if (z > 0)
			{
				circle.setFillColor(sf::Color::Red);
			}
			else
			{
				circle.setFillColor(sf::Color::Yellow);
			}
			circle.setPosition(x * scale - size, y * scale - size);
			target.draw(circle);
		}
	};
	//� ������ ������� ������ ������������.


	//������ ������� ���������. ���������� ����������� createPlanes destroyPlanes
	std::map<size_t, plane> planes;
	//������ ����. ����� ���� ��������� � ��������� createBullets, ��� ���������� ���� ��������� ���������� destroyBullets, ����� � ������� ����� ��� ������
	std::map<size_t, bullet> bullets;
	//������ �������
	//	UNLOGINED - �� ������ �����������, ���� � ������� 1 ������� �� ������ ��������� ��������� login, ���������� ����� ��������
	//	HANGAR - ������ ��������� � ������. ����� �������� �������� � ������, ��������� ��������� � �.�.
	//�������� � ������ ������ ����� ���������� ������� ������� � �.�. ����� ������� ��������� ���� ��������, ���������� ��������� ������� � �������
	//	ROOM - ������ ��������� � ������� ����� ���������� ������ ����������, ��������� ������ �� �������
	//���� ������ ��������, ��������, � ������ ��������� ��������� ������ ����������, � � �������� �������, ���������� ����� ������� ��� ��������������
	//�������� ������ ����� ����������� login, joinRoom, exitRoom, logout
	//����� ���� ��� ���� ���������� ���� �� ���� ���������, ���������� ��������� ������ � �������
	rplanes::network::ClientStatus status;
	//��������� ��������, ������� ���������� � ������
	rplanes::network::servermessages::room::InterfaceData interfaceData;
	//������� ������. �������� ������ ��������� � ������, ������� ����������� � ����.
	rplanes::playerdata::Profile profile;

	//���������� � ��������
	typedef rplanes::network::servermessages::hangar::RoomList::RoomInfo RoomInfo;
	std::vector<RoomInfo> rooms;

	//����� ������������ ��� ������������� ��� ��������� ��������� ���������
	//�����, ���������� ��������
	float serverTime;
	//�����, ������������ ��������. ������ �������������� c serverTime
	float clientTime;
}client;

//messages handlers
namespace rplanes
{
	namespace network
	{
		namespace servermessages
		{
			//�������� ����� �������� ������� �������
			void StatusMessage::handle()
			{
				client.status = status;

			}

			//��������� ������������ �������, ������������ � ������
			namespace hangar
			{
				//����� �������� ����������� ������ �������� ������������, ����������� ��� ���������� ������ interpolate � �.�.
				void ServerConfiguration::handle()
				{
					std::cout << "�������� ������������ ������� " << std::endl;
					rplanes::configuration() = conf;
				}
				void SendProfile::handle()
				{
					client.profile = profile;
				}
				void RoomList::handle()
				{
					client.rooms = rooms;
				}

			}

			//��������� ������������ �������, ������������ � �������
			namespace room
			{

				void ChangeMap::handle()
				{
					std::cout << "����� ������� �� " << mapName << std::endl;
				}

				//��������� �������� ����� ������� ���������� �������
				void ServerTime::handle()
				{
					client.serverTime = time;

					//����������� ��������� �����
					client.connection.sendMessage(clientmessages::room::ServerTimeRequest());

					//������ ������ �������� ���������� ��� ����� ����, �� ��� ���� �� ��������� �����
					//������� ���������� ������ ���������� ������ � �������� �������
					client.connection.sendMessage(client.controllable);
				}

				void CreateBullets::handle()
				{
					for (auto & bullet : bullets)
					{
						//�������� ���� �� ���������� �� ������ ������ �������
						if (client.clientTime > time)
							bullet.move(client.clientTime - time);
						client.bullets[bullet.ID] = bullet;
					}
				}

				//������ ������ ���������� ������
				void �reateMissiles::handle()
				{
				}

				void CreatePlanes::handle()
				{
					for (auto & plane : Planes)
					{

						if (client.planes.count(plane.id) != 0)
						{
							std::cout << "��������� �������� ��������" << std::endl;
						}
						std::cout << " �������� ����� ������� " << " "
							<< plane.planeName << " "
							<< plane.playerName << " "
							<< plane.id << std::endl;
						client.planes[plane.id] = plane;
					}

				}

				//��������� ��������� createBullets
				//����� ������������ ������ ����������� �������, � �.�.
				void �reateRicochetes::handle()
				{
					for (auto & bullet : bullets)
					{
						if (client.clientTime > time)
							bullet.move(client.clientTime - time);
						client.bullets[bullet.ID] = bullet;
					}
				}

				//�������� ������ ��� ����������. ��������� ���� ��������� � ������� �����
				void DestroyBullets::handle()
				{
					//���� ���������� �� ����� ��������
					for (auto & bullet : bullets)
					{
						client.bullets.erase(bullet.bulletID);
					}
				}

				void DestroyMissiles::handle()
				{
				}

				//����� �������� �� ������ �����������, �� � ����� �� ������� ���������
				void DestroyPlanes::handle()
				{
					for (auto & destroyedPlane : planes)
					{
						switch (destroyedPlane.reason)
						{
						case rplanes::network::servermessages::room::DestroyPlanes::MODULE_DESTROYED:
							std::cout << "�������" << destroyedPlane.planeID << " ��������� �� ������� ������ " << destroyedPlane.moduleNo 
								<< " " << rplanes::moduleTypesNames[ client.planes[destroyedPlane.planeID].modules[destroyedPlane.moduleNo].type ] << std::endl;
							break;
						case rplanes::network::servermessages::room::DestroyPlanes::FUEL:
							std::cout << "�������" << destroyedPlane.planeID << " ���������, ����������� ������� " << std::endl;
							break;
						case rplanes::network::servermessages::room::DestroyPlanes::VANISH:
							std::cout << "������� " << destroyedPlane.planeID << " �����!" << std::endl;
							break;
						case  rplanes::network::servermessages::room::DestroyPlanes::RAMMED:
							std::cout << "������� ��������� ������� " << destroyedPlane.planeID << std::endl;
							break;
						case rplanes::network::servermessages::room::DestroyPlanes::FIRE:
							std::cout << "������� ��������� ������� " << destroyedPlane.planeID << std::endl;
							break;
						}
						client.planes.erase(destroyedPlane.planeID);
					}
				}

				//�������� ������ ��������� ����
				void InterfaceData::handle()
				{
					client.interfaceData = *this;
				}

				//�������� ������ ��������� ����
				void SetPlanesPositions::handle()
				{
					for (auto & position : positions)
					{
						auto & plane = client.planes[position.planeID];
						plane.extrapolationData = position.extrapolationData;
						plane.pos = position.pos;
						//�������� ������� �� ���������� �������
						plane.extrapolate(client.clientTime - time);
					}
				}

				//�������� ������ ��� ��������� ���������� �������
				void UpdateModules::handle()
				{
					for (auto & module : modules)
					{
						if (client.planes.count(module.planeID) == 0)
						{
							continue;
						}
						auto & plane = client.planes[module.planeID];
						plane.modules[module.moduleNo].defected = module.defect;
						plane.modules[module.moduleNo].hp = module.hp;
						//switch (module.reason)
						//{
						//case rplanes::planedata::ModuleHP::FIRE:
						//	std::cout << " ������ ��������� �������. ";
						//	break;
						//case rplanes::planedata::ModuleHP::HIT:
						//	std::cout << " ������ ��������� �����. ";
						//	break;
						//case rplanes::planedata::ModuleHP::REPAIR:
						//	std::cout << " ������ ������������. ";
						//	break;
						//}

						//std::cout << "����� ������ " << module.moduleNo << ". ";
						//auto moduleType = plane.modules[module.moduleNo].type;
						//std::cout << "��� ������ " << moduleTypesNames[moduleType] << "." << std::endl;
					}
				}


				void RoomInfo::handle()
				{
					/*for (auto & player : newPlayers)
					{
						std::cout << player.name + " ������������� �� �������� " + player.planeName + "." << std::endl;
					}
					for (auto & player : disconnectedPlayers)
					{
						std::cout << player + " ������� �������." << std::endl;
					}
					std::cout << "���� ������� ���� - " + goal.description + "." << std::endl;
					for (auto & stat : updatedStatistics)
					{
						std::cout
							<< "���������� ������ "
							<< stat.first
							<< " ���������. ����� : "
							<< stat.second.destroyed
							<< " ���� : "
							<< stat.second.crashes
							<< " ��������� ���������� : "
							<< stat.second.friendsDestroyed
							<< std::endl;
					}*/
				}
			}
		}

		namespace bidirectionalmessages
		{
			void TextMessage::handle()
			{
				std::cout << "������  ����� " << text << std::endl;
			}
			void ExitRoom::handle()
			{
				std::cout << " ������ �������� �� ������� " << std::endl;
			}
		}
	}
}

std::string password = "qwerty";


void loginAndJoinRoom(std::string profileName, size_t planeNo, bool bot = false)
{
	std::chrono::milliseconds hangarFrameTime(static_cast<long long> (rplanes::configuration().server.hangarFrameTime * 1000));

	//������� ������� ��������� �����������
	rplanes::network::clientmessages::unlogined::Login loginMessage;
	loginMessage.name = profileName;
	loginMessage.encryptedPassword = password;

	//������������ � �������
	client.connect();
	client.connection.non_blocking(true);

	//���������� ��������� �����������
	client.connection.sendMessage(loginMessage);
	std::cout << "��������� ������ �����������" << std::endl;

	//����, ����� �� ��������� �����
	std::this_thread::sleep_for(hangarFrameTime);

	//���� ��� ������ �������, �� ��������� � ������. ��� �������� �� acmd ������� ����� ��������� � ������� ������.

	//����������� ������ ������
	client.connection.sendMessage(rplanes::network::clientmessages::hangar::RoomListRequest());

	//���� ������
	size_t tries = 0;
	for (; tries < 10; tries++)
	{
		if (client.connection.handleInput())
		{
			if (rplanes::network::servermessages::hangar::RoomList().getId() == client.connection.getLastMessageId())
			{
				break;
			}
		}
		std::this_thread::sleep_for(hangarFrameTime);
	}
	if (tries == 10)
	{
		std::cout << "����� �������" << std::endl;
		return;
	}

	std::this_thread::sleep_for(hangarFrameTime);

	//��� ������, � ������� �������� �� �������������
	std::string roomCreatorName;

	//���� ������ ������ ����, ������� ����. ��� ������ ����� ���������� ������.
	if (client.rooms.size() == 0)
	{
		rplanes::network::clientmessages::hangar::CreateRoomRequest crr;
		crr.description = "����� �� �������, ���� �����!";
		crr.mapName = "bot.map";
		client.connection.sendMessage(crr);
		std::this_thread::sleep_for(hangarFrameTime);
	}
	//����� ���������� ��� ��������� ������ �������
	else
	{
		roomCreatorName = client.rooms.front().creatorName;
	}

	//������� ��� ��� ������ �������


	//����� ��������� ������� ������������� � ������� ������� ���� � ������.
	sf::RenderWindow window(sf::VideoMode(1366, 768), "Sudden poligons!");

	window.setFramerateLimit(25);
	std::chrono::duration<float> frameTime;
	std::chrono::steady_clock::time_point iterationBegin;
	sf::View camera;
	camera.setSize(1366, 768);

	std::vector<sf::RectangleShape> gridBase, grid;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			gridBase.push_back(sf::RectangleShape(sf::Vector2f(1000.f, 1000.f)));
			gridBase.back().setPosition(i * 1000.f, j * 1000.f);
			gridBase.back().setOutlineColor(sf::Color::Green);
			gridBase.back().setOutlineThickness(1);
		}
	}
	grid = gridBase;


	sf::CircleShape turningSlider;
	turningSlider.setRadius(5.f);
	turningSlider.setFillColor(sf::Color::Yellow);
	turningSlider.setOutlineColor(sf::Color::Black);
	turningSlider.setOutlineThickness(1);

	sf::Vector2i mouseCenter(1366/2, 768/2);

	sf::RectangleShape  faintRECTALgle(sf::Vector2f(5000, 5000));
	sf::CircleShape aimCircle;
	aimCircle.setFillColor(sf::Color(0, 0, 0, 0));
	aimCircle.setOutlineThickness(1);
	aimCircle.setOutlineColor(sf::Color::Red);


	//������������ � ������ �������
	rplanes::network::clientmessages::hangar::JoinRoomRequest jrr;
	jrr.planeNo = planeNo;
	jrr.playerName = roomCreatorName;

	client.connection.sendMessage(jrr);

	//����������� ��������� �����. ����������� ������� ����� ���������� �� ����������� ServerTime
	client.connection.sendMessage(rplanes::network::clientmessages::room::ServerTimeRequest());

	//���� �� ����� �� ����
	bool focusPocus = true;

	//graphic loop
	while (window.isOpen())
	{
		iterationBegin = std::chrono::steady_clock::now();
		//����������� �����
		client.serverTime += frameTime.count();
		client.clientTime += frameTime.count();

		//������ ������������ ���������� �����
		client.clientTime += (client.serverTime - client.clientTime) * frameTime.count();
		if (std::abs(client.clientTime - client.serverTime) > 1.0)
		{
			client.clientTime = client.serverTime;
		}

		//��������������
		for (auto & plane : client.planes)
		{
			plane.second.extrapolate(frameTime.count());
		}

		//�������� ���� � ������� ���� �� ������
		std::vector<size_t> erasedBulletsKeys;
		for (auto & bullet : client.bullets)
		{
			bullet.second.move(frameTime.count());
			if (bullet.second.isSpent())
			{
				erasedBulletsKeys.push_back(bullet.first);
			}
		}
		for (auto key : erasedBulletsKeys)
		{
			client.bullets.erase(key);
		}

		//��������� ����������
		sf::Event event;
		client.controllable.params.shootingDistanceOffset = 0;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape))
				window.close();
			if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::R))
			{
				rplanes::network::clientmessages::room::AdministerRoom ar;
				ar.operation = rplanes::network::clientmessages::room::AdministerRoom::RESTART;
				client.connection.sendMessage(ar);
				std::cout << " ������� ���������� ������� ��������." << std::endl;
			}
			if (!bot)
			{
				if (event.type == sf::Event::MouseButtonPressed)
				{
					client.controllable.params.isShooting = true;
				}
				if (event.type == sf::Event::MouseButtonReleased)
				{
					client.controllable.params.isShooting = false;
				}
				if (event.type == sf::Event::GainedFocus)
				{
					focusPocus = true;
				}
				if (event.type == sf::Event::LostFocus)
				{
					focusPocus = false;
				}


				if (event.type == sf::Event::MouseWheelMoved)
				{
					auto factor = std::pow(1.2, event.mouseWheel.delta);
					scale *= factor;
					if (scale < 0.2f)
					{
						scale = 0.2f;
					}
					grid = gridBase;
					for (auto & cell : grid)
					{
						cell.setScale(scale, scale);
						auto position = cell.getPosition();
						position *= scale;
						cell.setPosition(position);
					}

				}
			}
		}
		if (!bot && focusPocus)
		{
			client.controllable.params.turningVal += (sf::Mouse::getPosition(window).x - mouseCenter.x) / 5.f;
			if (client.controllable.params.turningVal > 100)
			{
				client.controllable.params.turningVal = 100;
			}
			if (client.controllable.params.turningVal < -100)
			{
				client.controllable.params.turningVal = -100;
			}
			client.controllable.params.shootingDistanceOffset = -(sf::Mouse::getPosition(window).y - mouseCenter.y) / 2.5f;

			sf::Mouse::setPosition(mouseCenter, window);
		}
		if (bot)
		{
			client.controllable.params.turningVal = 40;
			client.controllable.params.isShooting = true;
			client.controllable.params.shootingDistanceOffset = (0.5f - static_cast<float>(rand()) / RAND_MAX) * 10.f;
		}

		//���������� ��������� ���������
		{

			auto & controllable = client.controllable.params;
			auto & interfaceData = client.interfaceData;
			bool isAbleToIncreasePower = true;
			for (auto & thermometer : interfaceData.thermometers)
			{
				if (thermometer.temperature > 0.8 * thermometer.criticalTemperature
					|| thermometer.dT > 0.8 * thermometer.dTmax)
				{
					isAbleToIncreasePower = false;
				}
				//std::cout << thermometer.temperature << " "
				//	<< thermometer.criticalTemperature << " "
				//	<< thermometer.dT << " "
				//	<< thermometer.dTmax << " "
				//	<< controllable.power << std::endl;
			}

			if ( !isAbleToIncreasePower || sf::Keyboard::isKeyPressed(sf::Keyboard::S) )
			{
				if (controllable.power != 0)
				{
					controllable.power--;
				}
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			{
				controllable.power++;
			}

			if (controllable.power > 100)
			{
				controllable.power = 100;
			}

		}


		//��������� ��������� ���������. ����� ������� ����� ����, ��������, � ��������� ��������� ���������
		while (client.connection.handleInput())
		{
		};

		//������� ���� �����������
		for (auto & plane : client.planes)
		{
			for (auto & module : plane.second.modules)
			{
				module.hitZone.spin(plane.second.pos.angle, plane.second.pos.roll);
			}
		}

		//������ �����
		for (auto & cell : grid)
		{
			window.draw(cell);
		}
		//������ ��������
		for (auto & plane : client.planes)
		{
			//�������� ������ �� ������ �������� � ������ ���������
			if (plane.second.playerName == profileName)
			{
				camera.setCenter(plane.second.pos.x * scale, plane.second.pos.y * scale);
				camera.setRotation(plane.second.pos.angle + 90);

				//������� ��������
				sf::Vector2f sliderPos;
				sliderPos.x = plane.second.pos.x
					+ std::cos((plane.second.pos.angle + client.controllable.params.turningVal * 0.3f) / 180.f * M_PI) * client.interfaceData.shootingDistance;
				sliderPos.y = plane.second.pos.y
					+ std::sin((plane.second.pos.angle + client.controllable.params.turningVal * 0.3f) / 180.f * M_PI) * client.interfaceData.shootingDistance;

				turningSlider.setPosition(sliderPos.x * scale - 5,
					sliderPos.y * scale - 5);

				//���������� ������ ��� �����������
				faintRECTALgle.setPosition(plane.second.pos.x * scale - 400, plane.second.pos.y * scale - 300);

				//������
				//�� �������� ������ ������ ���������� ������������ �����, � �� ������ ��������
				//�� ��� �� �������� � ��� �������� ���
				sf::Vector2f aimPos;
				aimPos.x = plane.second.pos.x
					+ std::cos(plane.second.pos.angle / 180.f * M_PI) * client.interfaceData.shootingDistance;
				aimPos.y = plane.second.pos.y
					+ std::sin(plane.second.pos.angle / 180.f * M_PI) * client.interfaceData.shootingDistance;
				aimCircle.setRadius(client.interfaceData.aimSize * scale);
				aimCircle.setPosition((aimPos.x - client.interfaceData.aimSize) * scale,
					(aimPos.y - client.interfaceData.aimSize) * scale);
			}
			window.draw(plane.second);
		}
		window.draw(turningSlider);
		window.draw(aimCircle);
		//������ ����
		for (auto & bullet : client.bullets)
		{
			window.draw(bullet.second);
		}
		window.setView(camera);

		//���������� ���������� ������
		unsigned int fv = std::pow(client.interfaceData.faintVal / 100.f, 4.0) * 255;
		if (fv > 255)
		{
			fv = 255;
		}
		faintRECTALgle.setFillColor(sf::Color(0, 0, 0, fv));
		window.draw(faintRECTALgle);

		window.display();
		window.clear(sf::Color::White);
		frameTime = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - iterationBegin);
	}
}

void registry(std::string profileName)
{
	//������� ������� ��������� �����������
	rplanes::network::clientmessages::unlogined::Registry registryMessage;
	registryMessage.name = profileName;
	registryMessage.password = password;

	//������������ � �������
	client.connect();
	//���������� ���������
	client.connection.sendMessage(registryMessage);
	//������������ ����� �������
	while (client.connection.handleInput())
	{
	}
}

void loginAndBuyPlane(std::string profileName, std::string planeName)
{
	//������� ������� ��������� �����������
	rplanes::network::clientmessages::unlogined::Login loginMessage;
	loginMessage.name = profileName;
	loginMessage.encryptedPassword = password;

	//������������ � �������
	client.connect();
	client.connection.non_blocking(true);


	//���������� ��������� �����������
	client.connection.sendMessage(loginMessage);
	std::cout << "��������� ������ �����������" << std::endl;
	//���� �������, ����� �� ��������� �����
	std::this_thread::sleep_for(std::chrono::seconds(1));

	//���������� ����� ��������� � ������� ������ �������

	//������ �� � ������, ����� �������

	client.connection.sendMessage(rplanes::network::clientmessages::hangar::ProfileRequest());
	//���� ����������� �� ������ �������, ������ ������� �����������.

	//���� ������ �������
	size_t tries = 0;
	for (; tries < 3; tries++)
	{
		if (client.connection.handleInput())
		{
			if (rplanes::network::servermessages::hangar::SendProfile().getId() == client.connection.getLastMessageId())
			{
				std::cout << "����������� ������ �������" << std::endl;
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	if (tries == 10)
	{
		std::cout << "����� �������" << std::endl;
		return;
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));
	rplanes::network::clientmessages::hangar::BuyPlaneRequest bpr;
	bpr.planeName = planeName;
	client.connection.sendMessage(bpr);
	std::cout << "��������� ������ ������� �������� " << std::endl;
	//������������ ����� �������
	while (true)
	{
		client.connection.handleInput();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}


int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "rus");

	if (argc < 2)
	{
		std::cout << "������ ������� ������� " << std::endl;
		return 0;
	}

	std::string command(argv[1]);

	try
	{
		if (command == "bot")
		{
			if (argc != 4 && argc != 5)
			{
				std::cout << "������ ������� ������� " << std::endl;
				return 0;
			}
			if (argc == 5)
			{
				server_ip = argv[4];
			}
			std::stringstream ss;
			ss << argv[3];
			size_t planeNum;
			ss >> planeNum;
			loginAndJoinRoom(argv[2], planeNum, true);
		}
		else if (command == "join")
		{
			if (argc != 4 && argc != 5)
			{
				std::cout << "������ ������� ������� " << std::endl;
				return 0;
			}
			if (argc == 5)
			{
				server_ip = argv[4];
			}
			std::stringstream ss;
			ss << argv[3];
			size_t planeNum;
			ss >> planeNum;
			loginAndJoinRoom(argv[2], planeNum);
		}
		else if (command == "reg")
		{
			if (argc != 3 && argc != 4)
			{
				std::cout << "������ ������� ������� " << std::endl;
				return 0;
			}
			if (argc == 4)
			{
				server_ip = argv[3];
			}
			registry(argv[2]);
		}
		else if (command == "buy")
		{
			if (argc != 4 && argc != 5)
			{
				std::cout << "������ ������� ������� " << std::endl;
				return 0;
			}
			if (argc == 5)
			{
				server_ip = argv[4];
			}
			loginAndBuyPlane(argv[2], argv[3]);
		}
		else
		{
			std::cout << "������ ������� ������� " << std::endl;
		}
	}
	catch (std::exception & e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
