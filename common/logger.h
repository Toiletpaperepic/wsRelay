#if !defined(LOGGER_COMPILE_OUT)
#define LOGGER_COMPILE_OUT 1
#endif

#include <stdio.h>

#if LOGGER_COMPILE_OUT 
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

void setuplogger();
struct allowed_log_types* getalt();

#define print(format, ...) printf(format "\n", ##__VA_ARGS__);
#define stderrprint(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__);

#define ERROR(format, ...) if(getalt()->error) {stderrprint(RED "[error] " RESET format, ##__VA_ARGS__)}
#define WARN(format, ...) if(getalt()->warn) {stderrprint(YEL "[warn] " RESET format, ##__VA_ARGS__)}
#define INFO(format, ...) if(getalt()->info) {stderrprint(BLU "[info] " RESET format, ##__VA_ARGS__)}
#define DEBUG(format, ...) if(getalt()->debug) {stderrprint(CYN "[debug] " RESET format, ##__VA_ARGS__)}
#define TRACE(format, ...) if(getalt()->trace) {stderrprint(MAG "[trace] " RESET format, ##__VA_ARGS__)}
#else
#define print(format, ...) printf(format "\n", ##__VA_ARGS__);
#define stderrprint(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__);

#define ERROR(format, ...) stderrprint(format, ##__VA_ARGS__)
#define WARN(format, ...) stderrprint(format, ##__VA_ARGS__)
#define INFO(format, ...) stderrprint(format, ##__VA_ARGS__)
#define DEBUG(format, ...) 
#define TRACE(format, ...) 
#endif