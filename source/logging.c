#include "logging.h"

void error_log(const char *format, ...){
	FILE *flog;
	//If the log file is over 3.5 megabytes, empty it and start again
	struct stat st;
	if (!stat(log_filename, &st) && st.st_size >= 36700160) //1048576 per megabyte
		flog = fopen(log_filename, "w+");
	else flog = fopen(log_filename, "a+");
	//Write to the log file
	if (!flog) fprintf(stderr, "Error! Could not open log file for writing!\n");
	else{
		//Get time of error
		time_t raw;
		struct tm *t;
		time(&raw);
		t = localtime(&raw);
		fprintf(flog, "[%d/%d %d:%d:%d] ", t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
		//Write error
		va_list args;
		va_start(args, format);
		vfprintf(flog, format, args);
		va_end(args);
		fclose(flog);
	}
}
