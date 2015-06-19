#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <WinCrypt.h>
#pragma comment(lib, "crypt32.lib")
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

#include "scope_guard.h"
#include "string_utility.h"


inline std::vector<BYTE> HttpRequest(const std::wstring& url, const std::wstring& action, const std::wstring& headers, const std::vector<BYTE>& content)
{
	std::vector<BYTE> response;
	HINTERNET hSession = InternetOpen(L"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; WOW64; Trident/6.0)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hSession == NULL)
	{
		return response;
	}
	ON_SCOPE_EXIT([&] { InternetCloseHandle(hSession); });

	HINTERNET hRequest = InternetOpenUrl(hSession, url.c_str(), NULL, 0, INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_DONT_CACHE |INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD, NULL);
	if (hRequest == NULL)
	{
		return response;
	}
	ON_SCOPE_EXIT([&] { InternetCloseHandle(hRequest); });

	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	for(;;)
	{
		InternetQueryDataAvailable(hRequest, &dwSize, 0, 0);
		if (dwSize == 0)
		{
			break;
		}
		std::vector<BYTE> buf(dwSize + 1);
		InternetReadFile(hRequest, (LPVOID)&buf[0], dwSize, &dwDownloaded);
		response.insert(response.end(), buf.begin(), buf.begin() + dwDownloaded);
	}
	return response;
}


inline std::wstring HttpRequest(const std::wstring& url, const std::wstring& action, const std::wstring& headers, const std::wstring& content)
{
	std::string contentUTF8 = UTF16ToUTF8(content);
	std::vector<BYTE> data (contentUTF8.begin(), contentUTF8.end());
	std::vector<BYTE> response = HttpRequest(url, action, headers, data);
	std::stringstream sstream;
	std::copy(response.begin(), response.end(), std::ostream_iterator<char>(sstream, ""));
	return UTF8ToUTF16(sstream.str());
}


inline std::vector<BYTE> HttpRequest(const std::wstring& hostName, int port, const std::wstring& path, const std::wstring& action, const std::wstring& headers, const std::vector<BYTE>& content)
{
	std::vector<BYTE> response;
	HINTERNET hSession = InternetOpen(L"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; WOW64; Trident/6.0)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hSession == NULL)
	{
		return response;
	}

	ON_SCOPE_EXIT([&] { InternetCloseHandle(hSession); });

	DWORD dwOption = 25000;
	InternetSetOption(hSession, INTERNET_OPTION_CONNECT_TIMEOUT, &dwOption, sizeof(dwOption));
	InternetSetOption(hSession, INTERNET_OPTION_DATA_SEND_TIMEOUT, &dwOption, sizeof(dwOption));
	InternetSetOption(hSession, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &dwOption, sizeof(dwOption));
	dwOption = 40000;
	InternetSetOption(hSession, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwOption, sizeof(dwOption));

	HINTERNET hConnect = InternetConnect(hSession, hostName.c_str(), (INTERNET_PORT)port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	if (hConnect == NULL)
	{
		return response;
	}
	ON_SCOPE_EXIT([&] { InternetCloseHandle(hConnect); });

	HINTERNET hRequest = HttpOpenRequest(hConnect, action.c_str(), path.c_str(),
		NULL, NULL, NULL,
		INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD,
		NULL);
	if (hRequest == NULL)
	{
		return response;
	}
	ON_SCOPE_EXIT([&] { InternetCloseHandle(hRequest); });

	LPVOID pData = content.size() > 0 ? (LPVOID)(&content[0]) : NULL;

	HttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.length(), pData, (DWORD)content.size());

	DWORD dwStatusCode = 0;
	DWORD dwTemp       = sizeof(dwStatusCode);
	HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatusCode, &dwTemp, NULL);

	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	std::vector<BYTE> buf(dwSize + 1);
	for(;;)
	{
		InternetQueryDataAvailable( hRequest, &dwSize, 0, 0);
		if (dwSize == 0)
		{
			break;
		}
		InternetReadFile( hRequest, (LPVOID)&buf[0], dwSize, &dwDownloaded );
		response.insert(response.end(), buf.begin(), buf.begin() + dwDownloaded);
	}
	return response;
}


inline std::wstring HttpRequest(const std::wstring& hostName, int port, const std::wstring& path, const std::wstring& action, const std::wstring& headers, const std::wstring& content)
{
	std::string contentUTF8 = UTF16ToUTF8(content);
	std::vector<BYTE> data (contentUTF8.begin(), contentUTF8.end());
	std::vector<BYTE> response = HttpRequest(hostName, port, path, action, headers, data);
	std::stringstream sstream;
	std::copy(response.begin(), response.end(), std::ostream_iterator<char>(sstream, ""));
	return UTF8ToUTF16(sstream.str());
}


//---------------------------------------------------------------------------------
inline std::vector<BYTE> HttpsRequest(const std::wstring& url, const std::wstring& action, const std::wstring& headers, const std::vector<BYTE>& content)
{
	HINTERNET hSession = InternetOpen(L"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; WOW64; Trident/6.0)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	ON_SCOPE_EXIT([&] { InternetCloseHandle(hSession); });

	HINTERNET hRequest = InternetOpenUrl(hSession, url.c_str(), NULL, 0, INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_DONT_CACHE |INTERNET_FLAG_PRAGMA_NOCACHE, NULL);
	ON_SCOPE_EXIT([&] { InternetCloseHandle(hRequest); });

	//server certificate validation
	PCCERT_CHAIN_CONTEXT pccert = NULL;
	ON_SCOPE_EXIT([&] 
	{ 
		if (pccert != NULL)
		{
			CertFreeCertificateChain(pccert);
		}
	});

	DWORD dwCertChainContextSize = sizeof(PCCERT_CHAIN_CONTEXT);
	CERT_CHAIN_POLICY_STATUS pps = {0};
	InternetQueryOption(hRequest, INTERNET_OPTION_SERVER_CERT_CHAIN_CONTEXT, &pccert, &dwCertChainContextSize);
	CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, pccert, NULL, &pps);

	std::vector<BYTE> response;
	for(;;)
	{
		DWORD dwSize = 0;
		InternetQueryDataAvailable( hRequest, &dwSize, 0, 0);

		if (dwSize == 0)
		{
			break;
		}
		std::vector<char> buf(dwSize + 1);
		DWORD dwDownloaded = 0;
		InternetReadFile( hRequest, (LPVOID)&buf[0], dwSize, &dwDownloaded );
		response.insert(response.end(), buf.begin(), buf.begin() + dwSize);
	}
	return response;
}


inline std::wstring HttpsRequest(const std::wstring& url, const std::wstring& action, const std::wstring& headers, const std::wstring& content)
{
	std::string contentUTF8 = UTF16ToUTF8(content);
	std::vector<BYTE> data (contentUTF8.begin(), contentUTF8.end());
	std::vector<BYTE> response = HttpsRequest(url, action, headers, data);
	std::stringstream sstream;
	std::copy(response.begin(), response.end(), std::ostream_iterator<char>(sstream, ""));
	return UTF8ToUTF16(sstream.str());
}


inline std::vector<BYTE> HttpsRequest(const std::wstring& hostName, int port, const std::wstring& path, const std::wstring& action, const std::wstring& headers, const std::vector<BYTE>& content)
{
	HINTERNET hSession = InternetOpen(L"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; WOW64; Trident/6.0)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	ON_SCOPE_EXIT([&] { InternetCloseHandle(hSession); });

	DWORD dwOption = 25000;
	InternetSetOption(hSession, INTERNET_OPTION_CONNECT_TIMEOUT, &dwOption, sizeof(dwOption));
	InternetSetOption(hSession, INTERNET_OPTION_DATA_SEND_TIMEOUT, &dwOption, sizeof(dwOption));
	InternetSetOption(hSession, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &dwOption, sizeof(dwOption));
	dwOption = 40000;
	InternetSetOption(hSession, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwOption, sizeof(dwOption));

	HINTERNET hConnect = InternetConnect(hSession, hostName.c_str(), (INTERNET_PORT)port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	ON_SCOPE_EXIT([&] { InternetCloseHandle(hConnect); });

	HINTERNET hRequest = HttpOpenRequest(hConnect, action.c_str(), path.c_str(),
		NULL, NULL, NULL,
		INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_DONT_CACHE |INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_SECURE,
		NULL);
	ON_SCOPE_EXIT([&] { InternetCloseHandle(hRequest); });

	LPVOID pData = content.size() > 0 ? (LPVOID)(&content[0]) : NULL;
	HttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.length(), pData, (DWORD)content.size());

	//server certificate validation
	PCCERT_CHAIN_CONTEXT pccert = NULL;
	ON_SCOPE_EXIT([&] 
	{ 
		if (pccert != NULL)
		{
			CertFreeCertificateChain(pccert);
		}
	});

	DWORD dwCertChainContextSize = sizeof(PCCERT_CHAIN_CONTEXT);
	CERT_CHAIN_POLICY_STATUS pps = {0};
	InternetQueryOption(hRequest, INTERNET_OPTION_SERVER_CERT_CHAIN_CONTEXT, &pccert, &dwCertChainContextSize);

	CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, pccert, NULL, &pps);

	DWORD dwStatusCode = 0;
	DWORD dwTemp       = sizeof(dwStatusCode);
	HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE| HTTP_QUERY_FLAG_NUMBER,
		&dwStatusCode, &dwTemp, NULL);

	std::vector<BYTE> response;
	for(;;)
	{
		DWORD dwSize = 0;
		InternetQueryDataAvailable( hRequest, &dwSize, 0, 0);

		if (dwSize == 0)
		{
			break;
		}
		std::vector<char> buf(dwSize + 1);
		DWORD dwDownloaded = 0;
		InternetReadFile( hRequest, (LPVOID)&buf[0], dwSize, &dwDownloaded );
		response.insert(response.end(), buf.begin(), buf.begin() + dwSize);
	}
	return response;
}


inline std::wstring HttpsRequest(const std::wstring& hostName, int port, const std::wstring& path, const std::wstring& action, const std::wstring& headers, const std::wstring& content)
{
	std::string contentUTF8 = UTF16ToUTF8(content);
	std::vector<BYTE> data (contentUTF8.begin(), contentUTF8.end());
	std::vector<BYTE> response = HttpsRequest(hostName, port, path, action, headers, data);
	std::stringstream sstream;
	std::copy(response.begin(), response.end(), std::ostream_iterator<char>(sstream, ""));
	return UTF8ToUTF16(sstream.str());
}
