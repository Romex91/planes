#pragma once
#include "stdafx.h"
#include "planesException.h"

class Client;

namespace rplanes
{
	namespace network
	{
		class Connection;
		class MessageStorage;

		//////////////////////////////////////////////////////////////////////////
		//message classes base
		class Message
		{
		public:
			virtual void handle() = 0;
			virtual unsigned short getId() const = 0;
			virtual ~Message(){}
			friend Connection;
			friend MessageStorage;
		protected:
			size_t clientID;
			virtual void writeData( boost::archive::binary_oarchive & )=0;
			virtual void readData(boost::archive::binary_iarchive &) = 0;
			virtual std::shared_ptr<Message> copy() = 0;
		};

		//////////////////////////////////////////////////////////////////////////
		//stores registered messages
		class MessageStorage
		{
		public:
			static void registryMessage(Message & mess);
			MessageStorage();
			Message & getMessage(unsigned short id);
			unsigned short getLastMessageID();
		private:
			std::map < unsigned short,std::shared_ptr< Message > > messages_;
			static std::map <unsigned short, std::shared_ptr<Message>> & baseMap();
			unsigned short lastMessageId_;
		};

		//////////////////////////////////////////////////////////////////////////
		template<class T>
		class MessageRegistrar
		{
		public:
			MessageRegistrar()
			{
				static bool registrated(false);
				if (!registrated)
				{
					T mess;
					MessageStorage::registryMessage(mess);
					registrated = true;
				}
			}
		};

		//////////////////////////////////////////////////////////////////////////
		class Connection
		{
		private:
			enum { header_length = 8 };
			MessageStorage messages_;
			size_t clientID_;
			template<typename BUFFER>
			size_t blockingRead(BUFFER & buff, boost::system::error_code & err );
			std::string ip_;
			boost::asio::ip::tcp::socket socket_;
		public:
			boost::system::error_code accept( boost::asio::ip::tcp::acceptor & acceptor);
			boost::system::error_code connect( boost::asio::ip::tcp::resolver::iterator & endpoint_iterator );
			boost::system::error_code close();
			void non_blocking( bool mode );
			Connection(boost::asio::io_service& io_service);
			void setClientID(size_t ID);
			size_t getClientID( );
			std::string getIP();
			unsigned short getLastMessageId();
			bool handleInput( );//nonblocking
			void sendMessage( Message & message );
		};
	}
}