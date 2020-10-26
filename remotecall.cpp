#include "remotecall.hpp"
#include "log.hpp"

#define FINDPATTERN_CHUNKSIZE 0x1000



namespace remotecall {

    bool Handle::IsValid() {
        return (pid != -1);
    }

    //get call address in memory
    unsigned long Handle::GetCallAddress(void* address) {
        unsigned long code = 0;

        if(Read((char*) address + 1, &code, sizeof(unsigned int))) {
            return code + (unsigned long) address + 5;
        }

        return 0;
    }

    // read data in memory
    bool Handle::Read(void* address, void* buffer, size_t size) {
        struct iovec local[1];
        struct iovec remote[1];

        local[0].iov_base = buffer;
        local[0].iov_len = size;
        remote[0].iov_base = address;
        remote[0].iov_len = size;

        return (process_vm_readv(pid, local, 1, remote, 1, 0) == size);
    }

    // write info in memory
     bool Handle::Write(void* address, void* buffer, size_t size) {
        struct iovec local[1];
        struct iovec remote[1];

        local[0].iov_base = buffer;
        local[0].iov_len = size;
        remote[0].iov_base = address;
        remote[0].iov_len = size;

        return (process_vm_writev(pid, local, 1, remote, 1, 0) == size);
    }


    // check processing running
    bool Handle::IsRunning() {
        if(!IsValid())
            return false;

        struct stat sts;
        return !(stat(("/proc/" + pidStr).c_str(), &sts) == -1 && errno == ENOENT);
    }


    std::string Handle::GetWorkDirectory() {
        return GetSymbolicLinkTarget(("/proc/" + pidStr + "/cwd"));
    }

    std::string Handle::GetPath() {
        return GetSymbolicLinkTarget(("/proc/" + pidStr + "/exe"));
    }

    Handle::Handle(pid_t target) {
        std::stringstream buffer;
        buffer << target;
        pid = target;
        pidStr = buffer.str();
    }
    Handle::Handle(std::string target) {
        // check if string is number
        if(strspn(target.c_str(), "0123456789") != target.size()) {
            pid = -1;
            pidStr.clear();
        } else {
            std::istringstream buffer(target);
            pidStr = target;
            buffer >> pid;
        }
    }

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