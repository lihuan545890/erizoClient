#pragma once

#include <functional>
#include <map>
#include <string>

class Event
{
private:
	std::map<std::string, std::function<void()>> eventHandlers;

public:
	virtual ~Event();
	void addEventHandler(const std::string& event, const std::function<void()>& eventHandler);
	void dispatchEvent(const std::string& event);
};
