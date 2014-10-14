#ifndef QUERY_H
#define QUERY_H

//Query delay times
extern int
	QTimeX,
	QTimeLogin,
	QTimeLoad,
	QTimeConfig,
	QTimeTime;
//Query frequency times
extern int
	QFreqX,
	QFreqLogin,
	QFreqLoad,
	QFreqConfig;

//XServer idleness
void query_x(int);
//Login status
void query_login();
//Processor load
void query_load();
//Configuration
void query_config();
//Time zone restriction
int query_time();

#endif
