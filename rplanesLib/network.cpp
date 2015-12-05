#include "network.h"
#include "messages.h"
#include "roomMessages.h"
namespace rplanes {
	namespace network {

		Connection::NoHandlerBehavior Connection::noHandlerBehavior =
			Connection::NoHandlerBehavior::THROW_EXCEPTION;

		void Connection::close()
		{
			_socket.close();
		}

		std::string Connection::getIP()
		{
			return _ip;
		}

		Connection::Connection(boost::asio::io_service& io_service, boost::asio::ip::tcp::resolver::iterator & endpoint_iterator) :_socket(io_service)
		{
			boost::asio::connect(_socket, endpoint_iterator);
			_ip = _socket.remote_endpoint().address().to_string();
			_socket.non_blocking(false);
		}

		Connection::Connection(boost::asio::ip::tcp::socket && socket) : _socket(std::move(socket))
		{
			_ip = _socket.remote_endpoint().address().to_string();
			_socket.non_blocking(false);
		}


		std::shared_ptr<Message> Connection::handleMessage()
		{
			std::vector<char> frontValue;
			{
				MutexLocker l(_mutex);
				if (_inputMessages.size() == 0) {
					if (!_socket.is_open())
						throw RPLANES_EXCEPTION("Connection lost {0}.", _ip);
					return nullptr;
				}
				frontValue = _inputMessages.front();
				_inputMessages.pop();
			}
			std::istringstream messageStream(std::string(&frontValue[0], frontValue.size()));

			NetworkIArchive archive(messageStream, boost::archive::no_header);
			auto message = readRegisteredMessage(archive);
			auto id = message->getId();
			_lastMessageId = id;
			if (_handlers.count(id) == 0) {
				if (noHandlerBehavior == NoHandlerBehavior::DO_NOTHING)
					return message;
				throw RPLANES_EXCEPTION("no handler for the message {0}", id);
			}
			_handlers[id](*message);
			return message;
		}


		void Connection::sendMessage(const Message & message)
		{
			if (!_socket.is_open())
				throw RPLANES_EXCEPTION("Connection lost {0}.", _ip);

			std::string data;
			{
				std::ostringstream data_header_stream;
				std::ostringstream archive_stream;

				//serializing data
				NetworkOArchive archive(archive_stream, boost::archive::no_header);
				writeRegisteredMessage(message, archive);

				//writing serailized data size to the header
				data_header_stream << std::setw(headerLength) << std::hex << archive_stream.str().size();
				data = data_header_stream.str() + archive_stream.str();
			}

			bool writeIsInProgress = false;
			{
				MutexLocker ml(_mutex);
				writeIsInProgress = _outputMessages.size() != 0;
				_outputMessages.emplace(std::move(data));
			}
			if (!writeIsInProgress)
			{
				writeMessageAsync();
			}
		}


		void Connection::readHeaderAsync()
		{
			auto self = shared_from_this();
			boost::asio::async_read(_socket,
				boost::asio::buffer(_headerBuffer, headerLength),
				[this, self](boost::system::error_code ec, std::size_t /*length*/)
			{
				size_t messageLength = 0;
				std::istringstream is(std::string(_headerBuffer, headerLength));
				if (!ec && (is >> std::setw(headerLength) >> std::hex >> messageLength))
				{
					_messageBuffer.resize(messageLength);
					if (_socket.is_open() && self.use_count() > 1)
						readMessageAsync();
				}
				else
				{
					_socket.close();
				}
			});
		}

		void Connection::readMessageAsync()
		{
			auto self = shared_from_this();

			boost::asio::async_read(_socket, boost::asio::buffer(_messageBuffer),
				[this, self](boost::system::error_code ec, std::size_t uploadedLength)
			{
				if (!ec && uploadedLength == _messageBuffer.size()) {
					{
						MutexLocker l(_mutex);
						_inputMessages.push(_messageBuffer);
					}
					if (_socket.is_open() && self.use_count() > 1)
						readHeaderAsync();
				}
				else
				{
					_socket.close();
				}
			});
		}

		void Connection::writeMessageAsync()
		{
	
			std::shared_ptr<std::string> str;
			{
				MutexLocker ml(_mutex);
				if (_outputMessages.size() == 0)
					return;
				str = std::make_shared<std::string>(std::move(_outputMessages.front()));
				_outputMessages.pop();
			}
			auto self = shared_from_this();
			
			boost::asio::async_write(_socket, boost::asio::buffer(*str),
				[this, self, str](boost::system::error_code ec, std::size_t /*length*/)
			{
				if (!ec)
				{
					writeMessageAsync();
				}
				else
				{
					_socket.close();
				}
			});

		}
	}
}