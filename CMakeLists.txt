# Alternative to the Visual Studio solution: works with Visual Studio's
# "Open Folder" (choose an x86 configuration) and with MinGW-w64 i686.
cmake_minimum_required(VERSION 3.15)
project(Coman1QoL CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "Configure for 32-bit x86 (e.g. -A Win32): the game is 32-bit.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
  add_compile_options(/GR- /EHs-c-)
  add_link_options(/SAFESEH:NO)
else()
  add_compile_options(-fno-exceptions -fno-rtti -Wall -Wno-unused-parameter)
  add_link_options(-static -s)
endif()

file(GLOB COMMON_SOURCES CONFIGURE_DEPENDS Common/*.cpp)
add_library(Common STATIC ${COMMON_SOURCES})
target_include_directories(Common PUBLIC Common)

# winmm.dll: forwards winmm to the system DLL and loads the *.c1m modules.
file(GLOB PROXY_SOURCES CONFIGURE_DEPENDS WinmmProxy/*.cpp)
add_library(WinmmProxy SHARED ${PROXY_SOURCES} WinmmProxy/winmm.def)
set_target_properties(WinmmProxy PROPERTIES PREFIX "" OUTPUT_NAME winmm)
target_link_libraries(WinmmProxy PRIVATE Common)
if(NOT MSVC)
  target_link_options(WinmmProxy PRIVATE -Wl,--enable-stdcall-fixup)
endif()

# Resolution.c1m
file(GLOB RESOLUTION_SOURCES CONFIGURE_DEPENDS ComanResolution/*.cpp)
add_library(ComanResolution SHARED ${RESOLUTION_SOURCES})
set_target_properties(ComanResolution PROPERTIES PREFIX "" OUTPUT_NAME Resolution SUFFIX ".c1m")
target_link_libraries(ComanResolution PRIVATE Common user32)
