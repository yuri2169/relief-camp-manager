#ifndef RELIEF_DEBUG_H
#define RELIEF_DEBUG_H

#include <iostream>

// Build with `make debug` (-DRELIEF_DEBUG) to watch the object lifecycle.
#ifdef RELIEF_DEBUG
#define RELIEF_LIFECYCLE(msg) std::cout << "[lifecycle] " << msg << std::endl
#else
#define RELIEF_LIFECYCLE(msg) ((void)0)
#endif

#endif
