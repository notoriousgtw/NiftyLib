cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
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

find_package(glm CONFIG REQUIRED)
find_package(Vulkan)
include_directories(${Vulkan_INCLUDE_DIRS})

file(GLOB SOURCES "src/*.cpp")

add_library(NiftyLib ${SOURCES})

target_compile_definitions(NiftyLib PRIVATE GLFW_STATIC)
target_include_directories(NiftyLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/generated>
    $<INSTALL_INTERFACE:src/include>
)

file(GLOB SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag")
set(GENERATED_HEADERS "")
set(SPIRV_FILES "")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
    set(HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/generated/${SHADER_NAME}.spv.h")

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER} -o ${SPIRV}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${HEADER}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/src/generated"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/spv_to_header.py" "${SPIRV}" "${HEADER}"
        DEPENDS "${SPIRV}"
        COMMENT "Converting ${SPIRV} to header in source tree"
        VERBATIM
    )

    list(APPEND GENERATED_HEADERS ${HEADER})
    list(APPEND SPIRV_FILES ${SPIRV})
endforeach()

add_custom_target(ShaderSpvFiles DEPENDS ${SPIRV_FILES})
add_custom_target(ShaderHeaders DEPENDS ${GENERATED_HEADERS})
add_dependencies(ShaderHeaders ShaderSpvFiles)
add_dependencies(NiftyLib ShaderHeaders)

target_sources(NiftyLib PRIVATE ${GENERATED_HEADERS})

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

# Install generated shader headers
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/generated/"
    DESTINATION include/generated
    FILES_MATCHING PATTERN "*.spv.h"
)

target_link_libraries(NiftyLib PUBLIC lunasvg::lunasvg Vulkan::Vulkan glm::glm glfw)