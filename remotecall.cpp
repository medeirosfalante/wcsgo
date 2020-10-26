#include "remotecall.hpp"
#include "log.hpp"

#define FINDPATTERN_CHUNKSIZE 0x1000



namespace remotecall {

     void Handle::ParseMaps() {
        regions.clear();
        std::ifstream maps("/proc/" + pidStr + "/maps");
         std::string line;
         while (std::getline(maps, line)) {
            std::istringstream iss(line);
            std::string memorySpace, permissions, offset, device, inode;
            if (iss >> memorySpace >> permissions >> offset >> device >> inode) {
                std::string pathname;
                for(size_t ls = 0, i = 0; i < line.length(); i++) {
                    if(line.substr(i, 1).compare(" ") == 0) {
                         ls++;
                         if(ls == 5) {
                             size_t begin = line.substr(i, line.size()).find_first_not_of(' ');
                            if(begin != -1) {
                                pathname = line.substr(begin + i, line.size());
                            } else {
                                pathname.clear();
                            }
                         }
                    }
                }
                MapModuleMemoryRegion region;
                size_t memorySplit = memorySpace.find_first_of('-');
                size_t deviceSplit = device.find_first_of(':');

                std::stringstream ss;

                if(memorySplit != -1) {
                    ss << std::hex << memorySpace.substr(0, memorySplit);
                    ss >> region.start;
                    ss.clear();
                    ss << std::hex << memorySpace.substr(memorySplit + 1, memorySpace.size());
                    ss >> region.end;
                    ss.clear();
                }

                if(deviceSplit != -1) {
                    ss << std::hex << device.substr(0, deviceSplit);
                    ss >> region.deviceMajor;
                    ss.clear();
                    ss << std::hex << device.substr(deviceSplit + 1, device.size());
                    ss >> region.deviceMinor;
                    ss.clear();
                }

                ss << std::hex << offset;
                ss >> region.offset;
                ss.clear();
                ss << inode;
                ss >> region.inodeFileNumber;

                region.readable = (permissions[0] == 'r');
                region.writable = (permissions[1] == 'w');
                region.executable = (permissions[2] == 'x');
                region.shared = (permissions[3] != '-');
                 if(!pathname.empty()) {
                    region.pathname = pathname;
                    size_t fileNameSplit = pathname.find_last_of('/');
                    if(fileNameSplit != -1) {
                        region.filename = pathname.substr(fileNameSplit + 1, pathname.size());
                    }
                 }
                regions.push_back(region);
            }
         }


     }

    bool Handle::IsValid() {
        return (pid != -1);
    }

    // Symbolic link taget
    std::string Handle::GetSymbolicLinkTarget(std::string target) {
        char buf[PATH_MAX];

        ssize_t len = ::readlink(target.c_str(), buf, sizeof(buf) - 1);

        if(len != -1) {
            buf[len] = 0;

            return std::string(buf);
        }

        return std::string();
    }

    // get region in memory
    MapModuleMemoryRegion* Handle::GetRegionOfAddress(void* address) {
        for(size_t i = 0; i < regions.size(); i++) {
            if(regions[i].start > (unsigned long) address && (regions[i].start + regions[i].end) <= (unsigned long) address) {
                return &regions[i];
            }
        }

        return NULL;
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