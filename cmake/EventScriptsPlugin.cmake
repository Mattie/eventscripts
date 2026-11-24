if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR NOT MSVC_VERSION EQUAL 1400)
  message(FATAL_ERROR "EVENTSCRIPTS_BUILD_PLUGIN requires Windows x86 VC++ 2005 SP1 (MSVC_VERSION 1400), not a modern MSVC or Linux toolchain. Standalone checks remain available with the option OFF.")
endif()
# CMake's VC8 identification omits the servicing revision; query each actual compiler.
foreach(language IN ITEMS C CXX)
  execute_process(COMMAND "${CMAKE_${language}_COMPILER}"
    RESULT_VARIABLE ES_VC_RESULT OUTPUT_VARIABLE ES_VC_OUTPUT ERROR_VARIABLE ES_VC_ERROR)
  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+" ES_VC_FULL_VERSION "${ES_VC_OUTPUT}\n${ES_VC_ERROR}")
  if(NOT ES_VC_RESULT EQUAL 0 OR NOT ES_VC_FULL_VERSION MATCHES "^14\\.0+\\." OR
     ES_VC_FULL_VERSION VERSION_LESS "14.0.50727.762")
    message(FATAL_ERROR "The plugin requires VC++ 2005 SP1 or its servicing updates (14.0.50727.762 or newer within VC8). Cannot verify that full version from the ${language} compiler banner: ${CMAKE_${language}_COMPILER} (reported '${ES_VC_FULL_VERSION}').")
  endif()
  message(STATUS "Verified ${language} compiler VC8 servicing version: ${ES_VC_FULL_VERSION}")
endforeach()
if(CMAKE_CONFIGURATION_TYPES OR NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(FATAL_ERROR "Use a single-configuration generator such as NMake Makefiles with -DCMAKE_BUILD_TYPE=Release for the historical plugin.")
endif()
enable_language(RC)

set(EVENTSCRIPTS_SDK_ROOT "" CACHE PATH "Explicit external Orange Box Source SDK root")
set(EVENTSCRIPTS_PYTHON_ROOT "" CACHE PATH "Explicit external Python 2.5 headers and Windows x86 import library root")
foreach(dependency IN ITEMS EVENTSCRIPTS_SDK_ROOT EVENTSCRIPTS_PYTHON_ROOT)
  if(NOT IS_ABSOLUTE "${${dependency}}" OR NOT IS_DIRECTORY "${${dependency}}")
    message(FATAL_ERROR "Set ${dependency} to an absolute external dependency directory; no bundled SDK/Python or recovery fallback is used.")
  endif()
endforeach()
set(ES_SDK_DIR "${EVENTSCRIPTS_SDK_ROOT}")
set(ES_PYTHON_DIR "${EVENTSCRIPTS_PYTHON_ROOT}")
set(ES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(ES_SDK_SOURCES
  "${ES_SDK_DIR}/tier1/characterset.cpp"
  "${ES_SDK_DIR}/tier1/convar.cpp"
  "${ES_SDK_DIR}/public/filesystem_helpers.cpp"
  "${ES_SDK_DIR}/tier1/interface.cpp"
  "${ES_SDK_DIR}/tier1/KeyValues.cpp"
  "${ES_SDK_DIR}/public/tier0/memoverride.cpp")
set(ES_SDK_LIBRARIES
  "${ES_SDK_DIR}/lib/public/tier0.lib"
  "${ES_SDK_DIR}/lib/public/tier1.lib"
  "${ES_SDK_DIR}/lib/public/tier2.lib"
  "${ES_SDK_DIR}/lib/public/tier3.lib"
  "${ES_SDK_DIR}/lib/public/mathlib.lib"
  "${ES_SDK_DIR}/lib/public/vstdlib.lib")
foreach(required IN LISTS ES_SDK_SOURCES ES_SDK_LIBRARIES)
  if(NOT EXISTS "${required}")
    message(FATAL_ERROR "Missing explicitly required Orange Box SDK input: ${required}")
  endif()
endforeach()
foreach(required IN ITEMS Include/Python.h Include/patchlevel.h PC/pyconfig.h libs/python25.lib)
  if(NOT EXISTS "${ES_PYTHON_DIR}/${required}")
    message(FATAL_ERROR "Missing explicitly required Python 2.5 input: ${ES_PYTHON_DIR}/${required}")
  endif()
endforeach()
file(READ "${ES_PYTHON_DIR}/Include/patchlevel.h" ES_PYTHON_VERSION)
if(NOT ES_PYTHON_VERSION MATCHES "#[ \t]*define[ \t]+PY_MAJOR_VERSION[ \t]+2([ \r\n]|$)" OR
   NOT ES_PYTHON_VERSION MATCHES "#[ \t]*define[ \t]+PY_MINOR_VERSION[ \t]+5([ \r\n]|$)")
  message(FATAL_ERROR "EVENTSCRIPTS_PYTHON_ROOT must provide Python 2.5 headers, not host Python 3.")
endif()

# Python.h uses a quoted include; stage the Windows configuration beside its headers.
set(ES_PYTHON_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/python-include")
file(COPY "${ES_PYTHON_DIR}/Include/" DESTINATION "${ES_PYTHON_INCLUDE}" PATTERN "pyconfig.h" EXCLUDE)
configure_file("${ES_PYTHON_DIR}/PC/pyconfig.h" "${ES_PYTHON_INCLUDE}/pyconfig.h" COPYONLY)

get_filename_component(ES_VC_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
set(ES_INTRIN_HEADER "${ES_VC_BIN}/../include/intrin.h")
if(NOT EXISTS "${ES_INTRIN_HEADER}")
  message(FATAL_ERROR "Cannot locate the selected VC8 compiler's intrin.h: ${ES_INTRIN_HEADER}")
endif()
file(READ "${ES_INTRIN_HEADER}" ES_INTRINSICS)
foreach(intrinsic IN ITEMS _interlockedbittestandset _interlockedbittestandreset)
  set(old "${intrinsic}(long *a")
  set(fixed "${intrinsic}(long volatile *a")
  string(FIND "${ES_INTRINSICS}" "${old}" old_position)
  string(FIND "${ES_INTRINSICS}" "${fixed}" fixed_position)
  if(NOT old_position EQUAL -1 AND fixed_position EQUAL -1)
    string(REPLACE "${old}" "${fixed}" ES_INTRINSICS "${ES_INTRINSICS}")
  elseif(fixed_position EQUAL -1 OR NOT old_position EQUAL -1)
    message(FATAL_ERROR "Unknown VC8 ${intrinsic} declaration; refusing an unverified intrinsic-header rewrite.")
  endif()
endforeach()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/compat-include")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/compat-include/intrin.h" "${ES_INTRINSICS}")

include("${PROJECT_SOURCE_DIR}/cmake/StageSdkUtlbuffer.cmake")
eventscripts_stage_sdk_utlbuffer("${ES_SDK_DIR}/tier1/utlbuffer.cpp" "${CMAKE_CURRENT_BINARY_DIR}/sdk-compat/utlbuffer.cpp")

add_library(es_sqlite STATIC
  sqlite/alter.c
  sqlite/analyze.c
  sqlite/attach.c
  sqlite/auth.c
  sqlite/btree.c
  sqlite/build.c
  sqlite/callback.c
  sqlite/complete.c
  sqlite/date.c
  sqlite/delete.c
  sqlite/expr.c
  sqlite/fts1.c
  sqlite/fts1_hash.c
  sqlite/fts1_porter.c
  sqlite/fts1_tokenizer1.c
  sqlite/func.c
  sqlite/hash.c
  sqlite/insert.c
  sqlite/legacy.c
  sqlite/loadext.c
  sqlite/main.c
  sqlite/opcodes.c
  sqlite/os.c
  sqlite/os_os2.c
  sqlite/os_unix.c
  sqlite/os_win.c
  sqlite/pager.c
  sqlite/parse.c
  sqlite/pragma.c
  sqlite/prepare.c
  sqlite/printf.c
  sqlite/random.c
  sqlite/select.c
  sqlite/shell.c
  sqlite/table.c
  sqlite/tclsqlite.c
  sqlite/tokenize.c
  sqlite/trigger.c
  sqlite/update.c
  sqlite/utf.c
  sqlite/util.c
  sqlite/vacuum.c
  sqlite/vdbe.c
  sqlite/vdbeapi.c
  sqlite/vdbeaux.c
  sqlite/vdbefifo.c
  sqlite/vdbemem.c
  sqlite/vtab.c
  sqlite/where.c)
target_compile_definitions(es_sqlite PRIVATE NO_TCL _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)

add_library(es_vsplib STATIC vsplib/VSPLib.cpp vsplib/VspPlayer.cpp)
target_include_directories(es_vsplib PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/compat-include"
  "${ES_SDK_DIR}/public" "${ES_SDK_DIR}/public/tier0" "${ES_SDK_DIR}/public/tier1")
target_compile_definitions(es_vsplib PRIVATE WIN32 NDEBUG _LIB COMPILER_MSVC COMPILER_MSVC32)

add_library(mattie_eventscripts SHARED
  es_client_messages.cpp
  es_cmddef.cpp
  es_convar.cpp
  es_debug.cpp
  es_hashtable.cpp
  es_keygroups.cpp
  es_keymsg.cpp
  es_mathparse.cpp
  es_playerutil.cpp
  es_plugin_helpers.cpp
  es_profiler.cpp
  es_shellcmds.cpp
  es_sqlite.cpp
  es_stringtable.cpp
  es_strutils.cpp
  es_usermsg.cpp
  es_vecstring.cpp
  es_version.cpp
  mattie_eventscripts.cpp
  MRecipientFilter.cpp
  version.rc
  VirtualMachine/Block.cpp
  VirtualMachine/BlockArgs.cpp
  VirtualMachine/BlockStateManager.cpp
  VirtualMachine/ConditionalStateManager.cpp
  VirtualMachine/ExecutionManager.cpp
  VirtualMachine/Script.cpp
  VirtualMachine/ScriptManager.cpp
  VirtualMachine/VirtualMachine.cpp
  ${ES_SDK_SOURCES}
  "${CMAKE_CURRENT_BINARY_DIR}/sdk-compat/utlbuffer.cpp")
target_include_directories(mattie_eventscripts PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/compat-include"
  "${ES_SOURCE_DIR}" "${ES_SOURCE_DIR}/sqlite" "${ES_PYTHON_INCLUDE}" "${ES_PYTHON_DIR}/PC"
  "${ES_SDK_DIR}/public/tier0" "${ES_SDK_DIR}/public/tier1" "${ES_SDK_DIR}/public"
  "${ES_SDK_DIR}/game" "${ES_SDK_DIR}/game_shared" "${ES_SDK_DIR}/public/vstdlib"
  "${ES_SDK_DIR}/game/server" "${ES_SDK_DIR}/dlls" "${ES_SDK_DIR}/game/shared")
target_compile_definitions(mattie_eventscripts PRIVATE WIN32 NDEBUG _WINDOWS mattie_eventscriptsONLY PYTHON_SUPPORT ORANGEBOX
  BUILDNUM=379 REVISION=local_root_rebuild VER_PRIVATEBUILD_STR="Local VC2005 rebuild"
  _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
target_compile_options(mattie_eventscripts PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/Zc:forScope-> /Zc:wchar_t- /W3)
target_link_libraries(mattie_eventscripts PRIVATE
  eventscripts_regex eventscripts_parser es_sqlite es_vsplib
  "${ES_PYTHON_DIR}/libs/python25.lib" ${ES_SDK_LIBRARIES} delayimp ws2_32)
target_link_options(mattie_eventscripts PRIVATE /DELAYLOAD:python25.dll /MAP)
set_target_properties(mattie_eventscripts es_vsplib PROPERTIES
  CXX_STANDARD 98 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
