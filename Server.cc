#include <arpa/inet.h>
#include <cstdio>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "Application.hh"
#include "Soap.hh"
#include "Server.hh"
#include "ServiceStruct.hh"
#include "main.hh"

#define BUFSIZE 1024*10

void CServer::Start()
{
	Initialize();
	MainLoop();
}

void CServer::Initialize()
{
	struct sockaddr_in srv_addr;
	struct protoent *pe;

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = INADDR_ANY;
	srv_addr.sin_port = htons( AppInstance->Port );

	if (!(pe = getprotobyname("tcp")))
		throw "CServer::Initialize(): getprotobyname(): Unknown protocol: TCP";

	sock = socket(PF_INET,SOCK_STREAM,pe->p_proto);
	if (sock<0)
		throw "CServer::Initialize(): socket(): Could not open socket";

	int opt = 1;
	if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)))
		throw "CServer::Initialize(): setsockopt(): Could not set socket options";	

	if (bind(sock,(struct sockaddr *)&srv_addr,sizeof(srv_addr)))
		throw "CServer::Initialize(): bind(): Could not bind socket";

	if (listen(sock, AppInstance->Threads ))
		throw "CServer::Initialize(): listen(): Could not listen to socket";
}

void CServer::MainLoop()
{
	struct sockaddr_in cl_addr;

	for(;;)
	{
		socklen_t addr_len = sizeof(cl_addr);
		int cl = accept(sock,(struct sockaddr *)&cl_addr,&addr_len);
		if (cl<0)
			continue;
		struct hostent *he;
		char *ip,*name;
		ip = inet_ntoa(cl_addr.sin_addr);
		if (!(he = gethostbyaddr((const char *)&cl_addr.sin_addr,sizeof(cl_addr.sin_addr),AF_INET)))
			name = ip;
		else
			name = he->h_name;
		printf("clientID=%d from %s (%s:%hu)\n",cl,name,ip,ntohs(cl_addr.sin_port));
		
		pthread_t thrd;
		int error;

		if ((error = pthread_create(&thrd,NULL,Handler,(void *)cl)))
		{
			fprintf(stderr,"CServer::MainLoop(): pthread_create(): %s\n",strerror(error));
			close(cl);
		}
	}
}

void *CServer::Handler(void *arg)
{
	int cl = (int)arg;
	char buf_req[BUFSIZE],buf_res[BUFSIZE];
	ssize_t r;
	char *p = buf_req;
	size_t sz = BUFSIZE-1;
	int error;

	pthread_cleanup_push(ConnectionClose,arg);

	if ((error = pthread_detach(pthread_self())))
	{
		fprintf(stderr,"CServer::Handler(): pthread_detach(): %s\n",strerror(error));
		pthread_exit(NULL);
	}

	if ((error = pthread_setspecific( AppInstance->req_key ,buf_req)) ||
		(error = pthread_setspecific( AppInstance->res_key ,buf_res)) )
	{
		fprintf(stderr,"CServer::Handler(): pthread_setspecific(): %s\n",strerror(error));
		pthread_exit(NULL);
	}

	while ( sz>0 && (r = read(cl,p,sz)) > 0 )
	{
		p[r] = '\0';
		p += r;
		sz -= r;
		if (strstr(buf_req, "\n\n") || strstr(buf_req, "\r\n\r\n"))
			break;
	}

	if (sz == 0)
	{
		fprintf(stderr,"clientID=%d Request too long\n",cl);
		pthread_exit(NULL);
	}

	if (r == 0)
	{
		fprintf(stderr,"clientID=%d Premature EOF\n",cl);
		pthread_exit(NULL);
	}

	if (r < 0)
	{
		fprintf(stderr,"clientID=%d Receiving request error: %s\n",cl,strerror(errno));
		pthread_exit(NULL);
	}

	char *method = strtok(buf_req," "),*uri = strtok(NULL," "),*ver=strtok(NULL," \r\n");

	if (!method || (strcmp(method,"GET") && strcmp(method,"POST")))
	{
		ErrorResponse(cl,501,uri);
		pthread_exit(NULL);
	}

	if (!ver || (strcmp(ver,"HTTP/1.0") && strcmp(ver,"HTTP/1.1")))
	{
		ErrorResponse(cl,505,uri);
		pthread_exit(NULL);
	}

	if (!uri || uri[0]!='/' || uri[1]=='/')
	{
		ErrorResponse(cl,400,uri);
		pthread_exit(NULL);
	}

	if (!AppInstance->GetService(uri+1))
	{
		ErrorResponse(cl,404,uri);
		pthread_exit(NULL);
	}

	if (!strcmp(method,"GET"))
		GETResponse(cl,uri);
	else
		POSTResponse(cl,uri);

	pthread_cleanup_pop(1);
}

void CServer::ConnectionClose(void *arg)
{
	int cl = (int)arg;
	if (close(cl))
		fprintf(stderr,"clientID=%d close(socket): %s\n",cl,strerror(errno));
}

void CServer::ErrorResponse(int cl,int code,char *uri)
{
	char *buf_res = (char*)pthread_getspecific( AppInstance->res_key );

	const char *reason;
	switch (code)
	{
		case 400:
			reason = "Bad Request";
			break;
		case 403:
			reason = "Forbidden";
			break;
		case 404:
			reason = "Not Found";
			break;
		case 501:
			reason = "Not Implemented";
			break;
		case 505:
			reason = "HTTP Version Not Supported";
			break;
		default:
			reason = "Unknown";
			break;
	}
	int len = snprintf(buf_res,BUFSIZE,
		"HTTP/1.1 %d %s\r\n\r\n"
		"<HTML>\r\n<HEAD><TITLE>%1$d %2$s</TITLE></HEAD>\r\n"
		"<BODY>Error %1$d %2$s</BODY>\r\n</HTML>\r\n",
		code,reason);
	printf("clientID=%d uri='%s' code=%d\n",cl,uri?uri:"",code);
	SendBuffer(cl,buf_res,len);
}

void CServer::GETResponse(int cl,char *uri)
{
	int len;

	char *buf_res = (char*)pthread_getspecific( AppInstance->res_key );

	Service *s = AppInstance->GetService(uri+1);

	len = snprintf(buf_res,BUFSIZE,
		"HTTP/1.1 200 OK\r\n\r\n"
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<definitions xmlns=\"http://schemas.xmlsoap.org/wsdl/\" xmlns:soap=\"http://schemas.xmlsoap.org/wsdl/soap/\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:ws=\"xmlws\" targetNamespace=\"xmlws\" name=\"%sService\">\r\n\r\n"
		,s->Name.c_str());
	SendBuffer(cl,buf_res,len);

	std::vector<Method*>::iterator i;

	for (i=s->Methods.begin();i!=s->Methods.end();i++)
	{
		Method *m = *i;
		len = snprintf(buf_res,BUFSIZE,"\t<message name=\"%sRequest\">\r\n",m->Name.c_str());
		SendBuffer(cl,buf_res,len);

		std::vector< pair<Types,std::string> >::iterator ii;

		for (ii=m->Parameters.begin();ii!=m->Parameters.end();ii++)
		{
			len = snprintf(buf_res,BUFSIZE,
				"\t\t<part name=\"%s\" type=\"xsd:%s\" />\r\n",(*ii).second.c_str(),GetStringFromType((*ii).first)
			);
			SendBuffer(cl,buf_res,len);
		}
		
		len = snprintf(buf_res,BUFSIZE,"\t</message>\r\n\r\n");
		SendBuffer(cl,buf_res,len);

		len = snprintf(buf_res,BUFSIZE,
			"\t<message name=\"%sResponse\">\r\n"
			"\t\t<part name=\"return\" type=\"xsd:%s\" />\r\n"
			"\t</message>\r\n\r\n",m->Name.c_str(),GetStringFromType(m->ReturnType));
		SendBuffer(cl,buf_res,len);
	}

	len = snprintf(buf_res,BUFSIZE,
		"\t<portType name=\"%sPortType\">\r\n",
		s->Name.c_str()
	);
	SendBuffer(cl,buf_res,len);
	
	for (i=s->Methods.begin();i!=s->Methods.end();i++)
	{
		Method *m = *i;
		len = snprintf(buf_res,BUFSIZE,
			"\t\t<operation name=\"%1$s\">\r\n"
			"\t\t\t<input message=\"ws:%1$sRequest\" />\r\n"
			"\t\t\t<output message=\"ws:%1$sResponse\" />\r\n"
			"\t\t</operation>\r\n",
			m->Name.c_str()
		);
		SendBuffer(cl,buf_res,len);
	}
	
	len = snprintf(buf_res,BUFSIZE,
		"\t</portType>\r\n\r\n\t<binding name=\"%1$sBinding\" type=\"ws:%1$sPortType\">\r\n"
		"\t\t<soap:binding transport=\"http://schemas.xmlsoap.org/soap/http\" style=\"rpc\" />\r\n",
		s->Name.c_str()
	);
	SendBuffer(cl,buf_res,len);

	for (i=s->Methods.begin();i!=s->Methods.end();i++)
	{
		Method *m = *i;

		len = snprintf(buf_res,BUFSIZE,
			"\t\t\t<operation name=\"%s\">\r\n"
			"\t\t\t\t<soap:operation soapAction=\"\" />\r\n"
			"\t\t\t\t\t<input>\r\n"
			"\t\t\t\t\t\t<soap:body use=\"encoded\" encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" />\r\n"
			"\t\t\t\t\t</input>\r\n"
			"\t\t\t\t\t<output>\r\n"
			"\t\t\t\t\t\t<soap:body use=\"encoded\" encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" />\r\n"
			"\t\t\t\t\t</output>\r\n"
			"\t\t\t</operation>\r\n",
			m->Name.c_str()
		);
		SendBuffer(cl,buf_res,len);
	}
		
	len = snprintf(buf_res,BUFSIZE,
		"\t</binding>\r\n\r\n\t<service name=\"%1$sService\">\r\n"
		"\t\t<documentation>%2$s</documentation>\r\n"
		"\t\t<port name=\"%1$sPort\" binding=\"ws:%1$sBinding\">\r\n"
		"\t\t\t<soap:address location=\"http://%3$s:%4$d/%1$s\" />\r\n"
		"\t\t</port>\r\n"
		"\t</service>\r\n\r\n</definitions>\r\n",
		s->Name.c_str(),s->Docs.c_str(),AppInstance->Host,AppInstance->Port
	);
	SendBuffer(cl,buf_res,len);

	printf("clientID=%d uri='%s' code=%d GET\n",cl,uri?uri:"",200);
}

void CServer::POSTResponse(int cl,char *uri)
{
	int len;

	char *buf_res = (char*)pthread_getspecific( AppInstance->res_key );

	SendBuffer(cl,"HTTP/1.1 100 Continue\r\n\r\n",25);

	len = read(cl,buf_res,BUFSIZE);
	buf_res[len] = 0;

	CSoap *soap = new CSoap();
	soap->Response(cl,uri+1,buf_res);
	delete soap;

	printf("clientID=%d uri='%s' code=%d POST/SOAP\n",cl,uri?uri:"",200);
}

int CServer::SendBuffer(int cl,char *buf,int len)
{
	char *p = buf;
	ssize_t w;

	while (len>0)
	{
		if ((w = write(cl,p,len))<0)
		{
			fprintf(stderr,"clientID=%d Sending buffer: %s\n",cl,strerror(errno));
			return -1;
		}
		p += w;
		len -= w;
	}
	return 0;
}
