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

#include "clwrapper.hpp"

CLWrapper *CLWrapper::instance = 0;

CLWrapper::CLWrapper(cl_device_type device_type) : _device_type(device_type) {
    if (instance) {
        throw "Only one instance plz!";
    }

    createPlatform();
    createDevice();
    createContext();
    createCommandQueue();

    printOpenCLInfo();

    _program = createProgram("src/kernels.cl");

    instance = this;
}

CLWrapper::~CLWrapper() {
    if (instance) {
        clReleaseCommandQueue(_cqueue);
        clReleaseContext(_context);
    }
    instance = 0;
}

char *CLWrapper::getPlatformInfo(cl_platform_info paramName) {
    size_t infoSize = 0;
    CL_SAFE_CALL(clGetPlatformInfo(_platform, paramName, 0, NULL, &infoSize));
    char *info = (char *) malloc(infoSize);
    CL_SAFE_CALL(clGetPlatformInfo(_platform, paramName, infoSize, info, NULL));
    return info;
}

void CLWrapper::createPlatform() {
    CL_SAFE_CALL(clGetPlatformIDs(1, &_platform, NULL));
}

void *CLWrapper::getDeviceInfo(cl_device_info paramName) {
    size_t infoSize = 0;
    CL_SAFE_CALL(clGetDeviceInfo(_device_id, paramName, 0, NULL, &infoSize));
    char *info = (char *) malloc(infoSize);
    CL_SAFE_CALL(clGetDeviceInfo(_device_id, paramName, infoSize, info, NULL));
    return info;
}

void CLWrapper::createDevice() {
    CL_SAFE_CALL(clGetDeviceIDs(_platform, _device_type, 1, &_device_id, NULL));
}

void CLWrapper::createContext() {
    _context = clCreateContext(0, 1, &_device_id, NULL, NULL, NULL);
    if (!_context) {
        std::cerr << "Context creation failed!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void CLWrapper::createCommandQueue() {
    _cqueue = clCreateCommandQueue(_context, _device_id, 0, NULL);
    if (!_cqueue) {
        std::cerr << "Command queue creation failed!" << std::endl;
    }
}

void CLWrapper::printOpenCLInfo() {
    std::cout << getPlatformInfo(CL_PLATFORM_VERSION) << std::endl;

    cl_uint *max_compute_units = (cl_uint *) getDeviceInfo(CL_DEVICE_MAX_COMPUTE_UNITS);
    std::cout << "Max compute units: " << *max_compute_units << std::endl;
}

bool CLWrapper::fileToString(const char *path, char *&out, int &len) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    len = file.tellg();
    out = new char[len + 1];
    file.seekg(0, std::ios::beg);
    file.read(out, len);
    file.close();
    out[len] = 0;
    return true;
}

cl_program CLWrapper::createProgram(const char *fileName) {
    char *programSource = NULL;
    int len = 0;

    if (!fileToString(fileName, programSource, len)) {
        std::cerr << "Error loading program: " << fileName << std::endl;
        exit(EXIT_FAILURE);
    }
    cl_program program = 0;
    cl_int err;
    program = clCreateProgramWithSource(_context, 1, (const char **) &programSource, NULL, &err);
    if (!program) {
        std::cerr << "Error: Failed to create compute program!" << std::endl;
        switch (err) {
            case CL_INVALID_CONTEXT:
                std::cout << "context is not a valid context.\n";
                break;
            case CL_INVALID_VALUE:
                std::cout << "count is zero or if strings or any entry in strings is NULL.\n";
                break;
            case CL_OUT_OF_HOST_MEMORY:
                std::cout <<
                "there is a failure to allocate resources required by the OpenCL implementation on the host.\n";
                break;
        }
        exit(EXIT_FAILURE);
    }

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

    size_t msglen;
    char buffer[2048];

    clGetProgramBuildInfo(program, _device_id, CL_PROGRAM_BUILD_LOG,
                          sizeof(buffer), buffer, &msglen);
    if (buffer[0]) {
        std::cerr << "Program build message: " << std::endl;
        std::cerr << buffer << std::endl;
    }

    return program;
}

cl_kernel CLWrapper::createKernel(cl_program program, const char *kernelName) {
    cl_kernel kernel;
    cl_int err;
    kernel = clCreateKernel(program, kernelName, &err);

    switch (err) {
        case CL_INVALID_PROGRAM:
            std::cout << "program is not a valid program object.\n";
            break;
        case CL_INVALID_PROGRAM_EXECUTABLE:
            std::cout << "there is no successfully built executable for program.\n";
            break;
        case CL_INVALID_KERNEL_NAME:
            std::cout << "kernel_name is not found in program.\n";
            break;
        case CL_INVALID_KERNEL_DEFINITION:
            std::cout <<
            "the function definition for __kernel function given by kernel_name such as the number of arguments, the argument types are not the same for all devices for which the program executable has been built.\n";
            break;
        case CL_INVALID_VALUE:
            std::cout << "kernel_name is NULL.\n";
            break;
        case CL_OUT_OF_HOST_MEMORY:
            std::cout <<
            "there is a failure to allocate resources required by the OpenCL implementation on the host.\n";
            break;
    }


    size_t msglen;
    char buffer[2048];

    clGetProgramBuildInfo(program, _device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &msglen);
    if (buffer[0]) {
        std::cerr << "Kernel build message for " << kernelName << ": " << std::endl;
        std::cerr << buffer << std::endl;
    }
    return kernel;
}

const char *CLWrapper::getErrorString(cl_int error)
{
    switch(error){
        // run-time and JIT compiler errors
        case 0: return "CL_SUCCESS";
        case -1: return "CL_DEVICE_NOT_FOUND";
        case -2: return "CL_DEVICE_NOT_AVAILABLE";
        case -3: return "CL_COMPILER_NOT_AVAILABLE";
        case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case -5: return "CL_OUT_OF_RESOURCES";
        case -6: return "CL_OUT_OF_HOST_MEMORY";
        case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case -8: return "CL_MEM_COPY_OVERLAP";
        case -9: return "CL_IMAGE_FORMAT_MISMATCH";
        case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case -11: return "CL_BUILD_PROGRAM_FAILURE";
        case -12: return "CL_MAP_FAILURE";
        case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
        case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
        case -15: return "CL_COMPILE_PROGRAM_FAILURE";
        case -16: return "CL_LINKER_NOT_AVAILABLE";
        case -17: return "CL_LINK_PROGRAM_FAILURE";
        case -18: return "CL_DEVICE_PARTITION_FAILED";
        case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

            // compile-time errors
        case -30: return "CL_INVALID_VALUE";
        case -31: return "CL_INVALID_DEVICE_TYPE";
        case -32: return "CL_INVALID_PLATFORM";
        case -33: return "CL_INVALID_DEVICE";
        case -34: return "CL_INVALID_CONTEXT";
        case -35: return "CL_INVALID_QUEUE_PROPERTIES";
        case -36: return "CL_INVALID_COMMAND_QUEUE";
        case -37: return "CL_INVALID_HOST_PTR";
        case -38: return "CL_INVALID_MEM_OBJECT";
        case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case -40: return "CL_INVALID_IMAGE_SIZE";
        case -41: return "CL_INVALID_SAMPLER";
        case -42: return "CL_INVALID_BINARY";
        case -43: return "CL_INVALID_BUILD_OPTIONS";
        case -44: return "CL_INVALID_PROGRAM";
        case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
        case -46: return "CL_INVALID_KERNEL_NAME";
        case -47: return "CL_INVALID_KERNEL_DEFINITION";
        case -48: return "CL_INVALID_KERNEL";
        case -49: return "CL_INVALID_ARG_INDEX";
        case -50: return "CL_INVALID_ARG_VALUE";
        case -51: return "CL_INVALID_ARG_SIZE";
        case -52: return "CL_INVALID_KERNEL_ARGS";
        case -53: return "CL_INVALID_WORK_DIMENSION";
        case -54: return "CL_INVALID_WORK_GROUP_SIZE";
        case -55: return "CL_INVALID_WORK_ITEM_SIZE";
        case -56: return "CL_INVALID_GLOBAL_OFFSET";
        case -57: return "CL_INVALID_EVENT_WAIT_LIST";
        case -58: return "CL_INVALID_EVENT";
        case -59: return "CL_INVALID_OPERATION";
        case -60: return "CL_INVALID_GL_OBJECT";
        case -61: return "CL_INVALID_BUFFER_SIZE";
        case -62: return "CL_INVALID_MIP_LEVEL";
        case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
        case -64: return "CL_INVALID_PROPERTY";
        case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
        case -66: return "CL_INVALID_COMPILER_OPTIONS";
        case -67: return "CL_INVALID_LINKER_OPTIONS";
        case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

            // extension errors
        case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
        case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
        case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
        case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
        case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
        case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
        default: return "Unknown OpenCL error";
    }
}
