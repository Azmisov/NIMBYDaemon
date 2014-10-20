#include <stdio.h>
#include <string.h>
#include "query.h"
#include "comms.h"
#include "config.h"
#include "logging.h"

//Random stuff
inline int nimby_set(int);
char hostname[50];
char ip_address[16];
char log_filename[512];

/*
	NIMBY Toggler Daemon:
	This program queries the system status periodically to determine if it is
	not being used. If it is not used, it will add the computer to the Tractor farm.
	See the server configuration file for more details.
*/
int main(int argc, char *argv[]){
	//NIMBY state; 3 = unset, 2 = default, 1 = on, 0 = off
	int nimby_state = 3;

	//Default log file location
	strcpy(log_filename, "/opt/pixar/NIMBYDaemon.log");

	//Setup for HTTP requests
	int err_num = 0;
	while (1){
		//Try getting system IP address every 6 sec
		if (get_ipaddress(ip_address)){
			if (++err_num % 10 == 0)
				error_log("Failed to get system IP address for %d minutes\n", err_num/10);
			sleep(6);
		}
		else break;
	}
	get_hostname(hostname, 50);
	curl = open_curl();
	
	//Make sure we can query X for idleness before running the daemon
	err_num = 0;
	while (1){
		if (!(display = XOpenDisplay(NULL))){
			if (++err_num % 10 == 0)
				error_log("XServer has not been running for %d minutes\n", err_num/10);
			sleep(6);
		}
		else break;
	}
	int event_base, error_base;
	if (!XScreenSaverQueryExtension(display, &event_base, &error_base)){
		error_log("XScreenSaver extension not present\n");
		return 3;
	}
	XCloseDisplay(display);
	
	/// QUERY LOOP
	int i, sleep_delay = 1, new_sleep_delay, new_nimby_state;
	while (1){
		//Query new system state
		new_nimby_state = 2;
		//If daemon is disabled, only check config file
		if ((QTimeConfig -= sleep_delay) <= 0){
			query_config();
			QTimeConfig = QFreqConfig;
		}
		new_sleep_delay = QTimeConfig;
		if (EnableDaemon){
			new_nimby_state = 1;
			//Check time zone
			if (EnableTimeZone || OnlyWeekends){
				if ((QTimeTime -= sleep_delay) <= 0)
					query_time();
				if (new_sleep_delay > QTimeTime)
					new_sleep_delay = QTimeTime;
			}
			//If valid time zone, check other parameters
			if (!(EnableTimeZone || OnlyWeekends) || sys_valid_time){
				//Check login information
				//By default, we only disable NIMBY when nobody is logged in
				if ((QTimeLogin -= sleep_delay) <= 0){
					query_login();
					QTimeLogin = QFreqLogin;
				}
				if (new_sleep_delay > QTimeLogin)
					new_sleep_delay = QTimeLogin;
				new_nimby_state = sys_login;
				//Idle detection!
				int idle_login = EnableLoginIdle && sys_login,
					idle_logout = EnableLogoutIdle && !sys_login;
				if (idle_login || idle_logout){
					//Query X11 idleness
					if ((QTimeX -= sleep_delay) <= 0){
						query_x(sleep_delay);
						QTimeX = QFreqX;
					}
					if (new_sleep_delay > QTimeX)
						new_sleep_delay = QTimeX;
					//If we detect user interaction, enable NIMBY, if it was off
					if (!nimby_state)
						new_nimby_state = sys_idle_avg <= 0;
					else{
						//Login idleness
						if (idle_login){
							//Also needs to know processor load, in case user is rendering in background
							if (nimby_state && (QTimeLoad -= sleep_delay) <= 0){
								query_load();
								QTimeLoad = QFreqLoad;
							}
							if (new_sleep_delay > QTimeLoad)
								new_sleep_delay = QTimeLoad;
							new_nimby_state = sys_idle_avg <= 0 || sys_idle_time < IdleLoginTime || sys_load >= IdleProcessorLoad;
						}
						//Logout idleness
						else if (idle_logout)
							new_nimby_state = sys_idle_avg <= 0 || sys_idle_time < IdleLogoutTime;
					}
				}
			}
		}

		//NIMBY needs to be changed; if request succeeds, change the state
		if (new_nimby_state != nimby_state){
			if (nimby_set(new_nimby_state))
				nimby_state = new_nimby_state;
		}

		//Decrement queue times and sleep
		sleep_delay = new_sleep_delay;
		sys_idle_time += sleep_delay;
		if (VERBOSE)
			error_log("---- SLEEPING %d ----\n", sleep_delay);
		sleep(sleep_delay);
	}

	close_curl(curl);
	error_log("Unexpected termination of daemon\n");
	return 4;
}

/*
	Toggles tractor blade's NIMBY option
*/
inline int nimby_set(int new_nimby_state){
	//First, login to the tractor engine
	String *chunk = request_curl(curl, "http://tractor-engine/Tractor/monitor?q=login&user=admin");
	if (chunk == NULL)
		return 0;
	//We need to extract the login id from the JSON response
	cJSON *root = cJSON_Parse(chunk->memory);
	if (!root){
		error_log("Failed to parse tractor login JSON [%s]\n%s\n", cJSON_GetErrorPtr(), chunk->memory);
		string_free(chunk);
		return 0;
	}
	cJSON *tsid_json = cJSON_GetObjectItem(root, "tsid");
	if (!tsid_json){
		error_log("Failed to find tractor login tsid\n%s\n", chunk->memory);
		cJSON_Delete(root);
		string_free(chunk);
		return 0;
	}
	//Copy id into local memory, so we can delete the json object
	char tsid[35];
	memcpy(tsid, tsid_json->valuestring, sizeof(char)*strlen(tsid_json->valuestring));
	cJSON_Delete(root);
	string_free(chunk);

	//Okay, now we need to send the NIMBY request
	char req_buffer[256];
	char nimby[] = "default";
	if (new_nimby_state != 2){
		nimby[0] = new_nimby_state + 48;
		nimby[1] = '\0';
	}
	sprintf(
		req_buffer,
		"http://tractor-engine/Tractor/ctrl?q=battribute&b=%s%%2F%s%%3A9005&tsid=%s&nimby=%s",
		hostname, ip_address, tsid, nimby 
	);
	chunk = request_curl(curl, req_buffer);
	if (chunk == NULL)
		return 0;
	string_free(chunk);
	
	//If we are enabling NIMBY, we need to kick off any farm jobs
	if (new_nimby_state != 0){
		sprintf(
			req_buffer,
			"http://tractor-engine/Tractor/queue?q=ejectall&blade=%s%%2F%s&tsid=%s",
			hostname, ip_address, tsid
		);
		chunk = request_curl(curl, req_buffer);
		if (chunk == NULL)
			return 0;
		string_free(chunk);
	}
	return 1;
}
