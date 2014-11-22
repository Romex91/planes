#pragma  once
namespace rplanes
{

	#define PRINT_VAR(x) std::cout <<  #x << " " << x <<std::endl

	inline std::string _str(UINT uID, HINSTANCE hInstance = ::GetModuleHandle(NULL))
	{
		std::string sDest;
		PWCHAR wsBuf; // no need to initialize
		sDest.clear();
		if (size_t len = ::LoadStringW(hInstance, uID, (PWCHAR)&wsBuf, 0) * sizeof WCHAR)
		{
			sDest.resize(++len); // make room for trailing '\0' in worst case
			sDest.resize(::LoadStringA(hInstance, uID, &*sDest.begin(), len));
		}
		return sDest;
	}

	static const std::string moduleTypesNames[] = {
		"gun",
		"missile",
		"wing",
		"tail",
		"framework",
		"tank",
		"engine",
		"ammunition",
		"turret"
	};

}