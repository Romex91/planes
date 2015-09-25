#include"planesString.h"

using namespace boost::filesystem;
void rplanes::PlanesString::load(boost::filesystem::path stringsFolder)
{
	if (!exists(stringsFolder)
		|| !is_directory(stringsFolder))
		throw std::exception("cannot find strings resource folder");

	std::for_each(directory_iterator(stringsFolder), directory_iterator(), [](directory_entry entry)
	{
		if (is_regular_file(entry))
		{
			std::fstream fs(entry.path().c_str(), std::ios_base::in);
			std::string line;
			while (std::getline(fs, line)) 
			{
				std::vector<std::string> tokens;
				boost::split(tokens, line, boost::is_any_of("\t"));
				if (tokens.size() != 2)
				{
					std::stringstream ss;
					ss << "strings resource file has a wrong format. " << entry.path();
					throw std::exception(ss.str().c_str());
				}
				formatStrings_[tokens[0]] = tokens[1];
			}
		}
	});
}

template<class Archive>
void rplanes::PlanesString::load(Archive & ar, const unsigned int version)
{
	size_t formatStringID = 0;
	ar & formatStringID;
	formatStringIterator_ = formatStrings_.begin() + formatStringID;
	ar & arguments_;
}

template<class Archive>
void rplanes::PlanesString::save(Archive & ar, const unsigned int version) const
{
	size_t formatStringID = formatStringIterator_ - formatStrings_.begin();
	ar & formatStringID;
	ar & arguments_;
}


rplanes::PlanesString::~PlanesString()
{

}

std::string rplanes::PlanesString::str() const
{
	try {
		if (formatStringIterator_ == formatStrings_.end())
			throw std::exception();
		std::string format = formatStringIterator_->second;
		for (size_t i = 0; i < arguments_.size(); i++)
		{
			std::stringstream ss;
			ss << "{" << i << "}";
			boost::replace_all(format, ss.str(), arguments_[i]);
		}
		return format;
	}
	catch (...) 
	{
		return "error: failed getting string";
	}
}

rplanes::PlanesString::PlanesString(const std::string & formatCS)
{
	formatStringIterator_ = formatStrings_.find(formatCS);
}


rplanes::PlanesString::Map rplanes::PlanesString::formatStrings_;

