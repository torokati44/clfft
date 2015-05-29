#ifndef CLKERNEL_HPP_INCLUDED
#define CLKERNEL_HPP_INCLUDED

#include "clwrapper.hpp"

template<typename... PARAMS>
class CLKernel {

    cl_kernel kernel;

    template<typename HEAD>
    void setParam(int i, HEAD head) {
        clSetKernelArg(kernel, i, sizeof(HEAD), &head);
    }

    template<typename HEAD, typename... TAIL>
    void setParam(int i, HEAD head, TAIL... tail) {
        clSetKernelArg(kernel, i, sizeof(HEAD), &head);
        setParam(i + 1, tail...);
    }

    std::string name;

public:

    CLKernel(const char *name) : name{name} {
        kernel = CLWrapper::instance->createKernel(CLWrapper::instance->program(), name);
    }

    void execute(size_t size, PARAMS... params) {
        cl_event ev;
        setParam(0, params...);
        int result = clEnqueueNDRangeKernel(CLWrapper::instance->cqueue(), kernel,
                                            1, nullptr, &size, nullptr, 0, nullptr, &ev);

        if (result != CL_SUCCESS)
            std::cerr << CLWrapper::getErrorString(result) << std::endl;
        else {
            clWaitForEvents(1, &ev);
            clReleaseEvent(ev);
        }
    }

    ~CLKernel() {
        clReleaseKernel(kernel);
    }
};

#endif
