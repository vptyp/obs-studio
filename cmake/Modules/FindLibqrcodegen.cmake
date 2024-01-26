#[=======================================================================[.rst
FindLibqrcodegen
----------------

FindModule for Libqrcodegen and associated libraries

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the :prop_tgt:`IMPORTED` target ``Libqrcodegen::Libqrcodegen``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libqrcodegen_FOUND``
  True, if all required components and the core library were found.
``Libqrcodegen_VERSION``
  Detected version of found Libqrcodegen libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libqrcodegen_LIBRARY``
  Path to the library component of Libqrcodegen.
``Libqrcodegen_INCLUDE_DIR``
  Directory containing ``qrcodegen.h``.

Distributed under the MIT License, see accompanying LICENSE file or
https://github.com/PatTheMav/cmake-finders/blob/master/LICENSE for details.
(c) 2023 Trystan Mata

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libqrcodegen QUIET qrcodegen)
endif()

# Libqrcodegen_set_soname: Set SONAME on imported library target
macro(Libqrcodegen_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${Libqrcodegen_LIBRARY}' | grep -v '${Libqrcodegen_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET Libqrcodegen::Libqrcodegen PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "${CMAKE_OBJDUMP} -p '${Libqrcodegen_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET Libqrcodegen::Libqrcodegen PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

# Libqrcodegen_find_dll: Find DLL for corresponding import library
macro(Libqrcodegen_find_dll)
  cmake_path(GET Libqrcodegen_IMPLIB PARENT_PATH _implib_path)
  cmake_path(SET _bin_path NORMALIZE "${_implib_path}/../bin")

  string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" _dll_version "${Libqrcodegen_VERSION}")

  find_program(
    Libqrcodegen_LIBRARY
    NAMES qrcodegen.dll
    HINTS ${_implib_path} ${_bin_path}
    DOC "Libqrcodegen DLL location")

  if(NOT Libqrcodegen_LIBRARY)
    set(Libqrcodegen_LIBRARY "${Libqrcodegen_IMPLIB}")
  endif()
  unset(_implib_path)
  unset(_bin_path)
  unset(_dll_version)
endmacro()

find_path(
  Libqrcodegen_INCLUDE_DIR
  NAMES qrcodegen.h
  HINTS ${PC_Libqrcodegen_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES qrcodegen
  DOC "Libqrcodegen include directory")

if(PC_Libqrcodegen_VERSION VERSION_GREATER 0)
  set(Libqrcodegen_VERSION ${PC_Libqrcodegen_VERSION})
else()
  if(NOT Libqrcodegen_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libqrcodegen version.")
  endif()
  set(Libqrcodegen_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  find_library(
    Libqrcodegen_IMPLIB
    NAMES libqrcodegen qrcodegen
    DOC "Libqrcodegen import library location")

  libqrcodegen_find_dll()
else()
  find_library(
    Libqrcodegen_LIBRARY
    NAMES libqrcodegen qrcodegen
    HINTS ${PC_Libqrcodegen_LIBRARY_DIRS}
    PATHS /usr/lib /usr/local/lib
    DOC "Libqrcodegen location")
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Libqrcodegen_ERROR_REASON "Ensure that a qrcodegen distribution is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Libqrcodegen_ERROR_REASON "Ensure that qrcodegen is installed on the system.")
endif()

find_package_handle_standard_args(
  Libqrcodegen
  REQUIRED_VARS Libqrcodegen_LIBRARY Libqrcodegen_INCLUDE_DIR
  VERSION_VAR Libqrcodegen_VERSION REASON_FAILURE_MESSAGE "${Libqrcodegen_ERROR_REASON}")
mark_as_advanced(Libqrcodegen_INCLUDE_DIR Libqrcodegen_LIBRARY Libqrcodegen_IMPLIB)
unset(Libqrcodegen_ERROR_REASON)

if(Libqrcodegen_FOUND)
  if(NOT TARGET Libqrcodegen::Libqrcodegen)
    if(IS_ABSOLUTE "${Libqrcodegen_LIBRARY}")
      if(DEFINED Libqrcodegen_IMPLIB)
        if(Libqrcodegen_IMPLIB STREQUAL Libqrcodegen_LIBRARY)
          add_library(Libqrcodegen::Libqrcodegen STATIC IMPORTED)
        else()
          add_library(Libqrcodegen::Libqrcodegen SHARED IMPORTED)
          set_property(TARGET Libqrcodegen::Libqrcodegen PROPERTY IMPORTED_IMPLIB "${Libqrcodegen_IMPLIB}")
        endif()
      else()
        add_library(Libqrcodegen::Libqrcodegen UNKNOWN IMPORTED)
      endif()
      set_property(TARGET Libqrcodegen::Libqrcodegen PROPERTY IMPORTED_LOCATION "${Libqrcodegen_LIBRARY}")
    else()
      add_library(Libqrcodegen::Libqrcodegen INTERFACE IMPORTED)
      set_property(TARGET Libqrcodegen::Libqrcodegen PROPERTY IMPORTED_LIBNAME "${Libqrcodegen_LIBRARY}")
    endif()

    libqrcodegen_set_soname()
    set_target_properties(
      Libqrcodegen::Libqrcodegen
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_Libqrcodegen_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${Libqrcodegen_INCLUDE_DIR}"
                 VERSION ${Libqrcodegen_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libqrcodegen PROPERTIES
  URL "https://www.nayuki.io/page/qr-code-generator-library"
  DESCRIPTION "This project aims to be the best, clearest library for generating QR Codes in C.")
