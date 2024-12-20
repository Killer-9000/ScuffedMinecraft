cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)

project(ScuffedMinecraft)

# Specify the C++ standard
set ( CMAKE_CXX_STANDARD 20 )
set ( CMAKE_CXX_STANDARD_REQUIRED True )

# output directories
set ( USE_FOLDERS ON)
set ( PREDEFINED_TARGETS_FOLDER "CMakePredefinedTargets")
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# copy assets
add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/assets
  ${CMAKE_BINARY_DIR}/assets
  COMMENT "Copying assets")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# global dependency directories
add_subdirectory(vendor)

file(GLOB_RECURSE sources ${CMAKE_SOURCE_DIR}/src/*)

add_executable(scuffed_mc ${sources})

target_include_directories ( scuffed_mc
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/
        ${CMAKE_SOURCE_DIR}/src/imgui
)
target_link_libraries( scuffed_mc
    PUBLIC
        fmt::fmt
        glfw
		glm::glm
        Tracy::TracyClient
)

if(LINUX)
  add_compile_definitions(LINUX)
elseif(WIN32)
  add_compile_definitions(WINDOWS)
endif()

# make sure the program runs in the right place from visual studio
#set_target_properties(scuffed_mc PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
#target_link_libraries(scuffed_mc imgui $<IF:$<PLATFORM_ID:Windows>,glfw3,glfw>)
