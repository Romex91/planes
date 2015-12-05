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
		//io_service::run should work in some other thread
		class Connection : private boost::noncopyable, public std::enable_shared_from_this<Connection>
		{
		public:

			enum { headerLength = sizeof(size_t) * 2};

			enum class NoHandlerBehavior{
				THROW_EXCEPTION,
				DO_NOTHING
			};

			Connection(boost::asio::io_service& io_service, boost::asio::ip::tcp::resolver::iterator & endpoint_iterator);

			Connection(boost::asio::ip::tcp::socket && socket);

			//specify message handler.
			//the message handler would be called only when executing handleMessage()
			template<class _Message>
			void setHandler(std::function<void(const _Message &)> handler)
			{
				_handlers[_Message::id] = [handler](const Message & message) {
					handler(dynamic_cast<const _Message &>(message));
				};
			}

			//send message
			void sendMessage(const Message & message);

			//handle a pending message. calls earlier specified handler and returns the message
			//if no messages pending returns nullptr 
			std::shared_ptr<Message> handleMessage();

			bool isOpen();

			std::shared_ptr<Connection> start()
			{
				if (!_started)
				{
					_started = true;
					readHeaderAsync();
				}
				return shared_from_this();
			}

			void close();

			std::string getIP();

			static NoHandlerBehavior noHandlerBehavior;

			MessageId getLastMessageId() 
			{
				return _lastMessageId;
			}


		protected:
			bool _started = false;
			std::string _ip;
			boost::asio::ip::tcp::socket _socket;
			std::map<MessageId, std::function<void(const Message &)>> _handlers;
			std::queue<std::vector<char>> _inputMessages;
			std::queue<std::string> _outputMessages;
			//shared resources are _inputMessages and _outputMessages
			Mutex _mutex;

			MessageId _lastMessageId = 0;

			//read header containing message data length
			void readHeaderAsync();
			char _headerBuffer[headerLength];

			//read message data and push it to _inputMessages
			void readMessageAsync();
			std::vector<char> _messageBuffer;

			//write a message from _outputMessages queue
			void writeMessageAsync();
		};


		//asynchronious listener
		class Listener : private boost::noncopyable
		{
		public:
			Listener(boost::asio::io_service& io_service, boost::asio::ip::tcp::endpoint & endpoint,
				std::function<void(std::shared_ptr<Connection>)> connectionHandler) :
				_acceptor(io_service, endpoint),
				_socket(io_service),
				_connectionHandler(connectionHandler)
			{
				do_accept();
			}

		private:

			void do_accept()
			{
				_acceptor.async_accept(_socket,
					[this](boost::system::error_code err)
				{
					if (!err) {
						_connectionHandler(std::make_shared<Connection>(std::move(_socket))->start());
					}
					do_accept();
				});
			}
			std::function<void(std::shared_ptr<Connection>)> _connectionHandler;
			boost::asio::ip::tcp::acceptor _acceptor;
			boost::asio::ip::tcp::socket _socket;
		};
	}
}


