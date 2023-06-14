// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "./include/logger.h"

LOG_LEVEL current_level = DEBUG;

void setLogLevel(LOG_LEVEL newLevel)
{
    if (newLevel >= DEBUG && newLevel <= FATAL)
        current_level = newLevel;
}

char *levelDescription(LOG_LEVEL level)
{
    static char *description[] = {"DEBUG", "INFO", "ERROR", "FATAL"};
    if (level < DEBUG || level > FATAL)
        return "";
    return description[level];
}
