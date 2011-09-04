#include <strings.h>
#include <iostream>
#include <vector>
#include "Application.hh"
#include "Config.hh"
#include "ServiceStruct.hh"
#include "ezxml.h"

void CConfig::Read(char *xmlfile)
{
	ezxml_t work,server,service,method,root;
	
	root = ezxml_parse_file(xmlfile);

	server = ezxml_child(root,"server");

	work = ezxml_child(server,"host");
	AppInstance->Host = strdup(work->txt);										// /server/host/text()
	
	work = ezxml_child(server,"port");
	AppInstance->Port = atoi(work->txt);										// /server/port/text()

	work = ezxml_child(server,"threads");
	AppInstance->Threads = atoi(work->txt);										// /server/port/text()

	service = ezxml_child(root,"service");										// /service
	while (service)
	{
		char *desc = NULL;
		work = ezxml_child(service,"desc");
		if (work) desc = ezxml_txt(work);
		Service *s = new Service( ezxml_attr(service,"name") , desc );	// /service/name && docs
		method = ezxml_child(service,"method");									// /service/method
		while (method)
		{
			Types rettype = GetTypeFromString( ezxml_attr(method,"returns") );	// /service/method/returns
			Method *m = new Method(rettype, ezxml_attr(method,"name") , ezxml_attr(method,"exec") );
			work = ezxml_child(method,"param");									// /service/method/param
			while (work)
			{
				Types type = GetTypeFromString( ezxml_attr(work,"type") );		// /service/method/param/type
				m->AddParameter(type, ezxml_attr(work,"name") );				// /service/method/param/name
				work = ezxml_next(work);
			}
			s->AddMethod(m);
			method = ezxml_next(method);
		}
		AppInstance->AddService( s );

		service = ezxml_next(service);
	}

	ezxml_free(root);

	if (AppInstance->Threads<=0)
		AppInstance->Threads=1;
}
