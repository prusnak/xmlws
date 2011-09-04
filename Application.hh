#include <pthread.h>
#include <vector>
#include "ServiceStruct.hh"

class CApplication
{
	private:
		static CApplication *_instance;
		void ReadConfig();
		void CreateThreadKeys();
	public:
		CApplication();
		~CApplication();
		static CApplication *Instance();
		void Run();
		void AddService(Service *s);
		Service* GetService(char *name);

		const char *Host;
		int Port;
		int Threads;
		std::vector< Service* > Services;
		pthread_key_t req_key, res_key;
		pthread_mutex_t mutex;
};

#define AppInstance CApplication::Instance()
