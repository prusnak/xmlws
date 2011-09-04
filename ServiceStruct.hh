#ifndef _SERVICESTRUCT_HH_
#define _SERVICESTRUCT_HH_ 1

#include <string>
#include <vector>
#include <pair.h>

const int TypesCount = 5;
enum Types { tBool, tDouble, tFloat, tInt, tString };
static const char *TypesString[TypesCount] = { "boolean", "double", "float", "int", "string" };

class Method
{
	public:
		Types ReturnType;
		std::string Name;
		std::string Exec;
		std::vector< pair<Types,std::string> > Parameters;
		Method(Types ReturnType,const char *Name,const char *Exec);
		void AddParameter(Types Type,const char *Name);
		~Method();
};

class Service
{
	public:
		std::string Name;
		std::string Docs;
		std::vector< Method* > Methods;
		Service(const char *Name,const char *Docs);
		void AddMethod(Method *m);
		Method* GetMethod(char *name);
		~Service();
};

Types GetTypeFromString(const char *str);
const char* GetStringFromType(Types type);

#endif
