cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(LuaVsPython)

get_filename_component(VI_LUA_DIR "$ENV{LUA_ROOT}/." DIRECTORY)
get_filename_component(VI_PYTHON_DIR "$ENV{PYTHON_ROOT}/." DIRECTORY)

### Files #####################################################################
set(SOURCE_FILES)

set(FILE_GROUP
  "${PROJECT_SOURCE_DIR}/header.h"
  "${PROJECT_SOURCE_DIR}/lua_metrics.cpp"
  "${PROJECT_SOURCE_DIR}/lua_metrics.h"
  "${PROJECT_SOURCE_DIR}/LuaVsPython.cpp"
  "${PROJECT_SOURCE_DIR}/python_metrics.cpp"
  "${PROJECT_SOURCE_DIR}/python_metrics.h"
  "${PROJECT_SOURCE_DIR}/sample.lua"
  "${PROJECT_SOURCE_DIR}/sample.py"
)
source_group("Source Files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "../timing.h"
  "../vi_timing_proxy.h"
)
source_group("Interface files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "${VI_LUA_DIR}/src/lua.hpp"
)
source_group("LUA files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "${VI_PYTHON_DIR}/include/Python.h"
)
source_group("PYTHON files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  C_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  OUTPUT_NAME_RELEASE "${PROJECT_NAME}"
  OUTPUT_NAME_DEBUG   "${PROJECT_NAME}_d"

  LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"

  LIBRARY_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
)

add_dependencies(${PROJECT_NAME}
	vi_timing
)

## Compiler ##################################################################
target_compile_definitions(${PROJECT_NAME}
PRIVATE
  $<$<CONFIG:Debug>: _DEBUG> # Microsoft debug RTL
  $<$<CONFIG:Release>: NDEBUG>
)

target_include_directories(${PROJECT_NAME}
PRIVATE
    "${VI_LUA_DIR}/src"
    "${VI_PYTHON_DIR}/include"
    "../../"
)

if (WIN32)
   target_compile_definitions(${PROJECT_NAME}
   PRIVATE
     WIN32_LEAN_AND_MEAN # Exclude rarely-used stuff from Windows headers.
     NOMINMAX # WinAPI
   )

  target_compile_options(${PROJECT_NAME}
  PRIVATE
    /MP /W3 /nologo /EHsc /Zi
    /experimental:c11atomics # "Atomics are available in Visual Studio 2022 version 17.5 Preview 2 with the /experimental:c11atomics flag, in /std:c11 mode or later."
    $<$<CONFIG:Release>: /MT  /O2 /Oi /GL /Gy>
    $<$<CONFIG:Debug>:   /MTd /Od /RTC1>
  )
elseif (UNIX)
  target_compile_options(${PROJECT_NAME}
  PRIVATE
    -fvisibility=hidden
    -Wno-psabi #suppress "note: parameter passing for argument of type <...> changed in GCC 7.1" message.
    $<$<COMPILE_LANGUAGE:CXX>: -fPIC>
    $<$<CONFIG:Release>: -O3 -ggdb3 -s>
  )
endif()

### Linker ####################################################################

target_link_libraries(${PROJECT_NAME}
PRIVATE
	vi_timing
)

if (WIN32)
  target_link_options(${PROJECT_NAME}
  PRIVATE
    /SUBSYSTEM:CONSOLE /DEBUG
    $<$<CONFIG:Release>: /INCREMENTAL:NO /LTCG /OPT:REF /OPT:ICF>
    $<$<CONFIG:Debug>:   /INCREMENTAL>
  )

  target_link_directories(${PROJECT_NAME}
  PRIVATE
    "${VI_LUA_DIR}/x64CPP"
    "${VI_PYTHON_DIR}/libs"
  )

  target_link_libraries(${PROJECT_NAME}
  PRIVATE
    lua54
#    python3
  )
elseif (UNIX)
  target_link_options(${PROJECT_NAME}
  PRIVATE
    -Wl,--exclude-libs,ALL
  )

  target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rt
    pthread
  PUBLIC
    atomic
  )
endif()
