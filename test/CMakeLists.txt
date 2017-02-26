# CMake build control file for tests
# by Jeff Trull <jetrull@sbcglobal.net>

#Copyright (c) 2012 Jeffrey E Trull
#
#Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

# First, use Collada writing library to create some test data files.
# This gives us finer control over what we test for, and avoids some licensing issues with example files I've found "in the wild".
# Also has a side benefit of giving some coverage of the OpenCollada writer

# check that we have the writer - decoupled because we only use it in tests
find_package_handle_standard_args(OpenCollada
                                  REQUIRED_VARS COLLADASW_INCLUDE_DIR
                                  COLLADASW_LIB COLLADABF_LIB F2A_LIB)
include_directories(..)
include_directories(SYSTEM ${COLLADASW_INCLUDE_DIR})

# define data creation executables and their result files
set(CREATE_LIBS ${COLLADASW_LIB} ${COLLADABF_LIB} ${UTF_LIB} ${F2A_LIB} ${COLLADABU_LIB} ${PCRE_LIB} )

add_executable(create_cube create_cube.cpp)
target_link_libraries(create_cube ${CREATE_LIBS})
add_custom_command(OUTPUT cube.dae DEPENDS create_cube COMMAND create_cube cube.dae)
add_custom_target(cube_data ALL DEPENDS cube.dae)

# define tests themselves
add_executable(cube_test cube_test.cpp)
add_test(cube_test cube_test -- cube.dae)
target_link_libraries(cube_test ${APPLIBS})
#add_executable(tetra_test tetra_test.cpp)

