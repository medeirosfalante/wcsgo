#include "remotecall.hpp"
#include "log.hpp"

#define FINDPATTERN_CHUNKSIZE 0x1000



namespace remotecall {

    void* MapModuleMemoryRegion::find(Handle handle, const char* data, const char* pattern) {
         char buffer[FINDPATTERN_CHUNKSIZE];

        size_t len = strlen(pattern);
        size_t chunksize = sizeof(buffer);
        size_t totalsize = this->end - this->start;
        size_t chunknum = 0;

         while(totalsize) {
            size_t readsize = (totalsize < chunksize) ? totalsize : chunksize;
            size_t readaddr = this->start + (chunksize * chunknum);
            bzero(buffer, chunksize);

            if(handle.Read((void*) readaddr, buffer, readsize)) {
                 for(size_t b = 0; b < readsize; b++) {
                    size_t matches = 0;
                    while(buffer[b + matches] == data[matches] || pattern[matches] != 'x') {
                         matches++;
                         if(matches == len) {
                              return (char*) (readaddr + b);
                         }

                    }
                 }
            }
            totalsize -= readsize;
            chunknum++;
         }
         return NULL;
    }

};