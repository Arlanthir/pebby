#ifndef COMMON_H
#define COMMON_H

//#define DEBUG_BUILD

#ifdef DEBUG_BUILD
#define LOG(...) APP_LOG(__VA_ARGS__)
#else
#define LOG(...)
#endif
	
#define MAX(a, b) (( a > b)? a : b)
	
#define PERSIST_BOTTLE 1
#define PERSIST_DIAPER 2
#define PERSIST_MOON_START 3
#define PERSIST_MOON_END 4
#define PERSIST_LOG 5

#endif // COMMON_H
