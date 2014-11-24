#include "configuration.h"

namespace rplanes
{
	void Configuration::load(const std::string & path)
	{
		std::ifstream fs;
		fs.open(path);
		boost::archive::xml_iarchive archive(fs);
		archive >> boost::serialization::make_nvp("configuration", *this);
	}

	void Configuration::save(const std::string & path)
	{
		std::ofstream fs;
		fs.open(path);
		boost::archive::xml_oarchive archive(fs);
		archive << boost::serialization::make_nvp("configuration", *this);
	}



	Configuration & configuration()
	{
		static bool isFirstCall = true;
		static Configuration conf;
		if (isFirstCall) 
		{
			isFirstCall = false;
			std::string path = "../Resources/configuration.xml";
			try
			{
				conf.load(path);
			}
			catch (std::exception)
			{
				std::cout << "cannot open configuration " + path << std::endl
					<< "creating file with default values..." << std::endl;
				conf.save(path);
			}
		}
		return conf;
	}


	configurationvalues::Profile::Profile()
	{
		startPlanes.push_back("me262");
		startPlanes.push_back("mig9");
		startMaps.push_back("attack.map");
		startMaps.push_back("bot.map");
	}
}
