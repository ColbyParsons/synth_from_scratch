#pragma once

#include <fstream>

#ifdef NO_DEBUG
    #define DEBUG(x)
#else
    #define DEBUG(x) x
    #define DEBUG_LOG(x) debugLog << x << std::endl;
# endif

std::ofstream debugLog;

void initDebug() {
    DEBUG( debugLog.open( "log.txt" ); )
}
