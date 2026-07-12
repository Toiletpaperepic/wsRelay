#ifndef COMMON_LOGGER_H
#define COMMON_LOGGER_H

#if !defined(LOGGER_COMPILE_OUT)
#define LOGGER_COMPILE_OUT 0
#endif

#include "dll_export.h"
#include <stdio.h>

#if LOGGER_COMPILE_OUT != 1
#include <stdbool.h>

struct allowed_log_types {
    bool error;
    bool warn;
    bool info;
    bool debug;
    bool trace;
};

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

DLL_EXPORT void setuplogger();
DLL_EXPORT struct allowed_log_types* getalt();

#define print(format, ...) printf(format "\n", ##__VA_ARGS__);
#define stderrprint(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__);

#define error(format, ...) if(getalt()->error) {stderrprint(RED "[error] (%s) " RESET format ,__func__, ##__VA_ARGS__)}
#define warn(format, ...) if(getalt()->warn) {stderrprint(YEL "[warn] (%s) " RESET format ,__func__, ##__VA_ARGS__)}
#define info(format, ...) if(getalt()->info) {stderrprint(BLU "[info] (%s) " RESET format ,__func__, ##__VA_ARGS__)}
#define debug(format, ...) if(getalt()->debug) {stderrprint(CYN "[debug] (%s) " RESET format ,__func__, ##__VA_ARGS__)}
#define trace(format, ...) if(getalt()->trace) {stderrprint(MAG "[trace] (%s) " RESET format ,__func__, ##__VA_ARGS__)}
#else
#define print(format, ...) printf(format "\n", ##__VA_ARGS__);
#define stderrprint(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__);

#define error(format, ...) stderrprint(format, ##__VA_ARGS__)
#define warn(format, ...) stderrprint(format, ##__VA_ARGS__)
#define info(format, ...) stderrprint(format, ##__VA_ARGS__)
#define debug(format, ...) 
#define trace(format, ...) 
#endif
#endif
