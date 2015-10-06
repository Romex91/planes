#include <rstring.h>
#include <fstream>
#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>
#include <boost/archive/text_woarchive.hpp>
#include <boost/archive/text_wiarchive.hpp>
#include <boost/filesystem.hpp>
using namespace rstring;
int main()
{
	//open old resource
	EditableResource<wchar_t, wchar_t> oldResource;
	try {
		std::wifstream xmlFs("../Resources/strings/english.xml");
		boost::archive::xml_wiarchive archive(xmlFs, boost::archive::no_header);
		archive >> boost::serialization::make_nvp("strings", oldResource);
	} catch (std::exception ex) {
		//we cannot use resource strings here because the resorce is not ready
		//so using a raw string constant
		std::cout << "cannot load old resource: " << ex.what() << std::endl;
	}

	//create a resource using the build log
	EditableResource<wchar_t, wchar_t> newResource;
	for (boost::filesystem::directory_iterator i("./"); i != boost::filesystem::directory_iterator(); ++i) {
		if (boost::filesystem::extension(*i) == ".log")
		{
			std::wifstream buildLogFile(i->path().c_str());
			std::wstring buildLog((std::istreambuf_iterator<wchar_t>(buildLogFile)),
				std::istreambuf_iterator<wchar_t>());
			newResource.addStringsFromCompilerOutput(buildLog);
		}
	}

	//merge resources
	oldResource.update(newResource);
	oldResource.merge(newResource);
	oldResource.printOrphanedStrings(newResource);

	//suggest to save the updated resource
	std::cout << "would you like to save the resource(y/n)?" << std::endl;
	char c;
	std::cin >> c;
	if (c == 'y')
	{
		std::cout << std::endl << "saving the archive..." << std::endl;
		std::wofstream xmlFs("../Resources/strings/english.xml");
		boost::archive::xml_woarchive archive(xmlFs, boost::archive::no_header);
		archive << boost::serialization::make_nvp("strings", oldResource);
	}
}