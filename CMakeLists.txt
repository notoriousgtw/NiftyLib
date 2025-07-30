cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DBUILD_SHARED_LIBS=OFF -DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON)
set(DCMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")

 
# Static runtime for MSVC
if(MSVC)
    foreach(flag_var
        CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_DEBUG)
        string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endforeach()
endif()

# Specify the project name
project(NiftyLib)

include(FetchContent)
FetchContent_Declare(
    lunasvg
    GIT_REPOSITORY https://github.com/sammycage/lunasvg.git
    GIT_TAG master  # Specify the desired branch or tag
)
FetchContent_MakeAvailable(lunasvg)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG latest
)
FetchContent_MakeAvailable(glfw)

find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

file(GLOB SOURCES "src/*.cpp")

add_library(NiftyLib ${SOURCES})

target_compile_definitions(NiftyLib PRIVATE GLFW_STATIC)
target_include_directories(NiftyLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
    $<INSTALL_INTERFACE:src/include>
)

install(TARGETS NiftyLib EXPORT NiftyLibTargets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include
    )

install(EXPORT NiftyLibTargets
        FILE NiftyLibConfig.cmake
        NAMESPACE NiftyLib::
        DESTINATION lib/cmake/NiftyLib
    )

if (WIN32)
    target_link_libraries(NiftyLib PRIVATE opengl32 gdi32 user32 shell32)
endif()

target_link_libraries(NiftyLib PRIVATE lunasvg::lunasvg imgui::imgui glfw glad::glad)