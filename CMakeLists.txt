# Prefer static libraries and static runtime by default
set(BUILD_SHARED_LIBS OFF)

# Specify the project name
project(NiftyLib)

# include(FetchContent)
# FetchContent_Declare(
#     lunasvg
#     GIT_REPOSITORY https://github.com/sammycage/lunasvg.git
#     GIT_TAG master  # Specify the desired branch or tag
# )
# FetchContent_MakeAvailable(lunasvg)

# FetchContent_Declare(
#     glfw
#     GIT_REPOSITORY https://github.com/glfw/glfw.git
#     GIT_TAG latest
# )
# FetchContent_MakeAvailable(glfw)

find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(lunasvg CONFIG REQUIRED)

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

if(UNIX AND NOT APPLE)
    target_link_options(NiftyLib PRIVATE -static -static-libgcc -static-libstdc++)
endif()

target_link_libraries(NiftyLib PRIVATE lunasvg::lunasvg imgui::imgui glm::glm glfw glad::glad)