#include <stdio.h>
#include <stdlib.h>

#include "HTTPHANDLE.h"
#include "HTTP.h"


#ifdef __WIN32__RELEASE__
 #include <sys/timeb.h>
 #include <process.h>
 #include <time.h>
#include <windows.h>


#else
 #include <stdlib.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <sys/socket.h>
 #include <sys/ioctl.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <pthread.h>
 #include <ctype.h>
 #include <time.h>
 #include <sys/timeb.h>
 #define FILETIME time_t
#endif

/*******************************************************************************************************/
HHANDLE::HHANDLE(void)
{
	target = 0;
	*targetDNS = 0;
	port = 0;
	ThreadID = 0;
	#ifdef _OPENSSL_SUPPORT_
	NeedSSL = 0;
	#endif
	version=0;
	AdditionalHeader = NULL;
	Cookie = NULL;
	UserAgent= NULL;
	DownloadBwLimit = NULL;
	DownloadLimit = NULL;
	conexion = NULL;
	ClientConnection = NULL;
	AsyncHTTPRequest = 0;
	LastRequestedUri = NULL;
	LastAuthenticationString = NULL;
	lpProxyHost = NULL;
	lpProxyPort = NULL;
	lpProxyUserName  = NULL;
	lpProxyPassword = NULL;
	memset(lpTmpData,0,sizeof(lpTmpData));
	#ifdef __WIN32__RELEASE__
	ThreadID = GetCurrentThreadId();
	#else
	ThreadID = pthread_self();
	#endif
	CookieSupported  = 1; /* Enabled by default */
	AutoRedirect	 = 1;
	MaximumRedirects = MAXIMUM_HTTP_REDIRECT_DEEP;

}
/*******************************************************************************************************/

int HHANDLE::InitHandle(HTTPSTR hostname,int HTTPPort,int ssl)
{
	struct sockaddr_in remote;
	remote.sin_addr.s_addr = inet_addr(hostname);
	if (remote.sin_addr.s_addr == INADDR_NONE)
	{
		struct hostent *hostend=gethostbyname(hostname);
		if (!hostend)
		{
			return(0);
		}
		memcpy(&remote.sin_addr.s_addr, hostend->h_addr, 4);
	}
	target=remote.sin_addr.s_addr;
	strncpy(targetDNS, hostname ,sizeof(targetDNS)-1);
	targetDNS[sizeof(targetDNS)-1]='\0';
	port			= HTTPPort;
	#ifdef _OPENSSL_SUPPORT_
	NeedSSL			= ssl;
	#else
	if (ssl) return(0);
	#endif
	version			= 1;
	ThreadID		= 0;
	AdditionalHeader = NULL;
	Cookie = NULL;
	UserAgent= NULL;
	DownloadBwLimit = NULL;
	DownloadLimit = NULL;
	conexion = NULL;
	ClientConnection = NULL;
	AsyncHTTPRequest = 0;
	LastRequestedUri = NULL;
	LastAuthenticationString = NULL;
	lpProxyHost = NULL;
	lpProxyPort = NULL;
	lpProxyUserName  = NULL;
	lpProxyPassword = NULL;
	memset(lpTmpData,0,sizeof(lpTmpData));
	CookieSupported  = 1;
	AutoRedirect	 = 1;
	MaximumRedirects = MAXIMUM_HTTP_REDIRECT_DEEP;
	return(1);
}
/*******************************************************************************************************/
HHANDLE::~HHANDLE() 
{
	target = 0;
	*targetDNS=0;
	port = 0;
	#ifdef _OPENSSL_SUPPORT_
	NeedSSL = 0;
	#endif
	version=0;
	if (AdditionalHeader) free(AdditionalHeader);
	AdditionalHeader = NULL;

	if (Cookie) free(Cookie);
	Cookie = NULL;

	if (UserAgent) free(UserAgent);
	UserAgent= NULL;

	if (DownloadBwLimit) free(DownloadBwLimit);
	DownloadBwLimit = NULL;

	if (DownloadLimit) free(DownloadLimit);
	DownloadLimit = NULL;

	conexion = NULL;
	ClientConnection = NULL;

	AsyncHTTPRequest = 0;
	if (LastRequestedUri)
	{
		free(LastRequestedUri);
		LastRequestedUri = NULL;
	}
	if (LastAuthenticationString) {
		free(LastAuthenticationString);
		LastAuthenticationString = NULL;
	}

	if (lpProxyHost) free(lpProxyHost);
	lpProxyHost = NULL;

	if (lpProxyPort) free(lpProxyPort);
	lpProxyPort = NULL;

	if (lpProxyUserName) free(lpProxyUserName);
	lpProxyUserName  = NULL;

	if (lpProxyPassword) free(lpProxyPassword);
	lpProxyPassword = NULL;

	memset(lpTmpData,0,sizeof(lpTmpData));
	CookieSupported  = 1;
	AutoRedirect	 = 1;
	MaximumRedirects = MAXIMUM_HTTP_REDIRECT_DEEP;
}

/*******************************************************************************************************/
int HHANDLE::SetHTTPConfig(int opt,int parameter)
{
	char tmp[12];
	sprintf(tmp,"%i",parameter);
	switch (opt)
	{
		case ConfigAsyncronousProxy:
			AsyncHTTPRequest = parameter;
			break;

		case ConfigMaxDownloadSpeed:
			if (DownloadBwLimit) free(DownloadBwLimit);
			DownloadBwLimit = strdup(tmp);
			break;

		case ConfigProxyPort:
			if (lpProxyPort) free(lpProxyPort);
			if (parameter) {
				lpProxyPort=strdup(tmp);
			} else {
				lpProxyPort=NULL;
			}
			break;
		case ConfigProtocolversion:
			version=parameter;
			break;
		case ConfigMaxDownloadSize:
			if (DownloadLimit) free(DownloadLimit);
			if (parameter) {
				DownloadLimit = strdup(tmp);
			} else {
				DownloadLimit = NULL;
			}
			break;
		case ConfigCookieHandling:
			CookieSupported=parameter;
			break;
		case ConfigAutoredirect:
			AutoRedirect=parameter;
			break;
		default:
			return(-1);
	}
	return(1);

}


/*******************************************************************************************************/
int HHANDLE::SetHTTPConfig(int opt,HTTPCSTR parameter)
{

	switch (opt)
	{
	case ConfigAsyncronousProxy:
			AsyncHTTPRequest = atoi(parameter);
		break;

	case ConfigMaxDownloadSpeed:
		if (DownloadBwLimit) free(DownloadBwLimit);
		if (parameter)
		{			 
			DownloadBwLimit = strdup(parameter);
		} else {
			DownloadBwLimit = NULL;
		}
		break;
	case ConfigCookie:
		if (Cookie)
		{
			free(Cookie);
			Cookie= NULL;
		}
		if ( (parameter) && (*parameter) ){			
			if (strnicmp(parameter,"Cookie: ",8)==0) //Validate the cookie parameter
			{
				Cookie=strdup(parameter);
			} else //Add Cookie Header..
			{
				Cookie=(char*)malloc( 8 + strlen(parameter) +1 );
				strcpy(Cookie,"Cookie: ");
				strcpy(Cookie+8,parameter);
			}
		}
		break;

	case ConfigAdditionalHeader:
		if (AdditionalHeader) 
		{
			free(AdditionalHeader);			
		}
		if ( (parameter) && (*parameter) && (strchr(parameter,':')) ) 
		{
			int len2 = (int) strlen(parameter);
			if (memcmp(parameter+len2 -2,"\r\n",2)!=0) {
				AdditionalHeader = (char*)malloc(len2 +2 +1 );
				memcpy(AdditionalHeader,parameter,len2);
				memcpy(AdditionalHeader +len2,"\r\n\x00",3);
			} else {
				AdditionalHeader = strdup(parameter);
			}
		}  else {
			AdditionalHeader=NULL;
		}
		break;

	case ConfigUserAgent:
		if (UserAgent) {
			free(UserAgent);
		}
		if (parameter) {			
			UserAgent= strdup(parameter);
		} else {
			UserAgent=NULL;
		}
		break;
	case ConfigProxyHost:
		if (lpProxyHost) {
			free(lpProxyHost);
			lpProxyHost=NULL;
		}
		//NeedSSL=0;
		if (parameter)
		{
			struct sockaddr_in remote;
			lpProxyHost=strdup(parameter);
			remote.sin_addr.s_addr = inet_addr(lpProxyHost);
			if (remote.sin_addr.s_addr == INADDR_NONE)
			{
				struct hostent *hostend=gethostbyname(lpProxyHost);
				if (!hostend) return(-1);
				memcpy(&remote.sin_addr.s_addr, hostend->h_addr, 4);
			}
			target=remote.sin_addr.s_addr;
		} else  {
			struct sockaddr_in remote;
			lpProxyHost = NULL;
			remote.sin_addr.s_addr = inet_addr(targetDNS);
			if (remote.sin_addr.s_addr == INADDR_NONE)
			{
				struct hostent *hostend=gethostbyname(targetDNS);
				if (!hostend) return(-1);
				memcpy(&remote.sin_addr.s_addr, hostend->h_addr, 4);
			}
			target=remote.sin_addr.s_addr;
		}
		conexion=NULL;
		if (!lpProxyPort) lpProxyPort=strdup("8080");
		break;

	case ConfigProxyPort:
		if (lpProxyPort) free(lpProxyPort);
		if (parameter) {	
			lpProxyPort=strdup(parameter);
		} else {
			lpProxyPort=NULL;
		}
		break;

	case ConfigProxyUser:
		if (lpProxyUserName) {
			free(lpProxyUserName);
		}
		if (parameter) {
			lpProxyUserName=strdup(parameter);
		} else lpProxyUserName=NULL;
		break;

	case ConfigProxyPass:
		if (lpProxyPassword) {
			free(lpProxyPassword);
		}
		if (parameter) {
			lpProxyPassword=strdup(parameter);
		} else lpProxyPassword=NULL;
		break;

	case ConfigProtocolversion:
		if (parameter) {
			version=atoi(parameter);
		} else version=1;
		break;
	case ConfigMaxDownloadSize:
		if (DownloadLimit) free(DownloadLimit);		
		if (parameter) {
			DownloadLimit = strdup(parameter);
		} else {
			DownloadLimit = NULL;
		}
		break;
	case ConfigCookieHandling:
		CookieSupported=atoi(parameter);
		break;
	case ConfigAutoredirect:
			AutoRedirect=atoi(parameter);;
			break;
	default:
		return(-1);

	}
	return(1);
}

/*******************************************************************************************************/
HTTPSTR HHANDLE::GetHTTPConfig(int opt)
{

	switch(opt)
	{
	case ConfigHTTPHost:
		return (targetDNS);
	case ConfigHTTPPort:
		sprintf(lpTmpData,"%i",port);
		return (lpTmpData);
	case ConfigMaxDownloadSpeed:
		return(NULL);
	case ConfigCookie:
		return ( Cookie );
	case ConfigAdditionalHeader:
		return ( AdditionalHeader );
	case ConfigUserAgent:
		return ( UserAgent);
	case ConfigProxyHost:
		return ( lpProxyHost);
	case ConfigProxyPort:
		return(lpProxyPort);
	case ConfigProxyUser:
		return ( lpProxyUserName );
	case ConfigProxyPass:
		return ( lpProxyPassword );
	case ConfigProtocolversion:
		sprintf(lpTmpData,"%i",version);
		return (lpTmpData);
	case ConfigSSLSupported:
		#ifdef _OPENSSL_SUPPORT_
		strcpy(lpTmpData,"1");
		#else
		strcpy(lpTmpData,"0");
		#endif
		return (lpTmpData);
	#ifdef _OPENSSL_SUPPORT_
	case ConfigSSLConnection:
		sprintf(lpTmpData,"%i",NeedSSL);
		return (lpTmpData);
	#endif
	case ConfigMaxDownloadSize:
		return (DownloadLimit);
	case ConfigCookieHandling:
		sprintf(lpTmpData,"%i",CookieSupported);
		return (lpTmpData);
	case ConfigAutoredirect:
		sprintf(lpTmpData,"%i",AutoRedirect);
		return (lpTmpData);
	}
	return(NULL);
}
/*******************************************************************************************************/



void *HHANDLE::ParseReturnedBuffer(struct httpdata *request, struct httpdata *response)
{
	char version[4];

	PREQUEST data = new prequest;
	strncpy(data->hostname,targetDNS,sizeof(data->hostname)-1);
	data->ip=target;
	data->port=port;
	#ifdef _OPENSSL_SUPPORT_
	data->NeedSSL = NeedSSL;
	#endif
	data->request=request;
	data->response=response;
	data->server=response->GetServerVersion();
	if (response->HeaderSize>=12)
	{
		memcpy(version,response->Header+9,3);
		version[3]='\0';
		data->status=atoi(version);
	}
	data->ContentType = request->GetHeaderValue("Content-Type:",0);
	data->challenge=response->IschallengeSupported("WWW-Authenticate:");

	char *line = request->GetHeaderValueByID(0);
	if (line)
	{
		char *url=strchr(line,' ');
		if (url)
		{
			*url=0;
			strncpy(data->Method,line,sizeof(data->Method)-1);
			url++;
			char *method = strchr(url,' ');		if (method) *method = 0;
			char *parameters= strchr(url,';');  if (parameters) *parameters = 0;
			parameters= strchr(url,'?');   		if (!parameters) parameters= strchr(url,'&');
			if (parameters) 
			{
				*parameters = 0;
				data->Parameters= strdup(parameters+1);
			}
			data->url=strdup(url);
		}
		free(line);
	}

	

	return(data);

}


/*******************************************************************************************************/
char *HHANDLE::GetAdditionalHeaderValue(const char *value,int n)
{
	char *base,*end;
	end=base=AdditionalHeader;
	if ( (AdditionalHeader) && (value) )
	{
		unsigned int valuelen= (unsigned int) strlen(value);
		while (*end) {
			if (*end=='\n')
			{
				if (strnicmp(base,value,valuelen)==0)
				{
					if (n==0)
					{
						base  = base + valuelen;
						while  (( *base==' ') || (*base==':') )  { base++; }
						unsigned int len = (unsigned int) (end-base);
						char *header=(char*)malloc(len+1);
						memcpy(header,base,len);
						if (header[len-1]=='\r')
						{
							header[len-1]='\0';
						} else {
							header[len]='\0';
						}
						return (header);
					} else
					{
						n--;
					}
				}
				base=end+1;
			}
			end++;
		}
	}
	return(NULL);
}
/*******************************************************************************************************/
