#pragma once

#include "..\..\includes.hpp"
#include <deque>

class eventlogs : public singleton <eventlogs>
{
public:
	void paint_traverse();
	void events(IGameEvent* event);
	void add(std::string text, Color color,bool full_display = true);

	bool last_log = false;
private:
	struct loginfo_t
	{
		loginfo_t(float log_time, std::string message, const Color& color)  //-V818
		{
			this->log_time = log_time;
			this->message = message; //-V820
			this->color = color;

			x = 0.0f;
			y = 0.0f;
			maxsize = 0.f;
			maxsize2 = 0.f;
		}
		bool yes = false;
		float maxsize, maxsize2, textsize, maxsize3;
		float log_time;
		std::string message;
		Color color;
		float x, y;
	};

	std::deque <loginfo_t> logs;
};