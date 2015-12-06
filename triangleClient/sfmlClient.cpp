#include "stdafx.h"
using boost::asio::ip::tcp;
using namespace rplanes::network;
std::string server_ip = "bulldozer";

float scale = 2.f; //увеличивает серверные данные при отрисовке

boost::asio::io_service io_service;

//особенности общения с сервером
//если в процессе общения с клиентом что-то идет не так, сервер обрывает соединение без передачи дополнительной информации
//соединение может быть оборвано:

//1)Если клиент находится в ангаре и отправил больше чем configuration().server.hangarMessagesPerFrame сообщений за configuration().server.hangarFrameTime
//2)Если клиент находится в комнате, и  отправляет больше чем configuration().server.roomMessagesPerFrame сообщений за configuration().server.roomFrameTime;
//3)Если клиент отправил сообщение, имея несоответствующий статус
//4)В случае возникновения любой ошибки у сервера, клиент удаляется


class Client
{
public:

	//сообщение, передаваемое серверу каждый серверный кадр
	rplanes::network::MSendControllable controllable;

	std::shared_ptr<rplanes::network::Connection> connection = std::make_shared<rplanes::network::Connection>
		(io_service, tcp::resolver(io_service).resolve(tcp::resolver::query(server_ip, "40000")));
	Client()
	{
		//////////////////////////////////////////////////////////////////////////
		//setting message handlers
		//////////////////////////////////////////////////////////////////////////

		//приходит после отправки запроса статуса
		connection->setHandler<MStatus>([this](const MStatus & m) {
			status = m.status;
		});
		//после успешной авторизации сервер отсылает конфигурацию, необходимую для корректной работы interpolate и т.п.
		connection->setHandler<MServerConfiguration>([this](const MServerConfiguration & m) {
			BOOST_LOG_TRIVIAL(info) << _rstrw("reading server configuration...").str();
			rplanes::configuration() = m.conf;
		});
		connection->setHandler<MProfile>([this](const MProfile & m) {
			profile = m.profile;
		});
		connection->setHandler<MRoomList>([this](const MRoomList & m) {
			rooms = m.rooms;
		});
		connection->setHandler<MChangeMap>([this](const MChangeMap & m) {
			BOOST_LOG_TRIVIAL(info) << _rstrw("changing map to {0}...", m.mapName).str();
		});
		//сообщение приходит после запроса серверного времени
		connection->setHandler<MServerTime>([this](const MServerTime & m) {
			client.serverTime = m.time;

			//запрашиваем серверное время
			client.connection->sendMessage(MServerTimeRequest());

			//клиент должен отсылать управление как можно чаще, но при этом не привышать квоту
			//поэтому отправляем данные управления вместе с запросом времени
			client.connection->sendMessage(client.controllable);
		});

		connection->setHandler<MCreateBullets>([this](const MCreateBullets & m) {
			for (auto & bullet : m.bullets)
			{
				client.bullets[bullet.ID] = bullet;
				//сдвигаем пулю на актуальную на данный момент позицию
				if (client.clientTime > m.time)
					client.bullets[bullet.ID].move(client.clientTime - m.time);
			}
		});
		connection->setHandler<MCreatePlanes>([this](const MCreatePlanes & m) {
			for (auto & plane : m.planes)
			{

				if (client.planes.count(plane.id) != 0)
				{
					BOOST_LOG_TRIVIAL(info) << _rstrw("duplicate plane creation").str();
				}
				BOOST_LOG_TRIVIAL(info) << _rstrw("creating new plane '{0}' player '{1}' id '{2}'",
					plane.planeName, plane.playerName, plane.id).str();
				client.planes[plane.id] = plane;
			}
		});
		//логически идентична createBullets
		//можно использовать другие графические эффекты, и т.п.
		connection->setHandler<MCreateRicochetes>([this](const MCreateRicochetes & m) {
			for (auto & bullet : m.bullets)
			{
				client.bullets[bullet.ID] = bullet;
				//сдвигаем пулю на актуальную на данный момент позицию
				if (client.clientTime > m.time)
					client.bullets[bullet.ID].move(client.clientTime - m.time);
			}
		});
		//приходит только при попаданиях. Остальные пули удаляются в главной петле
		connection->setHandler<MDestroyBullets>([this](const MDestroyBullets & m) {
			//пуля уничтожена во время коллизий
			for (auto & bullet : m.bullets)
			{
				client.bullets.erase(bullet.bulletID);
			}
		});
		//может означать не только уничтожение, но и вылет за границы видимости
		connection->setHandler<MDestroyPlanes>([this](const MDestroyPlanes & m) {
			for (auto & destroyedPlane : m.planes)
			{
				switch (destroyedPlane.reason)
				{
				case rplanes::network::MDestroyPlanes::MODULE_DESTROYED:
					BOOST_LOG_TRIVIAL(info) << _rstrw("plane {0} is downing because of {2} {1} destruction",
						destroyedPlane.planeID, destroyedPlane.moduleNo,
						rplanes::moduleTypesNames[client.planes[destroyedPlane.planeID].modules[destroyedPlane.moduleNo].type]).str();
					break;
				case rplanes::network::MDestroyPlanes::FUEL:
					BOOST_LOG_TRIVIAL(info) << _rstrw("plane {0} is out of fuel", destroyedPlane.planeID).str();
					break;
				case rplanes::network::MDestroyPlanes::VANISH:
					BOOST_LOG_TRIVIAL(info) << _rstrw("plane {0} vanished", destroyedPlane.planeID).str();
					break;
				case  rplanes::network::MDestroyPlanes::RAMMED:
					BOOST_LOG_TRIVIAL(info) << _rstrw("plane {0} rammed with another plane", destroyedPlane.planeID).str();
					break;
				case rplanes::network::MDestroyPlanes::FIRE:
					BOOST_LOG_TRIVIAL(info) << _rstrw("plane {0} is burning down", destroyedPlane.planeID).str();
					break;
				}
				client.planes.erase(destroyedPlane.planeID);
			}
		});

		//приходит каждый серверный кадр
		connection->setHandler<MInterfaceData>([this](const MInterfaceData & m) {
			client.interfaceData = m;
		});
		connection->setHandler<MPlanesPositions>([this](const MPlanesPositions & m) {
			for (auto & position : m.positions)
			{
				auto & plane = client.planes[position.planeID];
				plane.extrapolationData = position.extrapolationData;
				plane.pos = position.pos;
				//сдвигаем самолет на актуальную позицию
				plane.extrapolate(client.clientTime - m.time);
			}
		});
		connection->setHandler<MRoomInfo>([this](const MRoomInfo & m) {
			/*for (auto & player : newPlayers)
			{
			std::cout << player.name + " присоединился на самолете " + player.planeName + "." ;
			}
			for (auto & player : disconnectedPlayers)
			{
			std::cout << player + " покинул комнату." ;
			}
			std::cout << "Ваша текущая цель - " + goal.description + "." ;
			for (auto & stat : updatedStatistics)
			{
			std::cout
			<< "Статистика игрока "
			<< stat.first
			<< " обновлена. Убито : "
			<< stat.second.destroyed
			<< " умер : "
			<< stat.second.crashes
			<< " союзников уничтожено : "
			<< stat.second.friendsDestroyed
			;
			}*/
		});
		//приходит только при изменении параметров модулей
		connection->setHandler<MUpdateModules>([this](const MUpdateModules & m) {
			for (auto & module : m.modules)
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
				//	std::cout << " Модуль поврежден пожаром. ";
				//	break;
				//case rplanes::planedata::ModuleHP::HIT:
				//	std::cout << " Модуль поврежден пулей. ";
				//	break;
				//case rplanes::planedata::ModuleHP::REPAIR:
				//	std::cout << " Модуль восстановлен. ";
				//	break;
				//}

				//std::cout << "Номер модуля " << module.moduleNo << ". ";
				//auto moduleType = plane.modules[module.moduleNo].type;
				//std::cout << "Тип модуля " << moduleTypesNames[moduleType] << "." ;
			}
		});

		connection->setHandler<MText>([this](const MText & m) {
			BOOST_LOG_TRIVIAL(info) << _rstrw("server message : {0}", m.text).str();
		});
		connection->setHandler<MExitRoom>([this](const MExitRoom & m) {
			BOOST_LOG_TRIVIAL(info) << _rstrw("exited from room").str();
		});
		connection->setHandler<MResourceString>([this](const MResourceString & m) {
			BOOST_LOG_TRIVIAL(info) << _rstrw("server message : {0}", m.string).str();
		});

		connection->start();
	}

	//все следующие штуки мы получаем от сервера

	class Plane : public sf::Drawable, public rplanes::network::MCreatePlanes::Plane
	{
	public:
		Plane() :
			rplanes::network::MCreatePlanes::Plane()
		{}
		Plane(const rplanes::network::MCreatePlanes::Plane  & Plane) :
			rplanes::network::MCreatePlanes::Plane(Plane)
		{}
		void draw(sf::RenderTarget & target, sf::RenderStates states) const
		{
			sf::RectangleShape rectangle(sf::Vector2f(3.f, 3.f));
			switch (rplanes::network::MCreatePlanes::Plane::nation)
			{
			case rplanes::ALLIES:
				rectangle.setFillColor(sf::Color::Green);
				break;
			case rplanes::AXIS:
				rectangle.setFillColor(sf::Color::Red);
				break;
			default:
				break;
			}
			rectangle.setPosition(pos.x * scale + 10.f * scale, pos.y * scale);
			target.draw(rectangle);

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
				if (module.defected)
				{
					shape.setOutlineColor(sf::Color::Red);
				}
				else
				{
					shape.setOutlineColor(sf::Color::Green);
				}

				shape.setFillColor(sf::Color(255 * (1 - module.hp / module.hpMax), 255 * module.hp / module.hpMax, 0, 50));
				shape.setPosition(pos.x * scale, pos.y * scale);
				target.draw(shape);
			}
		}
	};
	class Bullet : public sf::Drawable, public rplanes::serverdata::Bullet
	{
	public:
		Bullet() :
			rplanes::serverdata::Bullet()
		{

		}
		Bullet(const rplanes::serverdata::Bullet & bullet) :
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
	//в данном клиенте ракеты игнорируются.


	//список видимых самолетов. изменяется сообщениями createPlanes destroyPlanes
	std::map<size_t, Plane> planes;
	//список пуль. Новые пули создаются в сообщении createBullets, при попаданиях пули удаляются сообщением destroyBullets, иначе в главной петле при излете
	std::map<size_t, Bullet> bullets;
	//статус клиента
	//	UNLOGINED - не прошел авторизацию, если в течении 1 секунды не придет легальное сообщение login, соединение будет оборвано
	//	HANGAR - клиент находится в ангаре. Может покупать самолеты и прочее, управлять комнатами и т.п.
	//находясь в ангаре клиент может отправлять запросы покупок и т.п. Чтобы увидеть результат этих действий, необходимо запросить профиль у сервера
	//	ROOM - клиент находится в комнате может отправлять данные управления, сообщение выхода из комнаты
	//Если клиент находясь, например, в ангаре попробует отправить данные управления, и в подобных случаях, соединение будет удалено без предупреждения
	//Изменять статус можно сообщениями login, joinRoom, exitRoom, logout
	//После того как было отправлено одно из этих сообщений, необходимо запросить статус у сервера
	rplanes::network::ClientStatus status;
	//показания приборов, уровень перегрузки и прочее
	rplanes::network::MInterfaceData interfaceData;
	//профиль игрока. Содержит список самолетов в ангаре, игровую статисктику и проч.
	rplanes::playerdata::Profile profile;

	//информация о комнатах
	typedef rplanes::network::MRoomList::RoomInfo RoomInfo;
	std::vector<RoomInfo> rooms;

	//время используется для синхронизации при получении некоторых сообщений
	//время, переданное сервером
	float serverTime;
	//время, используемое клиентом. Плавно корректироется c serverTime
	float clientTime;
}client;

//messages handlers





std::string password = "qwerty";


void loginAndJoinRoom(std::string profileName, size_t planeNo)
{
	std::chrono::milliseconds hangarFrameTime(static_cast<long long> (rplanes::configuration().server.hangarFrameTime * 1000));

	//заранее создаем сообщение авторизации
	rplanes::network::MLogin loginMessage;
	loginMessage.name = profileName;
	loginMessage.encryptedPassword = password;

	//отправляем сообщение авторизации
	BOOST_LOG_TRIVIAL(info) << _rstrw("sending auhtentication request...").str();
	client.connection->sendMessage(loginMessage);


	//ждем, чтобы не привысить квоту
	std::this_thread::sleep_for(hangarFrameTime);

	//если все прошло успешно, мы находимся в ангаре. Для проверки на acmd клиенте можно запросить у сервера статус.

	//запрашиваем список комнат
	client.connection->sendMessage(rplanes::network::MRoomListRequest());

	//ждем ответа
	size_t tries = 0;
	for (; tries < 10; tries++)
	{
		auto handledMessage = client.connection->handleMessage();
		if (handledMessage)
		{
			if (rplanes::network::MRoomList::id == handledMessage->getId())
			{
				break;
			}
		}
		std::this_thread::sleep_for(hangarFrameTime);
	}
	if (tries == 10)
	{
		BOOST_LOG_TRIVIAL(info) << _rstrw("something went wrong").str();
		return;
	}

	std::this_thread::sleep_for(hangarFrameTime);

	//имя игрока, к комнате которого мы присоединимся
	std::string roomCreatorName;

	//если список комнат пуст, создаем свою. Имя игрока может оставаться пустым.
	if (client.rooms.size() == 0)
	{
		rplanes::network::MCreateRoomRequest crr;
		crr.description = "the best map on the server";
		crr.mapName = "bot.map";
		client.connection->sendMessage(crr);
		std::this_thread::sleep_for(hangarFrameTime);
	}
	//иначе запоминаем имя владельца первой комнаты
	else
	{
		roomCreatorName = client.rooms.front().creatorName;
	}

	//считаем что все прошло успешно


	//перед отправкой запроса присоединения к комнате создаем окно и прочее.
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

	sf::Vector2i mouseCenter(1366 / 2, 768 / 2);

	sf::RectangleShape  faintRECTALgle(sf::Vector2f(5000, 5000));
	sf::CircleShape aimCircle;
	aimCircle.setFillColor(sf::Color(0, 0, 0, 0));
	aimCircle.setOutlineThickness(1);
	aimCircle.setOutlineColor(sf::Color::Red);


	//подключаемся к первой комнате
	rplanes::network::MJoinRoomRequest jrr;
	jrr.planeNo = planeNo;
	jrr.ownerName = roomCreatorName;

	client.connection->sendMessage(jrr);

	//запрашиваем серверное время. Последующие запросы будут отправлены из обработчика ServerTime
	client.connection->sendMessage(rplanes::network::MServerTimeRequest());

	//есть ли фокус на окне
	bool focusPocus = true;

	//graphic loop
	while (window.isOpen())
	{
		iterationBegin = std::chrono::steady_clock::now();
		//увеличиваем время
		client.serverTime += frameTime.count();
		client.clientTime += frameTime.count();

		//плавно корректируем клиентское время
		client.clientTime += (client.serverTime - client.clientTime) * frameTime.count();
		if (std::abs(client.clientTime - client.serverTime) > 1.0)
		{
			client.clientTime = client.serverTime;
		}

		//экстраполируем
		for (auto & plane : client.planes)
		{
			plane.second.extrapolate(frameTime.count());
		}

		//сдвигаем пули и удаляем пули на излете
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

		//считываем управление
		sf::Event event;
		client.controllable.shootingDistanceOffset = 0;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape))
				window.close();
			if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::R))
			{
				rplanes::network::MAdministerRoom ar;
				ar.operation = rplanes::network::MAdministerRoom::RESTART;
				client.connection->sendMessage(ar);
				BOOST_LOG_TRIVIAL(info) << _rstrw("sending restart command to the server").str();
			}
			if (event.type == sf::Event::MouseButtonPressed)
			{
				client.controllable.isShooting = true;
			}
			if (event.type == sf::Event::MouseButtonReleased)
			{
				client.controllable.isShooting = false;
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
		if (focusPocus)
		{
			client.controllable.turningVal += (sf::Mouse::getPosition(window).x - mouseCenter.x) / 5.f;
			if (client.controllable.turningVal > 100)
			{
				client.controllable.turningVal = 100;
			}
			if (client.controllable.turningVal < -100)
			{
				client.controllable.turningVal = -100;
			}
			client.controllable.shootingDistanceOffset = -(sf::Mouse::getPosition(window).y - mouseCenter.y) / 2.5f;

			sf::Mouse::setPosition(mouseCenter, window);
		}

		//управление мощностью двигателя
		{

			auto & controllable = client.controllable;
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
				//	<< controllable.power ;
			}

			if (!isAbleToIncreasePower || sf::Keyboard::isKeyPressed(sf::Keyboard::S))
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


		//принимаем серверные сообщения. Будут созданы новые пули, самолеты, и обновлено состояние самолетов
		while (client.connection->handleMessage())
		{
		};

		//вращаем зоны повреждения
		for (auto & plane : client.planes)
		{
			for (auto & module : plane.second.modules)
			{
				module.hitZone.spin(plane.second.pos.angle, plane.second.pos.roll);
			}
		}

		//рисуем сетку
		for (auto & cell : grid)
		{
			window.draw(cell);
		}
		//рисуем самолеты
		for (auto & plane : client.planes)
		{
			//центруем камеру по нашему самолету и рисуем интерфейс
			if (plane.second.playerName == profileName)
			{
				camera.setCenter(plane.second.pos.x * scale, plane.second.pos.y * scale);
				camera.setRotation(plane.second.pos.angle + 90);

				//слайдер поворота
				sf::Vector2f sliderPos;
				sliderPos.x = plane.second.pos.x
					+ std::cos((plane.second.pos.angle + client.controllable.turningVal * 0.3f) / 180.f * M_PI) * client.interfaceData.shootingDistance;
				sliderPos.y = plane.second.pos.y
					+ std::sin((plane.second.pos.angle + client.controllable.turningVal * 0.3f) / 180.f * M_PI) * client.interfaceData.shootingDistance;

				turningSlider.setPosition(sliderPos.x * scale - 5,
					sliderPos.y * scale - 5);

				//затемнение экрана при перегрузках
				faintRECTALgle.setPosition(plane.second.pos.x * scale - 400, plane.second.pos.y * scale - 300);

				//прицел
				//по хорошему прицел должен сдвигаться относительно пушек, а не центра самолета
				//но это бы запутало и так говновый кот
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
		//рисуем пули
		for (auto & bullet : client.bullets)
		{
			window.draw(bullet.second);
		}
		window.setView(camera);

		//обморочное затемнение экрана
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
	//заранее создаем сообщение регистрации
	rplanes::network::MRegistry registryMessage;
	registryMessage.name = profileName;
	registryMessage.password = password;

	//отправляем сообщение
	client.connection->sendMessage(registryMessage);
	//обрабатываем вывод сервера
	while (client.connection->handleMessage())
	{
	}
}

void loginAndBuyPlane(std::string profileName, std::string planeName)
{
	//заранее создаем сообщение авторизации
	rplanes::network::MLogin loginMessage;
	loginMessage.name = profileName;
	loginMessage.encryptedPassword = password;

	//отправляем сообщение авторизации
	BOOST_LOG_TRIVIAL(info) << _rstrw("sending authentication request..").str();
	client.connection->sendMessage(loginMessage);
	//ждем секунду, чтобы не привысить квоту
	std::this_thread::sleep_for(std::chrono::seconds(1));

	//по-хорошему нужно запросить у сервера статус клиента

	//теперь мы в ангаре, купим самолет

	client.connection->sendMessage(rplanes::network::MProfileRequest());
	//если авторизация не прошла успешно, сервер оборвет подклбчение.

	//ждем ответа сервера
	size_t tries = 0;
	for (; tries < 3; tries++)
	{
		auto handledMessage = client.connection->handleMessage();
		if (handledMessage)
		{
			if (rplanes::network::MProfile::id == handledMessage->getId())
			{
				BOOST_LOG_TRIVIAL(info) << _rstrw("authentication succeeded").str();
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	if (tries == 3)
	{
		BOOST_LOG_TRIVIAL(info) << _rstrw("authentication failed").str();
		return;
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));
	rplanes::network::MBuyPlaneRequest bpr;
	bpr.planeName = planeName;
	BOOST_LOG_TRIVIAL(info) << _rstrw("sending buying request").str();
	client.connection->sendMessage(bpr);
	//обрабатываем вывод сервера
	while (true)
	{
		while (client.connection->handleMessage());
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void failedToMakeUpGoodName(int argc, char* argv[])
{
	if (argc < 2)
	{
		BOOST_LOG_TRIVIAL(info) << _rstrw("wrong input data").str();
		return;
	}

	std::string command(argv[1]);

	try
	{
		if (command == "join")
		{
			if (argc != 4 && argc != 5)
			{
				BOOST_LOG_TRIVIAL(info) << _rstrw("wrong input data").str();
				return;
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
				BOOST_LOG_TRIVIAL(info) << _rstrw("wrong input data").str();
				return;
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
				BOOST_LOG_TRIVIAL(info) << _rstrw("wrong input data").str();
				return;
			}
			if (argc == 5)
			{
				server_ip = argv[4];
			}
			loginAndBuyPlane(argv[2], argv[3]);
		}
		else
		{
			BOOST_LOG_TRIVIAL(info) << _rstrw("wrong input data").str();
		}
	}
	catch (rplanes::PlanesException & e)
	{
		BOOST_LOG_TRIVIAL(error) << e.getString().str();
	}
	catch (std::exception & e)
	{
		BOOST_LOG_TRIVIAL(error) << e.what();
	}
}

int main(int argc, char* argv[])
{
#ifdef _MSC_VER
	system("chcp 65001");
#endif
	boost::locale::generator gen;
	std::locale::global(gen.generate(std::locale(), ""));
	BOOST_LOG_TRIVIAL(info).imbue(std::locale());

	std::wifstream xmlFs(rplanes::configuration().server.languageXml);
	boost::archive::xml_wiarchive archive(xmlFs, boost::archive::no_header);
	archive >> boost::serialization::make_nvp("strings", rstring::_rstrw_t::resource());

	omp_set_num_threads(2);
	omp_set_nested(true);

#pragma omp parallel sections
	{
#pragma omp section
		{
			io_service.run();
		}
#pragma omp section
		{
			failedToMakeUpGoodName(argc, argv);
			io_service.stop();
		}
	}

	return 0;
}
