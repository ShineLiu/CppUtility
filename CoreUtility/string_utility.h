#pragma once

#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include "error_handling_utility.h"

template <typename Iter>
inline std::wstring ConcatAll(Iter begin, Iter end, std::wstring const& delim = _T(""))
{
    if(begin == end)
    {
        return (L"");
    }

    std::wstringstream ss;

    ss << *begin++; // write the first elem

    for(; begin != end; ++begin)
    {
        if(!delim.empty())
        {
            ss << delim;
        }

        ss << *begin;
    }

    return ss.str();
}

template <typename CharT>
inline std::vector<std::basic_string<CharT>>& Split(const std::basic_string<CharT>& s, CharT delim, std::vector<std::basic_string<CharT>>& elems) 
{
    std::basic_stringstream<CharT> ss(s);
    std::basic_string<CharT> item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

template <typename CharT>
inline std::vector<std::basic_string<CharT>> Split(const std::basic_string<CharT>& s, CharT delim) 
{
    std::vector<std::basic_string<CharT>> elems;
    return Split(s, delim, elems);
}

template <typename T, typename CharT>
inline T lexical_cast(std::basic_string<CharT> const& str)
{
    std::basic_stringstream<CharT> ss;
    ss << str;
    T t;
    ss >> t;
    return t;
}

template <typename CharT>
inline POINT ParsePoint(std::basic_string<CharT> const& str)
{
    typedef std::basic_string<CharT> str_t;
    std::vector<str_t> xy = Split(str, L',');

    POINT pt = { lexical_cast<int>(xy[0]), lexical_cast<int>(xy[1]) };
    return pt;
}

template <typename CharT>
inline std::vector<int> ParseInts(std::basic_string<CharT> const& str)
{
    typedef std::basic_string<CharT> str_t;
    std::vector<str_t> intStrs = Split(str, L',');

    std::vector<int> ints;

    std::transform(intStrs.begin(), intStrs.end(), std::back_inserter(ints), [](str_t str) -> int { return lexical_cast<int>(str); });

    return ints;
}

template <typename T>
inline std::wstring ToString(T const& t)
{
    std::wstringstream ss;
    ss << t;
    return ss.str();
}

template <typename CharT, typename T>
inline std::basic_string<CharT> ToString(T const& t, int width, CharT padding)
{
    std::basic_stringstream<CharT> ss;
    ss << std::setw(width) << std::setfill(padding) << t;
    std::basic_string<CharT> s;
    ss >> s;
    return s;
}

inline bool IsEqualIgnoreCase(std::wstring str1, std::wstring str2)
{
    std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
    std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);

    return str1 == str2;
}

inline bool StartsWith(std::wstring const& str, std::wstring const& prefix)
{
    if(prefix.length() > str.length())
    {
        return false;
    }

    return str.compare(0, prefix.length(), prefix) == 0;
}

inline bool EndsWith(std::wstring const& str, std::wstring const& suffix)
{
    if(suffix.length() > str.length())
    {
        return false;
    }

    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

inline void ReplaceAll(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr)
{
    size_t pos = 0;
    while((pos = str.find(oldStr, pos)) != std::wstring::npos)
    {
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}

inline std::wstring ToUpper(const std::wstring& strIn)
{
	std::wstring s(strIn);
	transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

inline std::wstring ToLower(const std::wstring& strIn)
{
	std::wstring s(strIn);
	transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

inline std::wstring Format(WCHAR* format, ...)
{
	WCHAR mid[2048] = { 0 };
	va_list args;
	va_start(args, format);
	_vsnwprintf_s(mid, 2048 - 1, format, args);
	va_end(args);
	return std::wstring(mid);
}

inline int IndexOfFirst(const std::wstring& strIn, const std::wstring& strMatch)
{
	return (int)strIn.find(strMatch, 0);
}

inline int IndexOfLast(const std::wstring& strIn, const std::wstring& strMatch)
{
	return (int)strIn.rfind(strMatch, strIn.size() - 1);
}

inline std::wstring Trim(const std::wstring& strIn, const std::wstring& strMatch = (L""))
{
	std::wstring s(strIn);
	s.erase(0, s.find_first_not_of(strMatch != (L"") ? strMatch : (L" \r\n")));
	s.erase(s.find_last_not_of(strMatch != (L"") ? strMatch : (L" \r\n")) + 1);
	return s;
}

inline std::wstring TrimLeft(const std::wstring& strIn, const std::wstring& strMatch = (L""))
{
	std::wstring s(strIn);
	s.erase(0, s.find_first_not_of(strMatch != (L"") ? strMatch : (L" \r\n")));
	return s;
}

inline std::wstring TrimRight(const std::wstring& strIn, const std::wstring& strMatch = (L""))
{
	std::wstring s(strIn);
	s.erase(s.find_last_not_of(strMatch != (L"") ? strMatch : (L" \r\n")) + 1);
	return s;
}

inline int SplitString(const std::wstring& strIn, const std::wstring& strDelimiter, std::vector<std::wstring>& ret)
{
	ret.clear();

	int iPos = 0;
	int newPos = -1;
	int delimiterLength = (int)strDelimiter.size();
	int strInLength = (int)strIn.size();

	if (delimiterLength == 0 || strInLength == 0)
		return 0;

	std::vector<int> positions;

	newPos = (int)strIn.find(strDelimiter, 0);

	if (newPos < 0)
	{
		ret.push_back(strIn);
		return 1;
	}

	int numFound = 0;

	while (newPos >= iPos)
	{
		numFound++;
		positions.push_back(newPos);
		iPos = newPos;
		newPos = (int)strIn.find(strDelimiter, iPos + delimiterLength);
	}

	for (unsigned int i = 0; i <= positions.size(); ++i)
	{
		std::wstring s((L""));
		if (i == 0) 
		{ 
			s = strIn.substr(i, positions[i]);
		}
		else
		{
			int offset = positions[i-1] + delimiterLength;
			if (offset < strInLength)
			{
				if (i == positions.size())
				{
					s = strIn.substr(offset);
				}
				else
				{
					s = strIn.substr(offset, positions[i] - positions[i-1] - delimiterLength);
				}
			}
		}

		if (s.size() > 0)
		{
			ret.push_back(s);
		}
	}
	return numFound;
}

inline std::wstring UTF8ToUTF16(std::string const& str)
{
	int requiredSize =  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	if (requiredSize == 0){ return (L"");} 

	std::vector<wchar_t> wstr(requiredSize);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], (int)wstr.size());
	return &wstr[0];
}

inline std::string UTF16ToUTF8(std::wstring const& wstr)
{
	int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (requiredSize == 0){ return "";} 

	std::vector<char> str(requiredSize);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], (int)str.size(), nullptr, nullptr);
	return &str[0];
}

inline SYSTEMTIME WstringToSystemTime(std::wstring szTime)
{
	SYSTEMTIME tm;
	swscanf_s(szTime.c_str(), (L"%04hu/%02hu/%02hu/%01hu %02hu:%02hu:%02hu.%03hu"), &tm.wYear, &tm.wMonth, &tm.wDay, &tm.wDayOfWeek, &tm.wHour, &tm.wMinute, &tm.wSecond, &tm.wMilliseconds);
	return tm;
}

inline std::wstring SystemTimeToWstring(SYSTEMTIME tm)
{
	wchar_t tmpbuf[128] = {0};
	swprintf_s(tmpbuf, 128, (L"%04hu/%02hu/%02hu/%01hu %02hu:%02hu:%02hu.%03hu"), tm.wYear, tm.wMonth, tm.wDay,tm.wDayOfWeek, tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);
	return std::wstring(tmpbuf);
}

inline std::string ByteToHex(unsigned char byte)
{
    static char const HEX[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    auto high = byte >> 4;
    auto low = byte & 0x0f;
    ENSURE( 0 <= high && high < 16 && 0 <= low && low < 16 )(high)(low);

    char const result[] = { HEX[high], HEX[low], '\0' };
    return result;
}

inline std::string UrlEncode(std::string const& str)
{
    std::stringstream ss;

    std::for_each(str.begin(), str.end(), [&](char ch) 
    {
        if(isascii(ch))
        {
            ss << ch;
        }
        else
        {
            ss << "%" << ByteToHex(ch);
        }
    });

    return ss.str();
}

inline wchar_t* CharToWchar(const char* ch)  
{
	wchar_t* m_wchar;  

	int len = MultiByteToWideChar(CP_ACP, 0, ch, strlen(ch), NULL, 0);
	m_wchar = new wchar_t[len+1];  
	MultiByteToWideChar(CP_ACP, 0, ch, strlen(ch), m_wchar, len);  
	m_wchar[len] = '\0';  
	return m_wchar;  
}  
inline wchar_t* StringToWchar(const std::string& s)  
{  
	const char* temp = s.c_str();  
	return CharToWchar(temp);  
} 

inline std::wstring StringToWstring(const std::string &s)  
{  
	std::wstring ws;  
	wchar_t* wp = StringToWchar(s);  
	ws.append(wp);  
	return ws;  
}  

inline std::string WStringTostring(const std::wstring& strIn)
{
	int nBufSize = ::WideCharToMultiByte(GetACP(), 0, strIn.c_str(), -1, NULL, 0, 0, FALSE);
	char* szBuf = new char[nBufSize];
	::WideCharToMultiByte(GetACP(), 0, strIn.c_str(), -1, szBuf, nBufSize, 0, FALSE);

	std::string strRet(szBuf);
	delete [] szBuf;
	szBuf = NULL;

	return strRet;
}

inline bool HasChinese(const char *str)
{
	char c;
	while(1)
	{
		c = *str++;
		if (c == 0) //null
		{
			break;  
		}

		if (c < 0 || c > 127 || c == '?')
		{
			return TRUE;
		}
   }

   return FALSE;
}

inline std::wstring HtmlEncode(std::wstring unencoded)
{
	std::wstring result = unencoded;
	ReplaceAll(result, L"\n", L"<br>");
	ReplaceAll(result, L"&", L"&amp;");
	ReplaceAll(result, L"<", L"&lt;");
	ReplaceAll(result, L">", L"&gt;");
	ReplaceAll(result, L" ", L"&nbsp;");
	ReplaceAll(result, L"\'", L"&#39;");
	ReplaceAll(result, L"\"", L"&quot;");
	return result;
}

inline std::wstring HtmlDecode(std::wstring encoded)
{
	std::wstring result = encoded;
	ReplaceAll(result, L"<br>", L"\n");
	ReplaceAll(result, L"&amp;", L"&");
	ReplaceAll(result, L"&lt;", L"<");
	ReplaceAll(result, L"&gt;", L">");
	ReplaceAll(result, L"&nbsp;", L" ");
	ReplaceAll(result, L"&#39;", L"\'");
	ReplaceAll(result, L"&quot;", L"\"");
	return result;
	return result;
}



