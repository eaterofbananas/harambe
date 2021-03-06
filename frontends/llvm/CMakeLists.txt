#*******************************************************************************
#
# Author: Maxime Arthaud
#
# Contact: ikos@lists.nasa.gov
#
# Notices:
#
# Copyright (c) 2011-2016 United States Government as represented by the
# Administrator of the National Aeronautics and Space Administration.
# All Rights Reserved.
#
# Disclaimers:
#
# No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF
# ANY KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED
# TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS,
# ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
# OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE
# ERROR FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED, WILL CONFORM TO
# THE SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN
# ENDORSEMENT BY GOVERNMENT AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS,
# RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS
# RESULTING FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY
# DISCLAIMS ALL WARRANTIES AND LIABILITIES REGARDING THIRD-PARTY SOFTWARE,
# IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."
#
# Waiver and Indemnity:  RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST
# THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL
# AS ANY PRIOR RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE RESULTS
# IN ANY LIABILITIES, DEMANDS, DAMAGES, EXPENSES OR LOSSES ARISING FROM SUCH
# USE, INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM,
# RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD
# HARMLESS THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS,
# AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW.
# RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE IMMEDIATE,
# UNILATERAL TERMINATION OF THIS AGREEMENT.
#
#*****************************************************************************/

cmake_minimum_required(VERSION 2.8.12.2 FATAL_ERROR)

project(abstract-representation-llvm-frontend)

#
# Build settings
#

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message (FATAL_ERROR
    "In-source builds are not allowed. Please clean your source tree and try again.")
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE STRING "Install directory." FORCE)
endif()
message(STATUS "All libraries and binaries will be installed in ${CMAKE_INSTALL_PREFIX}")

#
# CMake policy
#

if (POLICY CMP0054)
  cmake_policy(SET CMP0054 OLD) # For add_cxx_flag
endif()

# Do not build anything when running make install
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)

#
# CMake utils
#

include(CheckCXXCompilerFlag)

macro(add_cxx_flag flag name)
  CHECK_CXX_COMPILER_FLAG(${flag} "CXX-SUPPORTS-${name}")
  if("CXX-SUPPORTS-${name}")
    add_definitions(${flag})
  else()
    message(FATAL_ERROR "${flag} flag is not supported by ${CMAKE_CXX_COMPILER}")
  endif()
endmacro()

#
# Dependency checks
#

# Add path for custom modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

set(CUSTOM_BOOST_ROOT "" CACHE PATH "Path to custom boost installation.")
if (CUSTOM_BOOST_ROOT)
  set(BOOST_ROOT "${CUSTOM_BOOST_ROOT}")
  set(Boost_NO_SYSTEM_PATHS "ON")
endif()

find_package(Boost 1.55.0 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(GMP REQUIRED)
include_directories(${GMP_INCLUDE_DIR})
include_directories(${GMPXX_INCLUDE_DIR})

find_package(RT)
if (NOT RT_FOUND)
  set(RT_LIB "")
endif()

find_package(ARBOS REQUIRED)
include_directories(${ARBOS_INCLUDE_DIR})

# If building against a locally built version of LLVM (this must be built with
# CMake and not the Autoconf/Makefile build system) you need to set the
# LLVM_DIR cache variable that find_package(LLVM ...) introduces.
# E.g.
# cmake -DLLVM_DIR:PATH=/path/to/llvm/build/share/llvm/cmake /path/to/this_projects_source_directory
find_package(LLVM REQUIRED)
message(STATUS "Found LLVM ${LLVM_VERSION}")

if (LLVM_VERSION VERSION_LESS "3.7")
  message(FATAL_ERROR "llvm 3.7 or higher required.")
endif()

set(LLVM_RUNTIME_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin")
set(LLVM_LIBRARY_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib")
set(LLVM_INSTALL_TOOLCHAIN_ONLY "ON") # call install() by ourselves
set(LLVM_REQUIRES_RTTI TRUE)
set(LLVM_REQUIRES_EH TRUE)

include_directories(${LLVM_INCLUDE_DIR})
link_directories(${LLVM_LIBRARY_DIRS})

# We incorporate the CMake features provided by LLVM:
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_ROOT}/share/llvm/cmake")

include(LLVMConfig)
include(HandleLLVMOptions)
include(AddLLVM)

#
# Compiler flags
#

add_cxx_flag("-std=c++1y" "CXX14")
add_cxx_flag("-Wall" "WALL")
add_cxx_flag("-D__STDC_CONSTANT_MACROS" "__STDC_CONSTANT_MACROS")
add_cxx_flag("-D__STDC_LIMIT_MACROS" "__STDC_LIMIT_MACROS")
add_definitions(${LLVM_DEFINITIONS})

#
# Targets
#

include_directories(include)

# llvm-to-ar
add_llvm_loadable_module(llvm-to-ar src/passes/llvm_to_ar.cpp)
target_link_libraries(llvm-to-ar ${GMP_LIB} ${GMPXX_LIB} ${RT_LIB})
install(TARGETS llvm-to-ar EXPORT LLVMExports LIBRARY DESTINATION lib OPTIONAL)

# ikos-pp passes (statically build)
add_library(ikos-pp-lib STATIC
  src/utils/local.cpp
  src/passes/lower_cst_expr.cpp
  src/passes/lower_select.cpp
  src/passes/lower_gv_initializers.cpp
  src/passes/name_values.cpp
  src/passes/non_det_init.cpp
  src/passes/mark_internal_inline.cpp
  src/passes/remove_unreachable_blocks.cpp
  src/passes/remove_printf_calls.cpp
  src/passes/mark_no_return_functions.cpp
)

# ikos-pp
add_executable(ikos-pp src/ikos-pp/ikos_pp.cpp)
target_link_libraries(ikos-pp ${GMP_LIB} ${GMPXX_LIB} pthread ikos-pp-lib)

llvm_map_components_to_libnames(IKOS_PP_LLVM_LIBS
  analysis
  asmparser
  bitreader
  bitwriter
  codegen
  core
  instcombine
  instrumentation
  ipo
  irreader
  mc
  objcarcopts
  scalaropts
  support
  target
  transformutils
  vectorize
)
target_link_libraries(ikos-pp ${IKOS_PP_LLVM_LIBS})

if (LLVM_VERSION VERSION_LESS "3.8")
  # IPA is only available on llvm 3.7 and below
  llvm_map_components_to_libnames(IKOS_PP_IPA ipa)
  target_link_libraries(ikos-pp ${IKOS_PP_IPA})
endif()

install(TARGETS ikos-pp RUNTIME DESTINATION bin OPTIONAL)
