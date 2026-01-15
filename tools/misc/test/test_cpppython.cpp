// Copyright 2021 Randal A. Koene
// License TBD

/*
This is a test of Boost.Python wrappers for C++ to build shared libraries
from C++ code with interfaces usable from Python.
*/

// Boost libraries need the following.
#pragma GCC diagnostic warning "-Wuninitialized"
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#pragma GCC diagnostic warning "-Wpedantic"

// *** If you need to get this to properly find Boost components then
// *** compare with other Formalizer programs using Boost and their Makefiles.
// *** Some steps:
// *** - To get pyconfig.h: sudo apt install python3-dev
// *** - To make pyconfig.h discoverable: add -I/usr/include/python3.8 to Makefile INCLUDES

#include <boost/python/numpy.hpp>
#include <iostream>

namespace p = boost::python;
namespace np = boost::python::numpy;

int main(int argc, char **argv) {
    Py_Initialize();
    np::initialize();

    p::tuple shape = p::make_tuple(3, 3);
    np::dtype dtype = np::dtype::get_builtin<float>();
    np::ndarray a = np::zeros(shape, dtype);

    np::ndarray b = np::empty(shape, dtype);

    std::cout << "Original array:\n"
              << p::extract<char const *>(p::str(a)) << std::endl;

    // Reshape the array into a 1D array
    a = a.reshape(p::make_tuple(9));
    // Print it again.
    std::cout << "Reshaped array:\n"
              << p::extract<char const *>(p::str(a)) << std::endl;
}
