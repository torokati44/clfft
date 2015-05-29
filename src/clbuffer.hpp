#ifndef CLBUFFER_HPP_INCLUDED
#define CLBUFFER_HPP_INCLUDED

#include <stdexcept>
#include <CL/cl.h>
#include "clwrapper.hpp"

#include <cstdio>
#include <cstring>

template<typename T>
class CLBuffer {

    cl_mem mem;
    T *mapped = nullptr;
    size_t length;
    int map_count = 0;

public:

    CLBuffer(size_t length) : length{length} {
        mem = clCreateBuffer(CLWrapper::instance->context(), CL_MEM_READ_WRITE,
                             length * sizeof(T), nullptr, nullptr);

        map();

        std::memset(mapped, 0, length * sizeof(T));

        unmap();
    }

    T *map() {
        if (map_count == 0) {
            cl_event ev;
            mapped = (T *) clEnqueueMapBuffer(CLWrapper::instance->cqueue(),
                                              mem, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0,
                                              length * sizeof(T),
                                              0, nullptr, &ev, nullptr);
            clWaitForEvents(1, &ev);
            clReleaseEvent(ev);
        }

        ++map_count;

        return mapped;
    }

    void unmap() {
        if (map_count == 0) {
            throw std::logic_error("unmap() called on a CLBuffer object which is not mapped");
        }

        --map_count;

        if (map_count == 0) {
            cl_event ev;
            clEnqueueUnmapMemObject(CLWrapper::instance->cqueue(), mem, mapped, 0, nullptr, &ev);
            clWaitForEvents(1, &ev);
            clReleaseEvent(ev);
        }
        mapped = 0;
    }

    operator cl_mem() {
        return mem;
    }

    ~CLBuffer() {
        if (map_count != 0) {
            unmap();
        }

        clReleaseMemObject(mem);
    }
};

#endif
