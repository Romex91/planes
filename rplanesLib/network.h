#pragma once
#include "stdafx.h"
#include "planesException.h"
#include "parallel.h"
#include "messagesRegistration.h"

namespace rplanes
{
	namespace network
	{


		//contains a bit of compile-time magic and asynch insanity
		class Connection
		{
		public:
			typedef boost::archive::binary_oarchive OArchive;
			typedef boost::archive::binary_iarchive IArchive;

			enum { headerLength = sizeof(size_t) * 2};

			enum class NoHandlerBehavior{
				THROW_EXCEPTION,
				DO_NOTHING
			};

			Connection(boost::asio::io_service& io_service);

			//specify message handler.
			//the message handler would be called only when executing handleMessage()
			template<class _Message>
			void setHandler(std::function<void(const _Message &)> handler)
			{
				_handlers[_Message::id] = [handler](const MessageBase & message) {
					handler(dynamic_cast<const _Message &>(message));
				};
			}

			//send message
			void sendMessage(const MessageBase & message);

			//handle a pending message. calls earlier specified handler and returns the message
			//if no messages pending returns nullptr 
			std::shared_ptr<MessageBase> handleMessage();

			boost::system::error_code accept(boost::asio::ip::tcp::acceptor & acceptor);
			boost::system::error_code connect(boost::asio::ip::tcp::resolver::iterator & endpoint_iterator);
			boost::system::error_code close();
			std::string getIP();

			static NoHandlerBehavior noHandlerBehavior;

		protected:
			std::string _ip;
			boost::asio::ip::tcp::socket _socket;
			std::map<MessageId, std::function<void(const MessageBase &)>> _handlers;
			std::queue<std::vector<char>> _pendingMessages;

			//shared resource is _pendingMessages
			Mutex _mutex;

			//read header containing message data length
			void readHeaderAsync();
			char _headerBuffer[headerLength];

			//read message data and push it to _pendingMessages
			void readMessageAsync(size_t messageLength);
			std::vector<char> _messageBuffer;

		};

	}
}


