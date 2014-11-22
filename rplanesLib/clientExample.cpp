#define PLANES_CLIENT //должно стоять до messages.h
#include <messages.h>
#include <database.h>
#include <exceptions.h>
#include <omp.h>
using boost::asio::ip::tcp;


//messages handlers
//напиши их!
namespace roman
{
	namespace network
	{
		namespace serverMessages
		{
			namespace combat
			{
				void createBullets::handle()
				{
					//обработчик сообщения, содержащего всякие там ништяки типа
					this->bullets;
				}
				void createMissiles::handle()
				{
				}
				void createPlane::handle()
				{
				}
				void createRicochetes::handle()
				{
				}
				void destroyBullets::handle()
				{
				}
				void destroyMissiles::handle()
				{
				}
				void destroyPlane::handle()
				{
				}
				void interfaceData::handle()
				{
				}
				void setPlanesPositions::handle()
				{
				}
				void updateModules::handle()
				{
				}
			}
			namespace profile
			{
				void sendProfile::handle()
				{
				}
			}
			namespace room
			{
				void roomInfo::handle()
				{
				}
				void roomList::handle()
				{
				}
			}
		}
		namespace bidirectionalMessages
		{
			void textMessage::handle()
			{
				std::cout << "Сервер  пишет " << text << std::endl; 
			}
			void exitFromRoom::handle()
			{

			}
			void logout::handle()
			{

			}
		}
	}
}

int main()
{
	//приветики
	//сервер еще не готов, поэтому данный пример может содержать ошибки, но не ссы
	//ебанутость пространств имен обусловленна тем, что odb не поддерживает вложенные классы
	setlocale(LC_ALL, "rus");
	boost::asio::io_service io_service;

	tcp::resolver resolver(io_service);
	tcp::resolver::query query("127.0.0.1", "40000");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	boost::system::error_code ec;

	//Если в течении 1 секунды после подключения серверу не придет корректное сообщение login, 
	//подключение будет разорвано
	//поэтому считываем логин/пароль до подключения

	roman::network::clientMessages::profile::registry registryMessage;
	roman::network::clientMessages::profile::login loginMessage;
	registryMessage.password = loginMessage.encryptedPassword="хуй";//пока никакой шифровки нет
	registryMessage.name = loginMessage.name="андрей";

	//подключаеся к серверу
	roman::network::connection conn(io_service, 0);
	boost::asio::connect( conn.socket, endpoint_iterator, ec);
	if ( ec )
	{
		std::cout <<ec.message() <<std::endl;
		system("Pause");
		return 0;
	}
	conn.socket.non_blocking(true);
	//создаем две нити. В одной запускаем обработчики входящих сообщений, в другой - все остальное
	//Можно делать это и последовательно. Отправляй и обрабатывай сообщения где хочешь и когда хочешь,
	//но не стоит запускать несколько sendMessage параллельно. То же самое касается и handleImput.
	//Потокобезопасность этих методов стоит в далеком TODO
	omp_set_num_threads(2);
#pragma  omp parallel sections
	{
#pragma omp section
		{
			try
			{
				//////////////////////////////////////////////////////////////////////////
				///////////////////АВТОРИЗАЦИЯ////////////////////////////////////////////
				//////////////////////////////////////////////////////////////////////////
				{
					//регистрируемся и заходим
					conn.sendMessage(registryMessage);
					conn.sendMessage(loginMessage);
					//сигналом успешного логина будет ответ сервера.Его обработчик будет запущен в параллельной нити
					roman::network::serverMessages::profile::sendProfile;
					//поэтому ждем
					size_t ждемОтветаСервера();
				}
				//теперь мы авторизованный игрок. Дисконнект не грозит. Можно не торопиться. Займемся покупками!
				//////////////////////////////////////////////////////////////////////////
				///////////////////МАГАЗИН////////////////////////////////////////////////
				//////////////////////////////////////////////////////////////////////////

				{
					//в сообщении sendProfile нам пришел профиль игрока
					roman::playerProfile::profile Profile;

					//чтобы пользоваться некоторыми функциями профиля, необходимо загрузить базу данных
					std::shared_ptr<odb::database> planesDB;
					planesDB = roman::loadDatabase("planes.db");


					//мы можем посмотреть сколько у нас денег
					Profile.money;
					//ассортимент самолетов
					auto planeName = Profile.planePriceList(planesDB).back().first;
					//понравился самолет? Покупаем!
					roman::network::clientMessages::profile::buyPlaneRequest buyPlaneMessage;
					buyPlaneMessage.planeName = planeName;
					conn.sendMessage(buyPlaneMessage);

					//смотрим список купленных самолетов
					auto plane = Profile.planes.back();
					//в самолете есть модули, например пушки
					size_t gunPosition =  plane.Guns.size() - 1;
					//а почему бы не заменить пушку на более крутую?
					auto gunName = Profile.modulePriceList(plane.ID.planeName,roman::GUN, gunPosition, planesDB).back().first;
					roman::network::clientMessages::profile::buyModuleRequest bmrMessage;
					bmrMessage.moduleName = gunName;
					bmrMessage.moduleType = roman::GUN;
					bmrMessage.planeName = plane.ID.planeName;
					bmrMessage.pos = gunPosition;
					conn.sendMessage(bmrMessage);

					//существуют и другие возможности управления профилем. См. 
					//roman::network::clientMessages::profile;
				}
				//и так, мы зашли в игру и купили самолет. Теперь можно заходить в комнату!
				//////////////////////////////////////////////////////////////////////////
				///////////////////БОЙ////////////////////////////////////////////////////
				//////////////////////////////////////////////////////////////////////////
				{
					//смотрим список комнат
					roman::network::clientMessages::room::roomListRequest rlrMess;
					conn.sendMessage(rlrMess);
					//создаем комнату
					roman::network::clientMessages::room::createRoomRequest crMess;
					crMess.bots=10;
					crMess.name="Моя самая крутая комната";
					conn.sendMessage(rlrMess);
					//заходим в комнату на понравившемся самолете
					size_t planeThatILike = 0;
					size_t roomThatILike = 0;
					roman::network::clientMessages::room::joinRoomRequest jrrMess;
					jrrMess.roomNumber = roomThatILike;
					jrrMess.planeNumber = planeThatILike;
					//К этому моменту карта должна быть загружена со всеми самолетами,
					//т.к. сервер сразу начнет осылать сообщения
					conn.sendMessage(jrrMess);
					//теперь мы в бою!
					roman::network::clientMessages::combat::sendControllable scMess;//уууу-хууу летаем!
					scMess.params.power=10000;//полный форсаж!
					scMess.params.isShooting = true;//тратататата!
					scMess.params.turningVal = 1;//уходим в право как можно резче!
					conn.sendMessage(scMess);
				}
//устал писать. думаю ты разберешься
//если что-то не понятно, спрашивай.
//Ну вот и наступила самая сложная для нас с тобой пора - пора совместной разработки.
//Может возникнуть множество охуительных ситуаций типа постоянных изменений в библиотеке, от которых клиент не работает,
//ебанутых требований с моей стороны,
//взаимная уверенность в том, что ошибка находится в чужом коде,
//да и просто взаимная ненависть!
//
//Не бери в голову!
//Отнесись к этому как к огромной куче дерьма, которую тебе нужно сожрать.
			}
			catch ( roman::planesException & e)
			{
				std::cout << e.what() << std::endl;
			}
		}
#pragma omp section
		{
			while (true)
			{
				try
				{
					conn.handleImput();
				}
				catch ( roman::planesException & e)
				{
					std::cout << e.what() << std::endl;
					break;
				}
				Sleep(100);
			}
		}
	}
	system("Pause");

}