#include "wall.hpp"


struct iovec g_remote[1024], g_local[1024];



struct wall::GlowObjectDefinition_t g_glow[1024];



int cachedSpottedAddress = -1;
int count = 0;