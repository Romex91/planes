#define PLANES_CLIENT //������ ������ �� messages.h
#include <messages.h>
#include <database.h>
#include <exceptions.h>
#include <omp.h>
using boost::asio::ip::tcp;


//messages handlers
//������ ��!
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
					//���������� ���������, ����������� ������ ��� ������� ����
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
				std::cout << "������  ����� " << text << std::endl; 
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
	//���������
	//������ ��� �� �����, ������� ������ ������ ����� ��������� ������, �� �� ���
	//���������� ����������� ���� ������������ ���, ��� odb �� ������������ ��������� ������
	setlocale(LC_ALL, "rus");
	boost::asio::io_service io_service;

	tcp::resolver resolver(io_service);
	tcp::resolver::query query("127.0.0.1", "40000");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	boost::system::error_code ec;

	//���� � ������� 1 ������� ����� ����������� ������� �� ������ ���������� ��������� login, 
	//����������� ����� ���������
	//������� ��������� �����/������ �� �����������

	roman::network::clientMessages::profile::registry registryMessage;
	roman::network::clientMessages::profile::login loginMessage;
	registryMessage.password = loginMessage.encryptedPassword="���";//���� ������� �������� ���
	registryMessage.name = loginMessage.name="������";

	//����������� � �������
	roman::network::connection conn(io_service, 0);
	boost::asio::connect( conn.socket, endpoint_iterator, ec);
	if ( ec )
	{
		std::cout <<ec.message() <<std::endl;
		system("Pause");
		return 0;
	}
	conn.socket.non_blocking(true);
	//������� ��� ����. � ����� ��������� ����������� �������� ���������, � ������ - ��� ���������
	//����� ������ ��� � ���������������. ��������� � ����������� ��������� ��� ������ � ����� ������,
	//�� �� ����� ��������� ��������� sendMessage �����������. �� �� ����� �������� � handleImput.
	//������������������ ���� ������� ����� � ������� TODO
	omp_set_num_threads(2);
#pragma  omp parallel sections
	{
#pragma omp section
		{
			try
			{
				//////////////////////////////////////////////////////////////////////////
				///////////////////�����������////////////////////////////////////////////
				//////////////////////////////////////////////////////////////////////////
				{
					//�������������� � �������
					conn.sendMessage(registryMessage);
					conn.sendMessage(loginMessage);
					//�������� ��������� ������ ����� ����� �������.��� ���������� ����� ������� � ������������ ����
					roman::network::serverMessages::profile::sendProfile;
					//������� ����
					size_t �����������������();
				}
				//������ �� �������������� �����. ���������� �� ������. ����� �� ����������. �������� ���������!
				//////////////////////////////////////////////////////////////////////////
				///////////////////�������////////////////////////////////////////////////
				//////////////////////////////////////////////////////////////////////////

				{
					//� ��������� sendProfile ��� ������ ������� ������
					roman::playerProfile::profile Profile;

					//����� ������������ ���������� ��������� �������, ���������� ��������� ���� ������
					std::shared_ptr<odb::database> planesDB;
					planesDB = roman::loadDatabase("planes.db");


					//�� ����� ���������� ������� � ��� �����
					Profile.money;
					//����������� ���������
					auto planeName = Profile.planePriceList(planesDB).back().first;
					//���������� �������? ��������!
					roman::network::clientMessages::profile::buyPlaneRequest buyPlaneMessage;
					buyPlaneMessage.planeName = planeName;
					conn.sendMessage(buyPlaneMessage);

					//������� ������ ��������� ���������
					auto plane = Profile.planes.back();
					//� �������� ���� ������, �������� �����
					size_t gunPosition =  plane.Guns.size() - 1;
					//� ������ �� �� �������� ����� �� ����� ������?
					auto gunName = Profile.modulePriceList(plane.ID.planeName,roman::GUN, gunPosition, planesDB).back().first;
					roman::network::clientMessages::profile::buyModuleRequest bmrMessage;
					bmrMessage.moduleName = gunName;
					bmrMessage.moduleType = roman::GUN;
					bmrMessage.planeName = plane.ID.planeName;
					bmrMessage.pos = gunPosition;
					conn.sendMessage(bmrMessage);

					//���������� � ������ ����������� ���������� ��������. ��. 
					//roman::network::clientMessages::profile;
				}
				//� ���, �� ����� � ���� � ������ �������. ������ ����� �������� � �������!
				//////////////////////////////////////////////////////////////////////////
				///////////////////���////////////////////////////////////////////////////
				//////////////////////////////////////////////////////////////////////////
				{
					//������� ������ ������
					roman::network::clientMessages::room::roomListRequest rlrMess;
					conn.sendMessage(rlrMess);
					//������� �������
					roman::network::clientMessages::room::createRoomRequest crMess;
					crMess.bots=10;
					crMess.name="��� ����� ������ �������";
					conn.sendMessage(rlrMess);
					//������� � ������� �� ������������� ��������
					size_t planeThatILike = 0;
					size_t roomThatILike = 0;
					roman::network::clientMessages::room::joinRoomRequest jrrMess;
					jrrMess.roomNumber = roomThatILike;
					jrrMess.planeNumber = planeThatILike;
					//� ����� ������� ����� ������ ���� ��������� �� ����� ����������,
					//�.�. ������ ����� ������ ������� ���������
					conn.sendMessage(jrrMess);
					//������ �� � ���!
					roman::network::clientMessages::combat::sendControllable scMess;//����-���� ������!
					scMess.params.power=10000;//������ ������!
					scMess.params.isShooting = true;//�����������!
					scMess.params.turningVal = 1;//������ � ����� ��� ����� �����!
					conn.sendMessage(scMess);
				}
//����� ������. ����� �� �����������
//���� ���-�� �� �������, ���������.
//�� ��� � ��������� ����� ������� ��� ��� � ����� ���� - ���� ���������� ����������.
//����� ���������� ��������� ����������� �������� ���� ���������� ��������� � ����������, �� ������� ������ �� ��������,
//�������� ���������� � ���� �������,
//�������� ����������� � ���, ��� ������ ��������� � ����� ����,
//�� � ������ �������� ���������!
//
//�� ���� � ������!
//�������� � ����� ��� � �������� ���� ������, ������� ���� ����� �������.
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