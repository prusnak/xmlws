#include <string>

class CSoap
{
	private:
		char *Exec(std::string file,std::string args);
	public:
		void Response(int cl,char *uri,char *soap);
};
