/*
 *
 * Copyright © 2010-2011 Balázs Tóth <tbalazs@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef CL_WRAPPER_HPP_INCLUDED
#define CL_WRAPPER_HPP_INCLUDED

#include <iostream>
#include <fstream>

#include <CL/opencl.h>
#include <CL/cl_platform.h>

#define REPORT_ERROR(CALL, FILE, LINE) throw std::runtime_error("Unsuccesful OpenCL call (" #CALL ")" #FILE " : " #LINE);
#define CL_SAFE_CALL(CALL) { \
    cl_int err = CALL; \
    if(CL_SUCCESS != err){ \
        REPORT_ERROR(CALL, __FILE__, __LINE__) \
    } \
}

class CLWrapper {
public:
    CLWrapper(cl_device_type _device_type = CL_DEVICE_TYPE_GPU);

    ~CLWrapper();

    static CLWrapper *instance;

    cl_device_id device_id() { return _device_id; }

    cl_context context() { return _context; }

    cl_command_queue cqueue() { return _cqueue; }

    cl_program program() { return _program; }

    char *getPlatformInfo(cl_platform_info paramName);

    void *getDeviceInfo(cl_device_info paramName);

    cl_program createProgram(const char *fileName);

    cl_kernel createKernel(cl_program program, const char *kernelName);

    void printOpenCLInfo();

    static const char *getErrorString(cl_int error);

private:
    cl_device_type _device_type;
    cl_platform_id _platform;
    cl_device_id _device_id;
    cl_context _context;
    cl_command_queue _cqueue;
    cl_program _program;

    void createPlatform();

    void createDevice();

    void createContext();

    void createCommandQueue();

    bool fileToString(const char *path, char *&out, int &len);
};

#endif
