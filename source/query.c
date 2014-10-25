#include <math.h>
#include <utmpx.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "comms.h"
#include "config.h"
#include "logging.h"

//Query delay times
int QTimeX = 0,
	QTimeLogin = 0,
	QTimeLoad = 0,
	QTimeConfig = 0,
	QTimeTime = 0;
//Query frequency times
//No need for QFreqTime, since it is set manually in query_time()
int QFreqX = 4,
	QFreqLogin = 20,
	QFreqLoad = 30,
	QFreqConfig = 600;

//Reset config for x11, load, and other queries
void query_reset_idle(){
	sys_idle_time = 0;
	sys_idle_avg = -1.0;
	sys_load = IdleProcessorLoad;
	QTimeX = 0;
	QTimeLoad = 0;
		
}
void query_reset_login(){
	sys_login = 1;
	QTimeLogin = 0;
}

//Update XServer idleness
void query_x(int dt){
	//Open XDisplay and query for idleness
	int err_num = 0;
	if (!(display = XOpenDisplay(NULL))){
		error_log("X server shutdown; stopping daemon\n");
		exit(7);
	}			
	XScreenSaverInfo info;
	XScreenSaverQueryInfo(display, DefaultRootWindow(display), &info);
	float idle_time = (float) info.idle/1000.0f;
	XCloseDisplay(display);

	//Running average of idleness
	int idle = idle_time < dt ? -1 : 1, i;
	float shift = idle*(1-sys_idle_avg_decay);
	for (i=0; i<dt; i++){
		sys_idle_avg *= sys_idle_avg_decay;
		sys_idle_avg += shift;
	}
	//If running average drops below 0, there was sustained user
	//interaction for the past BUSY_TIME seconds (approximately)
	if (sys_idle_avg <= 0)
		sys_idle_time = idle_time;
	//Debug x query
	if (VERBOSE)
		error_log("QueryX = %f, %f, %d\n", idle_time, sys_idle_avg, sys_idle_time);
}

//Update login status
void query_login(){
	sys_login = 0;
	struct utmpx *sys_usr;
	while ((sys_usr = getutxent())){
		char c0 = sys_usr->ut_line[0];
		int active = !(c0 == '~' && !sys_usr->ut_line[1]) && c0 != '{' && c0 != '|';
		if (sys_usr->ut_type == USER_PROCESS && active){
			sys_login = 1;
			break;
		}
	}
	endutxent();
	//Debug login query
	if (INVERT_LOGIN)
		sys_login = !sys_login;
	if (VERBOSE)
		error_log("QueryLogin = %d\n", sys_login);
}

//Update processor load
void query_load(){
	struct sysinfo sys_info;
	sysinfo(&sys_info);
	sys_load = sys_info.loads[0]/65536.0f;
	//Debug load query
	if (VERBOSE)
		error_log("QueryLoad = %f\n", sys_load);
}

//Update system time
void query_time(){
	time_t raw;
	struct tm *time_info;
	time(&raw);
	time_info = localtime(&raw);
	int is_weekend = time_info->tm_wday == 0 || time_info->tm_wday == 6;
	//Convert time to a float value (in hours)
	float ftime = time_info->tm_hour + (time_info->tm_min + (time_info->tm_sec)/60.0f)/60.0f;
	//Day of the week restrictions
	if (OnlyWeekends && !is_weekend){
		sys_valid_time = 0;
		QTimeTime = (5-time_info->tm_wday)*86400 + (24-ftime)*3600 + 10;
		if (VERBOSE)
			error_log("QueryTime = Daemon disabled until weekend (%d seconds)\n", QTimeTime);
		goto DISABLE_RESET;
	}
	//If we don't enable time restrictions, don't do any calculations
	if (!EnableTimeZone){
		sys_valid_time = 1;
		if (!OnlyWeekends)
			QTimeTime = 1;
		if (VERBOSE)
			error_log("QueryTime = Time zone restriction disabled\n");
		return;
	}
	//No time restrictions on weekends
	if (OnlyWeekdays && is_weekend){
		sys_valid_time = 1;
		QTimeTime = (24-ftime)*3600 + 10;
		if (time_info->tm_wday == 6)
			QTimeTime += 86400;
		if (VERBOSE)
			error_log("QueryTime = Time zone restriction disabled for weekend (%d seconds)\n", QTimeTime);
		return;
	}
	//Time bounds do not wrap; from 0+ to 24-
	if (TimeEnd > TimeStart)
		sys_valid_time = ftime > TimeStart && ftime < TimeEnd;
	//Time bounds wrap around; from 24- to 0+
	else sys_valid_time = ftime > TimeStart || ftime < TimeEnd;
	//Recompute delay until we need to check time again
	float new_delay;
	if (sys_valid_time)
		new_delay = TimeEnd - ftime;
	else new_delay = TimeStart - ftime;
	if (new_delay < 0)
		new_delay += 24;
	QTimeTime = new_delay*3600 + 10;
	//Debug query time
	if (VERBOSE)
		error_log("QueryTime = %d, %d\n", sys_valid_time, QTimeTime);
	if (sys_valid_time) return;

	DISABLE_RESET:
	query_reset_idle();
	query_reset_login();
}

//Update configuration
void query_config(){
	//Fetch JSON config file from the tractor engine website
	String *chunk = request_curl(curl, "http://tractor-engine/NIMBYDaemon.config");
	if (chunk != NULL){
		//Minify the JSON first
		cJSON_Minify(chunk->memory);
		cJSON *config = cJSON_Parse(chunk->memory);
		if (config){
			cJSON *obj;
			//Log file location
			obj = cJSON_GetObjectItem(config, "LogFile");
			if (!obj) error_log("Cannot find LogFile in JSON config\n");
			else if (obj->valuestring){
				//Make this is a valid file name
				FILE *chk = fopen(obj->valuestring, "a+");
				if (!chk) error_log("Invalid log filename specified in JSON config\n");
				else strcpy(log_filename, obj->valuestring);
			}
			//Config file refresh frequency
			obj = cJSON_GetObjectItem(config, "QFreqConfig");
			if (!obj) error_log("Cannot find QFreqConfig in JSON config\n");
			else if (obj->valueint > 0)
				QFreqConfig = obj->valueint;
			//Daemon enabled
			obj = cJSON_GetObjectItem(config, "EnableDaemon");
			if (!obj) error_log("Cannot find EnableDaemon in JSON config\n");
			else EnableDaemon = obj->valueint != 0;
			//Daemon enabled only on weekends
			obj = cJSON_GetObjectItem(config, "OnlyWeekends");
			if (!obj) error_log("Cannot find OnlyWeekends in JSON config\n");
			else OnlyWeekends = obj->valueint != 0;
			if (EnableDaemon){
				//Time zone enabled
				obj = cJSON_GetObjectItem(config, "EnableTimeZone");
				if (!obj) error_log("Cannot find EnableTimeZone in JSON config\n");
				else EnableTimeZone = obj->valueint != 0;
				if (EnableTimeZone){
					//Time zone start
					obj = cJSON_GetObjectItem(config, "TimeStart");
					if (!obj) error_log("Cannot find TimeStart in JSON config\n");
					else{
						float x = obj->valuedouble;
						if (x < 0 || x >= 24) x = 0;
						TimeStart = x;
					}
					//Time zone end
					obj = cJSON_GetObjectItem(config, "TimeEnd");
					if (!obj) error_log("Cannot find TimeEnd in JSON config\n");
					else{
						float x = obj->valuedouble;
						if (x < 0 || x >= 24) x = 0;
						TimeEnd = x;
					}
					//Time zone only applies to weekdays
					obj = cJSON_GetObjectItem(config, "OnlyWeekdays");
					if (!obj) error_log("Cannot find OnlyWeekdays in JSON config\n");
					else OnlyWeekdays = obj->valueint != 0;
				}
				//Login check frequency
				obj = cJSON_GetObjectItem(config, "QFreqLogin");
				if (!obj) error_log("Cannot find QFreqLogin in JSON config\n");
				else if (obj->valueint > 0)
					QFreqLogin = obj->valueint;
				//Login idle enabled
				obj = cJSON_GetObjectItem(config, "EnableLoginIdle");
				if (!obj) error_log("Cannot find EnableLoginIdle in JSON config\n");
				else EnableLoginIdle = obj->valueint != 0;
				//Logout idle enabled
				obj = cJSON_GetObjectItem(config, "EnableLogoutIdle");
				if (!obj) error_log("Cannot find EnableLogoutIdle in JSON config\n");
				else EnableLogoutIdle = obj->valueint != 0;
				//X11 Idle time settings
				if (EnableLoginIdle || EnableLogoutIdle){
					obj = cJSON_GetObjectItem(config, "BusyTime");
					if (!obj) error_log("Cannot find BusyTime in JSON config\n");
					else if (obj->valueint > 0)
						BusyTime = obj->valueint;
					//XServer query frequency
					obj = cJSON_GetObjectItem(config, "QFreqX");
					if (!obj) error_log("Cannot find QFreqX in JSON config\n");
					else if (obj->valueint > 0)
						QFreqX = obj->valueint;
				}
				//Login specific idle stuff
				if (EnableLoginIdle){
					//Processor load
					obj = cJSON_GetObjectItem(config, "IdleProcessorLoad");
					if (!obj) error_log("Cannot find IdleProcessorLoad in JSON config\n");
					else if (obj->valuedouble > 0)
						IdleProcessorLoad = obj->valuedouble;
					//Processor load frequency
					obj = cJSON_GetObjectItem(config, "QFreqLoad");
					if (!obj) error_log("Cannot find QFreqLoad in JSON config\n");
					else if (obj->valueint > 0)
						QFreqLoad = obj->valueint;
					//Login idle time
					obj = cJSON_GetObjectItem(config, "IdleLoginTime");
					if (!obj) error_log("Cannot find IdleLoginTime in JSON config\n");
					else if (obj->valueint > 0)
						IdleLoginTime = obj->valueint;
				}
				//Logout specific idle stuff
				if (EnableLogoutIdle){
					//Logout idle time
					obj = cJSON_GetObjectItem(config, "IdleLogoutTime");
					if (!obj) error_log("Cannot find IdleLogoutTime in JSON config\n");
					else if (obj->valueint > 0)
						IdleLogoutTime = obj->valueint;
				}
			}
			cJSON_Delete(config);
		}
		else error_log("Failed to parse JSON config file [%s]", cJSON_GetErrorPtr());
		string_free(chunk);
	}

	//Approximate solution to x^(BUSY_TIME + 1) + x < 1
	//This comes from the running average formula: x^(BUSY_TIME) + SUM(i=0 to BUSY_TIME, (1-x)x^i)
	//I did a regression on some values and came up with this formula (accurate to about .001)
	float x_pow = pow(BusyTime+1, 0.9697290257);
	float sys_idle_avg_decay = x_pow / (x_pow + 0.635406913);
	//We actually want to be slightly smaller than BUSY_TIME
	sys_idle_avg_decay -= .01;

	//We also need to update the time zone information, if it changed
	if (TimeEnd == TimeStart)
		EnableTimeZone = 0;
	QTimeTime = 0;

	//Reset stuff, if daemon is disabling
	if (!(EnableLoginIdle || EnableLogoutIdle) || !EnableDaemon)
		query_reset_idle();
	if (!EnableDaemon)
		query_reset_login();

	//Debug config
	if (VERBOSE)
		error_log("QueryConfig = %f\n", sys_idle_avg_decay);
}
