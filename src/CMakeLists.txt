set(TRE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tre")
add_library(eventscripts_tre STATIC
  "${TRE_DIR}/lib/tre-ast.c"
  "${TRE_DIR}/lib/tre-compile.c"
  "${TRE_DIR}/lib/tre-match-backtrack.c"
  "${TRE_DIR}/lib/tre-match-parallel.c"
  "${TRE_DIR}/lib/tre-match-approx.c"
  "${TRE_DIR}/lib/tre-mem.c"
  "${TRE_DIR}/lib/tre-parse.c"
  "${TRE_DIR}/lib/tre-stack.c"
  "${TRE_DIR}/lib/regcomp.c"
  "${TRE_DIR}/lib/regexec.c"
  "${TRE_DIR}/lib/regerror.c")
target_include_directories(eventscripts_tre SYSTEM PUBLIC "${TRE_DIR}/config" "${TRE_DIR}/lib")
target_compile_definitions(eventscripts_tre PRIVATE HAVE_CONFIG_H)

add_library(eventscripts_regex STATIC es_regex_adapter.cpp)
target_include_directories(eventscripts_regex PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(eventscripts_regex PUBLIC eventscripts_tre)
set_target_properties(eventscripts_regex PROPERTIES CXX_STANDARD 98 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

add_library(eventscripts_parser STATIC
  muParser/muParser.cpp
  muParser/muParserBase.cpp
  muParser/muParserBytecode.cpp
  muParser/muParserCallback.cpp
  muParser/muParserError.cpp
  muParser/muParserInt.cpp
  muParser/muParserTokenReader.cpp)
target_include_directories(eventscripts_parser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/muParser")
set_target_properties(eventscripts_parser PROPERTIES CXX_STANDARD 98 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

if(EVENTSCRIPTS_BUILD_PLUGIN)
  include("${PROJECT_SOURCE_DIR}/cmake/EventScriptsPlugin.cmake")
endif()
