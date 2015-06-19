#pragma once
#include <map>
#include <list>


class IEventReceiver
{
public:
	virtual int OnEvent(std::wstring evt, const void* para) = 0;

protected:
	int BindEvent(std::wstring evt);
	int UnbindEvent(std::wstring evt);
};


class EventSender
{
public:
	static int AddEventReceiver(std::wstring evt, IEventReceiver* receiver);
	static int RemoveEventReceiver(std::wstring evt, IEventReceiver* receiver);
	static int FireEvent(std::wstring evt, const void* para);

private:
	static std::map<std::wstring, std::list<IEventReceiver*>> eventReveivers_; 
};

struct EventPara_update_statusbar
{
	std::wstring title; 
	std::wstring subtitle; 
	std::wstring query; 
	std::wstring copyright; 
	std::wstring desc;
	std::wstring relatedQuery;
	bool bPreEnabled; 
	bool bNextEnabled; 
	bool bPlaying; 
	bool bLikeEnabled; 
	bool bLiked;
};

struct EventPara_setting_onchange
{
	std::wstring eleId; 
	bool value;
};

struct EventPara_search_cand_show
{
	std::wstring query; 
	std::wstring label; 
};

/***********************************************************/
inline int IEventReceiver::BindEvent(std::wstring evt)
{
	return EventSender::AddEventReceiver(evt, this);
}


inline int IEventReceiver::UnbindEvent(std::wstring evt)
{
	EventSender::RemoveEventReceiver(evt, this);
	return 0;
}

/***********************************************************/
inline int EventSender::AddEventReceiver(std::wstring evt, IEventReceiver* receiver)
{
	//if evt not exist in map, init map
	auto iter = eventReveivers_.find(evt);
	if (iter == eventReveivers_.end())
	{
		eventReveivers_[evt];
	}

	//if already exist, return
	auto iter2 = eventReveivers_[evt].begin();
	for(; iter2!=eventReveivers_[evt].end(); iter2++)
	{
		if (*iter2 == receiver)
		{
			return 0;
		}
	}

	//if not exist, add 
	if (iter2 == eventReveivers_[evt].end())
	{
		eventReveivers_[evt].push_back(receiver);
	}
	return 0;
}


inline int EventSender::RemoveEventReceiver(std::wstring evt, IEventReceiver* receiver)
{
	//if evt not exist in map, return
	auto iter = eventReveivers_.find(evt);
	if (iter == eventReveivers_.end())
	{
		return 0;
	}

	//if exist, remove
	auto iter2 = eventReveivers_[evt].begin();
	for(; iter2!=eventReveivers_[evt].end(); iter2++)
	{
		if (*iter2 == receiver)
		{
			eventReveivers_[evt].erase(iter2);
			if (eventReveivers_[evt].size() == 0)
			{
				eventReveivers_.erase(iter);
			}
			break;
		}
	}
	
	return 0;
}


inline int EventSender::FireEvent(std::wstring evt, const void* para)
{
	//if not exist, return
	auto iter = eventReveivers_.find(evt);
	if (iter == eventReveivers_.end())
	{
		return 0;
	}

	//if exist, fire
	for(auto iter2 = eventReveivers_[evt].begin(); iter2!= eventReveivers_[evt].end(); iter2++)
	{
		(*iter2)->OnEvent(evt, para);
	}
	
	return 0;
}
