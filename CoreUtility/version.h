#pragma once

#include <stdint.h>
#include <string>
#include "string_utility.h"

//date of the version
//12/04/10->12/4/10
//And now, verion looks big. :-)
#define WALLPAPER_CL_MAJOR_VERSION 13
#define WALLPAPER_CL_MINOR_VERSION 4
#define WALLPAPER_CL_BUILD_MAJOR_VERSION 22
//don't use this version number
#define WALLPAPER_CL_BUILD_MINOR_VERSION 0

//windows style version
#define WALLPAPER_CL_BUILD_VERSION WALLPAPER_CL_BUILD_MAJOR_VERSION
#define WALLPAPER_CL_REVISION_VERSION WALLPAPER_CL_BUILD_MINOR_VERSION

class Version
{
public:
    Version()
        :major_(0), minor_(0), build_(0), revision_(0)
    {}
    Version(uint32_t major, uint32_t minor)
        :major_(major), minor_(minor), build_(0), revision_(0)
    {}
    Version(uint32_t major, uint32_t minor, uint32_t build)
        :major_(major), minor_(minor), build_(build), revision_(0)
    {}
    Version(uint32_t major, uint32_t minor, uint32_t build, uint32_t revision)
        :major_(major), minor_(minor), build_(build), revision_(revision)
    {}
    //default copy constructor and operator=
public:
    uint32_t get_major() const {return major_;}
    uint32_t get_minor() const {return minor_;}
    uint32_t get_build() const {return build_;}
    uint32_t get_revision() const {return revision_;}
    uint16_t get_major_revision() const {return (uint16_t)(revision_ >> 16);}
    uint32_t get_minor_revision() const {return (uint16_t)(revision_ & 0xffff);}
public:
    int compare(const Version& v) const
    {
        if (major_ != v.major_) {return compare(major_, v.major_);}
        if (minor_ != v.minor_) {return compare(minor_, v.minor_);}
        if (build_ != v.build_) {return compare(build_, v.build_);}
        return compare(revision_, v.revision_);
    }
    bool operator==(const Version& v) const 
    {
        return major_ == v.major_ && minor_ == v.minor_ && build_ == v.build_ && revision_ == v.revision_;
    }
    bool operator!=(const Version& v) const
    {
        return !(*this==v);
    }
    bool operator<(const Version& v) const
    {
        return this->compare(v) < 0;
    }
    bool operator<=(const Version& v) const
    {
        return compare(v) <= 0;
    }
    bool operator >(const Version& v) const
    {
        return v < *this;
    }
    bool operator >=(const Version& v) const
    {
        return v <= *this;
    }
public:
    std::wstring to_wstring() const
	{
		//return string_utility::format(L"{0}.{1}.{2}.{3}", major_, minor_, build_, revision_);
		std::wstring version = L"";

		TCHAR strMajor[10];
		memset(strMajor,0,10);
		_itow_s(major_, strMajor, 10);
		version.append(strMajor).append(L".");
		
		TCHAR strMinor[10];
		memset(strMinor,0,10);
		_itow_s(minor_, strMinor, 10);
		version.append(strMinor).append(L".");
		
		TCHAR strBuild[10];
		memset(strBuild,0,10);
		_itow_s(build_, strBuild, 10);
		version.append(strBuild).append(L".");
		
		TCHAR strRevision[10];
		memset(strRevision,0,10);
		_itow_s(revision_, strRevision, 10);

		return version;
	}

public:
    static bool try_parse(const std::wstring& str, Version& v)
	{
		auto items = Split<TCHAR>(str, L'.');
		if (items.size() < 2 && items.size() > 4) return false;

		//if (!convert::try_to_uint32(items[0], v.major_)) return false;
		//if (!convert::try_to_uint32(items[1], v.minor_)) return false;
		v.major_ = _wtoi(items[0].c_str());
		if(v.major_ == 0)
		{
			return false;
		}

		v.minor_ = _wtoi(items[1].c_str());
		v.build_ = 0;
		v.revision_ = 0;
		if (items.size() > 2)
		{
			//if (!convert::try_to_uint32(items[2], v.build_)) return false;
			v.build_ = _wtoi(items[2].c_str());
		}
		if (items.size() > 3)
		{
			//if (!convert::try_to_uint32(items[3], v.revision_)) return false;
			v.revision_ = _wtoi(items[3].c_str());
		}
		if(v.major_ + v.minor_ + v.build_ + v.revision_ == 0)
		{
			return false;
		}

		return true;
	}

    static Version parse(const std::wstring& str)
	{
		Version v;
		try_parse(str, v);
		return v;
	}

    static Version get_wallpaper_cl_version()
    {
        return Version(WALLPAPER_CL_MAJOR_VERSION, WALLPAPER_CL_MINOR_VERSION, WALLPAPER_CL_BUILD_VERSION, WALLPAPER_CL_REVISION_VERSION);
    }
public:
    static Version get_min_value()
    {
        return Version();
    }
    static Version get_max_value()
    {
        return Version(UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX);
    }
private:
    static int compare(uint32_t x, uint32_t y)
    {
        if (x > y) return 1;
        if (x < y) return -1;
        return 0;
    }
private:
    uint32_t major_;
    uint32_t minor_;
    uint32_t build_;
    uint32_t revision_;
};

