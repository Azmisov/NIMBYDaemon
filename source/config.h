#ifndef CONFIG_H
#define CONFIG_H

#include <curl/curl.h>
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

//Use these for debugging
#define VERBOSE 0
#define INVERT_LOGIN 0

//Configuration options from the JSON config file
//See the JSON file for documentation on what each means
extern int
	EnableDaemon,
	EnableTimeZone,
	EnableLoginIdle,
	EnableLogoutIdle,
	OnlyWeekends,
	OnlyWeekdays,
	IdleLoginTime,
	IdleLogoutTime,
	BusyTime;
extern float
	IdleProcessorLoad,
	TimeStart,
	TimeEnd;

//Other global settings, derived from the config file
extern float
	sys_idle_avg,			//running average of system "idleness"; between -1 and 1, > 0 means system is idle
	sys_idle_avg_decay,		//how quickly the sys_idle_avg decays; this is computed based on BusyTime's value
	sys_load;				//processor load; average processes running in last minute
extern int
	sys_idle_time,			//how long system has been idle for (seconds)
	sys_login,				//whether a user is logged in or not (bool)
	sys_valid_time;			//whether it is a valid time zone (for time zone restrictions) (bool)

//Global cURL handler
extern CURL *curl;

//Global X11 handler
extern Display *display;

//Log file location
extern char log_filename[512];

#endif
