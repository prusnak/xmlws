#include <pthread.h>
#include <string.h>
#include <cstdio>
#include <vector>

#include "Application.hh"
#include "Config.hh"
#include "Server.hh"
#include "main.hh"

CApplication *CApplication::_instance = 0;

CApplication::CApplication()
{
	_instance = this;
	pthread_mutex_init(&mutex,NULL);
}

CApplication::~CApplication()
{
	pthread_mutex_destroy(&mutex);
	delete Host;
	_instance = 0;
}

CApplication *CApplication::Instance()
{
	if (_instance)
		return _instance;
	else
		throw "CApplication::Instance(): CApplication not instantiated ...";
}

void CApplication::Run()
{
	ReadConfig();
	CreateThreadKeys();
	CServer *server = new CServer();
	server->Start();
	delete server;
}

void CApplication::ReadConfig()
{
	CConfig *config = new CConfig();
	config->Read("xmlws.config");
	delete config;
}

void CApplication::AddService(Service *s)
{
	Services.push_back(s);
}

void CApplication::CreateThreadKeys()
{
	int error;
	if ((error = pthread_key_create(&req_key, NULL)) ||
		(error = pthread_key_create(&res_key, NULL)) )
		throw ssprintf("CApplication::CreateThreadKey(): pthread_key_create(): %s",strerror(error));
}

Service* CApplication::GetService(char *name)
{
	std::vector<Service*>::iterator i;
	for (i=Services.begin();i!=Services.end();i++)
		if ((*i)->Name == name) return (*i);
	return NULL;
}
