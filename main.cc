#include <cstdio>
#include <cstdlib>
#include <stdarg.h>
#include <signal.h>

#include "Application.hh"

char *ssprintf(const char *format, ...)
{
	static char buf[1024];
	va_list ap;
	va_start(ap,format);
	vsnprintf(buf,1023,format,ap);
	va_end(ap);
	return buf;
}

void hnd(int sig)
{
}

void setpipehandler()
{
	struct sigaction sa;
	sa.sa_handler = hnd;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGPIPE, &sa, NULL);
}

int main(int args,char **argv)
{
	setpipehandler();
	try
	{
		CApplication *xmlws = new CApplication();
		xmlws->Run();
		delete xmlws;
	}
	catch (char const* msg)
	{
		fprintf(stderr,"%s\n",msg);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
