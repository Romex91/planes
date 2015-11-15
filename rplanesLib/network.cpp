#include "network.h"

namespace rplanes {
	namespace network {

		Connection::NoHandlerBehavior Connection::noHandlerBehavior =
			Connection::NoHandlerBehavior::THROW_EXCEPTION;

		boost::system::error_code Connection::accept(boost::asio::ip::tcp::acceptor & acceptor)
		{
			boost::system::error_code err;
			acceptor.accept(_socket, err);
			if (!err)
				_ip = _socket.remote_endpoint().address().to_string();
			readHeaderAsync();
			return err;
		}

		boost::system::error_code Connection::connect(boost::asio::ip::tcp::resolver::iterator & endpoint_iterator)
		{
			boost::system::error_code err;
			boost::asio::connect(_socket, endpoint_iterator, err);
			_ip = _socket.remote_endpoint().address().to_string();
			readHeaderAsync();
			return err;
		}

		boost::system::error_code Connection::close()
		{
			boost::system::error_code err;
			_socket.close(err);
			return err;
		}

		std::string Connection::getIP()
		{
			return _ip;
		}

		Connection::Connection(boost::asio::io_service& io_service) :_socket(io_service)
		{

		}

		std::shared_ptr<MessageBase> Connection::handleMessage()
		{
			std::istringstream messageStream;
			{
				MutexLocker l(_mutex);
				if (_pendingMessages.size() == 0)
					return nullptr;
				auto frontValue = _pendingMessages.front();
				messageStream = std::istringstream(std::string(&frontValue[0], frontValue.size()));
				_pendingMessages.pop();
			}

			IArchive archive(messageStream, boost::archive::no_header);
			auto message = readRegisteredMessage(archive);
			auto id = message->getId();
			if (_handlers.count(id) == 0) {
				if (noHandlerBehavior == NoHandlerBehavior::DO_NOTHING)
					return message;
				throw RPLANES_EXCEPTION("no handler for the message {0}", id);
			}
			_handlers[id](*message);
			return message;
		}


		void Connection::sendMessage(const MessageBase & message)
		{
			std::vector<boost::asio::const_buffer> buffers;

			//serializing data
			std::ostringstream archive_stream;
			OArchive archive(archive_stream, boost::archive::no_header);
			writeRegisteredMessage(message, archive);

			//first send the serialized data size
			std::ostringstream data_header_stream;
			data_header_stream << std::setw(headerLength) << std::hex << archive_stream.str().size();
			buffers.push_back(boost::asio::buffer(data_header_stream.str()));
			//sending message
			buffers.push_back(boost::asio::buffer(archive_stream.str()));

			_socket.non_blocking(false);
			boost::system::error_code error;
			boost::asio::write(_socket, buffers, error);
			if (error)
			{
				throw RPLANES_EXCEPTION("Failed sending message. {0}", error.message());
			}
		}


		void Connection::readHeaderAsync()
		{
			boost::asio::async_read(_socket,
				boost::asio::buffer(_headerBuffer, headerLength),
				[this](boost::system::error_code ec, std::size_t /*length*/)
			{
				size_t messageLength = 0;
				std::istringstream is(std::string(_headerBuffer, headerLength));
				if (!ec && (is >> std::hex >> messageLength))
				{
					readMessageAsync(messageLength);
				}
				else
				{
					_socket.close();
				}
			});
		}

		void Connection::readMessageAsync(size_t messageLength)
		{
			_messageBuffer.resize(messageLength);
			boost::asio::async_read(_socket, boost::asio::buffer(_messageBuffer),
				[this, messageLength](boost::system::error_code ec, std::size_t uploadedLength)
			{
				if (!ec && uploadedLength == messageLength) {
					{
						MutexLocker l(_mutex);
						_pendingMessages.push(_messageBuffer);
					}
					readHeaderAsync();
				}
				else
				{
					_socket.close();
				}
			});
		}
	}
}