#pragma  once
namespace rplanes
{

	#define PRINT_VAR(x) std::cout <<  #x << " " << x <<std::endl

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