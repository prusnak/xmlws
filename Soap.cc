#include <cstdio>
#include <iostream>
#include <pthread.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

#include "Application.hh"
#include "Soap.hh"
#include "Server.hh"
#include "ezxml.h"

#define BUFSIZE 1024*10

void CSoap::Response(int cl,char *uri,char *soap)
{
	ezxml_t xmlroot = ezxml_parse_str(soap,strlen(soap));
	if (xmlroot==NULL)
		throw "CSoap::Response(): Cannot parse SOAP request";
	ezxml_t soapbody = ezxml_child(xmlroot,"soap:Body");
	if (soapbody==NULL)
		throw "CSoap::Response(): <soap:Body> is missing in SOAP request";
	ezxml_t methodnode = soapbody->child;
	if (methodnode==NULL)
		throw "CSoap::Response(): No method call in SOAP request";
	char *method = strdup(methodnode->name);

	ezxml_t param = methodnode->child;
	std::string paramstr = "";
	while (param)
	{
		paramstr += ezxml_txt(param);
		paramstr += "\n";
		param = param->sibling;
	}
	ezxml_free(xmlroot);

	pthread_mutex_lock(&AppInstance->mutex);
	char *result = Exec(AppInstance->GetService(uri)->GetMethod(method)->Exec,paramstr);
	pthread_mutex_unlock(&AppInstance->mutex);
	
	int len;	
	char *buf_res = (char*)pthread_getspecific( AppInstance->res_key );
	len = snprintf(buf_res,BUFSIZE,
		"HTTP/1.1 200 OK\r\nStatus: 200\r\nContent-Type: text/xml; charset=utf-8\r\n\r\n"
		"<?xml version=\"1.0\"?>"
		"<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsi=\"http://www.w3.org/1999/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/1999/XMLSchema\">"
		"<soap:Body>"
		"<Response soap:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<return xsi:type=\"xsd:%s\">%s</return>"
		"</Response>"
		"</soap:Body>"
		"</soap:Envelope>",
		GetStringFromType( AppInstance->GetService(uri)->GetMethod(method)->ReturnType ),
		result
		);
	CServer::SendBuffer(cl,buf_res,len);
	delete result;
}

char *CSoap::Exec(std::string file,std::string args)
{
	char *buf=new char[BUFSIZE];
	buf[0] = 0;
	int status,pdin[2],pdout[2];
	pid_t pid,pid2;
	pipe(pdin);
	pipe(pdout);
	switch (pid=fork())
	{
		case -1:
			fprintf(stderr,"fork(): Cannot fork\n");
			return NULL;
		case 0:
			close(1);
			dup2(pdin[1],1);
			close(pdin[1]);
			close(pdin[0]);
			write(1,args.c_str(),args.length());
			close(1);
			exit(0);
		default:
			close(0);
			dup2(pdin[0], 0);
			close(pdin[0]);
			close(pdin[1]);
			switch (pid2=fork())
			{
				case -1:
					fprintf(stderr,"fork(): Cannot fork\n");
					return NULL;
				case 0:
					close(1);
					dup2(pdout[1],1);
					close(pdout[1]);
					close(pdout[0]);
					execlp(file.c_str(),file.c_str(),NULL);
					break;
				default:
					close(0);
					dup2(pdout[0], 0);
					close(pdout[0]);
					close(pdout[1]);
					read(0,buf,1024);
					strtok(buf,"\r\n");
					return buf;
			}			
	}
}
