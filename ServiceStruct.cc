#include <string>
#include <strings.h>
#include <pair.h>
#include <vector>

#include "ServiceStruct.hh"

Service::Service(const char *Name,const char *Docs)
{
	this->Name = Name;
	if (!Docs)
		this->Docs = "No documentation available";
	else
		this->Docs = Docs;
}

void Service::AddMethod(Method *m)
{
	Methods.push_back(m);
}

Method* Service::GetMethod(char *name)
{
	std::vector<Method*>::iterator i;
	for (i=Methods.begin();i!=Methods.end();i++)
		if ((*i)->Name == name) return (*i);
	return NULL;
}

Service::~Service()
{
//	delete Name;
//	delete Methods;
}

Method::Method(Types ReturnType,const char *Name,const char *Exec)
{
	this->Name = Name;
	this->ReturnType = ReturnType;
	this->Exec = Exec;
}

void Method::AddParameter(Types Type,const char *Name)
{
	Parameters.push_back( make_pair(Type,Name) );
}

Method::~Method()
{
//	delete Name;
//	delete Params;
}

Types GetTypeFromString(const char *str)
{
	for (int i=0;i<TypesCount;i++)
		if (!strcmp(str,TypesString[i])) return (Types)i;
	throw "GetTypeFromString(): Unknown Type";
}

const char* GetStringFromType(Types type)
{
	return TypesString[ type ];
}
