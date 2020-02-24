#include "Event.h"

#include "ErizoLog.h"

using namespace std;

Event::~Event()
{
}

void Event::addEventHandler(const string& event, const function<void()>& eventHandler)
{
	if (eventHandler)
	{
		ERIZOLOG_TRACE << "addEventHandler for the event: " << event << endl;
		eventHandlers[event] = eventHandler;
	}
	else
	{
		ERIZOLOG_WARN << "addEventHandler: the event handler is empty" << endl;
	}
}

void Event::dispatchEvent(const string& event)
{
	if (eventHandlers[event])
	{
		eventHandlers[event]();
	}
	else
	{
		ERIZOLOG_WARN << "dispatchEvent: the event handler for event: " << event << " is empty..." << endl;
	}
}
