#include "network.h"

using namespace rplanes;
using namespace network;
std::map <unsigned short, std::shared_ptr<Message>> & rplanes::network::MessageStorage::baseMap() 
{
	static std::map <unsigned short, std::shared_ptr<Message>> bmap;
	return bmap;
}

void rplanes::network::MessageStorage::registryMessage( Message & mess )
{
	if (baseMap().count(mess.getId())!= 0)
	{
		throw PlanesException(_rstrw("Failed registering message {0}. The message is already registered.", mess.getId()));
	}
	baseMap()[ mess.getId() ] = mess.copy();
}

rplanes::network::MessageStorage::MessageStorage()
{
	for( auto & message:  baseMap() )
	{
		messages_[message.first] = message.second->copy();
	}
}

Message & rplanes::network::MessageStorage::getMessage( unsigned short id )
{
	auto mess = messages_.find(id);
	if ( mess == messages_.end() )
	{
		std::stringstream ss;
		ss << id;
		throw PlanesException(_rstrw("Message with id {0] is not found.", ss.str()));
	}
	lastMessageId_ = id;
	return *mess->second;
}

unsigned short rplanes::network::MessageStorage::getLastMessageID()
{
	return lastMessageId_;
}


template<typename BUFFER>
size_t rplanes::network::Connection::blockingRead( BUFFER & buff, boost::system::error_code & err )
{
	size_t len;
	err = boost::asio::error::would_block;
	size_t count = 0;
	while (err == boost::asio::error::would_block && count < 10 )
	{
		len = socket_.read_some(buff, err);
		count++;
	}
	return len;
}

//not blocking handling of one input message
bool rplanes::network::Connection::handleInput()
{
	boost::system::error_code error;
	//getting class name length
	char id_buffer[sizeof(unsigned short)];
	socket_.read_some(boost::asio::buffer(id_buffer), error);
	if ( error )
	{
		if ( error ==boost::asio::error::would_block )
		{
			return false;
		}
		throw PlanesException(_rstrw("Failed getting message. {0}" , error.message()));
	}
	unsigned short messageId = 0;
	{
		std::istringstream is(std::string(id_buffer, sizeof(unsigned short)));
		if (!(is >> std::hex >> messageId))
		{
			throw PlanesException(_rstrw("Failed getting message."));
		}
	}
	//loading message
	auto & message = messages_.getMessage(messageId);

	//getting serialized data length
	char data_header[header_length];
	blockingRead(boost::asio::buffer(data_header), error);
	if ( error )
	{
		throw PlanesException(_rstrw("Failed getting message. {0}", error.message()));
	}
	std::istringstream is(std::string(data_header, header_length));
	std::size_t data_size = 0;
	if (!(is >> std::hex >> data_size))
	{
		throw PlanesException(_rstrw("Failed getting message."));
	}

	//getting serialized data
	std::vector<char> data;
	data.resize(data_size);

	blockingRead(boost::asio::buffer(data), error);
	//socket.read_some( boost::asio::buffer(data), error );
	if ( error )
	{
		throw PlanesException(_rstrw("Failed getting message. {0}", error.message()));
	}
	std::string archive_data(&data[0], data.size());


	//deserializing data
	std::istringstream archive_stream(archive_data);
	boost::archive::binary_iarchive archive(archive_stream, boost::archive::no_header);
	message.readData(archive);

	//handling message
	message.clientID = clientID_;
	message.handle();
	return true;
}

void rplanes::network::Connection::sendMessage( Message & message )
{
	std::vector<boost::asio::const_buffer> buffers;
	//writing message id
	std::ostringstream id_stream;
	id_stream << std::setw(sizeof(unsigned short))
		<< std::hex << message.getId();
	if (!id_stream || id_stream.str().size() != sizeof(unsigned short))
	{
		throw PlanesException(_rstrw("Failed sending message."));
	}
	std::string id_header = id_stream.str();

	//serializing data
	std::ostringstream archive_stream;
	boost::archive::binary_oarchive archive(archive_stream, boost::archive::no_header);
	message.writeData(archive);

	std::string data;
	data = archive_stream.str();

	//writing serialized data length
	std::ostringstream data_header_stream;
	data_header_stream << std::setw(header_length)
		<< std::hex << data.size();
	if (!data_header_stream || data_header_stream.str().size() != header_length)
	{
		throw PlanesException(_rstrw("Failed sending message."));
	}
	std::string data_header = data_header_stream.str();

	//sending message
	buffers.push_back(boost::asio::buffer(id_header));
	buffers.push_back(boost::asio::buffer(data_header));
	buffers.push_back(boost::asio::buffer(data));

	boost::system::error_code error = boost::asio::error::would_block;
	while (error == boost::asio::error::would_block)
	{
		boost::asio::write(socket_, buffers, error);
	}
	if ( error )
	{
		throw PlanesException(_rstrw("Failed sending message. {0}", error.message()));
	}
}

boost::system::error_code rplanes::network::Connection::accept( boost::asio::ip::tcp::acceptor & acceptor )
{
	boost::system::error_code err;
	acceptor.accept(socket_, err);
	if(!err)
		ip_ = socket_.remote_endpoint().address().to_string();
	return err;
}

boost::system::error_code rplanes::network::Connection::connect( boost::asio::ip::tcp::resolver::iterator & endpoint_iterator )
{
	boost::system::error_code err;
	boost::asio::connect( socket_, endpoint_iterator,  err);
	ip_ = socket_.remote_endpoint().address().to_string();
	return err;
}

void rplanes::network::Connection::non_blocking( bool mode )
{
	socket_.non_blocking(mode);
}

boost::system::error_code rplanes::network::Connection::close()
{
	boost::system::error_code err;
	socket_.close(err);
	return err;
}

unsigned short rplanes::network::Connection::getLastMessageId()
{
	return messages_.getLastMessageID();
}

std::string rplanes::network::Connection::getIP()
{
	return ip_;
}

size_t rplanes::network::Connection::getClientID()
{
	return clientID_;
}

void rplanes::network::Connection::setClientID(size_t ID)
{
	clientID_ = ID;
}

rplanes::network::Connection::Connection(boost::asio::io_service& io_service) :socket_(io_service)
{

}
