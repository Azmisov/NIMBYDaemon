#include "config.h"

//Configuration defaults; see config JSON file for details
int EnableDaemon = 1,
	EnableTimeZone = 0,
	EnableLoginIdle = 1,
	EnableLogoutIdle = 1,
	OnlyWeekends = 0,
	OnlyWeekdays = 1,
	IdleLoginTime = 600,
	IdleLogoutTime = 60,
	BusyTime = 7;
float IdleProcessorLoad = 1.0,
	TimeStart = 20.5,
	TimeEnd = 8.0;

//Derivative global settings
float sys_idle_avg = -1,
	sys_idle_avg_decay = .93,
	sys_load = 1.0;
int sys_idle_time = 0,
	sys_login = 0,
	sys_valid_time = 0;

//Global cURL handler
CURL *curl;

//Global X11 handler
Display *display;
