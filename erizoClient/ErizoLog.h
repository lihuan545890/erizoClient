#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <windows.h>

#define ERIZOLOG_PREFIX		{	SYSTEMTIME systime; \
								GetLocalTime(&systime); \
								erizoLog << std::to_string(systime.wHour) + "h:" + \
								std::to_string(systime.wMinute) + "m:" + \
								std::to_string(systime.wSecond) + "s:" + \
								std::to_string(systime.wMilliseconds) + "ms		" + \
								__FUNCTION__ << "		"; }

#define ERIZOLOG_TRACE	ERIZOLOG_PREFIX \
						erizoLog << "TRACE:		"

#define ERIZOLOG_DEBUG	ERIZOLOG_PREFIX \
						erizoLog << "DEBUG:		"

#define ERIZOLOG_INFO	ERIZOLOG_PREFIX \
						erizoLog << "INFO:		"

#define ERIZOLOG_WARN	ERIZOLOG_PREFIX \
						erizoLog << "WARN:		"

#define ERIZOLOG_ERROR	ERIZOLOG_PREFIX \
						erizoLog << "ERROR:		"


class ErizoLog
{
private:
	std::ofstream* file;
	std::mutex mtx;

public:
	ErizoLog();
	~ErizoLog();

	template <typename T>
	ErizoLog& operator<<(const T& x)
	{
		std::lock_guard<std::mutex> lock(mtx);
		(*file) << x;
		return *this;
	}

	// for endl operation
	ErizoLog& operator<<(std::ostream& (*os)(std::ostream&))
	{
		std::lock_guard<std::mutex> lock(mtx);
		(*file) << os;
		return *this;
	}
};

extern ErizoLog erizoLog;
