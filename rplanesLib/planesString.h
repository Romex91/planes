#pragma once
namespace rplanes
{
	class PlanesString
	{
	public:
		//format is in c# style in a reduced form. 
		//	PlanesString("String: {0}, Int: {2}, float: {1}", "string", 1.f, 1) - correct
		//	PlanesString("String: %s, Int: %d, float: %f", "string, 1, 1.f") - incorrect
		template < typename... Args>
		explicit PlanesString(const std::string & formatCS, Args ... args) : PlanesString(formatCS)
		{
			saveArgs(args...);
		}

		//format is in c# style in a reduced form. 
		//	PlanesString("String: {0}, Int: {2}, float: {1}", "string", 1.f, 1) - correct
		//	PlanesString("String: %s, Int: %d, float: %f", "string, 1, 1.f") - incorrect
		explicit PlanesString(const std::string & formatCS);

		PlanesString() = default;

		virtual ~PlanesString();

		std::string str()const;

		static void load(boost::filesystem::path stringsFolder);
	private:
		//private members
		typedef boost::container::flat_map < std::string, std::string > Map;
		static Map formatStrings_;
		Map::iterator formatStringIterator_ = formatStrings_.end();
		std::vector<std::string> arguments_;

		//private methods
		template < typename First, typename... Other>
		void saveArgs(First first, Other ... other)
		{
			std::stringstream ss;
			ss << first;
			arguments_.push_back(ss.str());
			saveArgs(other...);
		}

		template < typename First>
		void saveArgs(First first)
		{
			std::stringstream ss;
			ss << first;
			arguments_.push_back(ss.str());
		}


		//boost serialization
		friend class boost::serialization::access;
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const;
		template<class Archive>
		void load(Archive & ar, const unsigned int version);
		BOOST_SERIALIZATION_SPLIT_MEMBER()

	};
	typedef PlanesString _str;
}
