
cmake_minimum_required(VERSION 3.10)
project(OpenGLStarter)

add_subdirectory(external/glfw-3.5.1)
add_subdirectory(external/glm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

add_executable(OpenGLStarter
    main.cpp
)

target_include_directories(OpenGLStarter PRIVATE external/glad/include)
target_link_libraries(OpenGLStarter PRIVATE glfw OpenGL::GL glm::glm)

add_executable(PilotAnimation
    pilot_demo.cpp
)

target_include_directories(PilotAnimation PRIVATE
    external/glad/include
)

target_link_libraries(PilotAnimation PRIVATE
    glfw
    OpenGL::GL
)

add_custom_command(TARGET PilotAnimation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/pilot_atlas.png
        $<TARGET_FILE_DIR:PilotAnimation>
)

add_executable(OfficerAnimation
    officer_demo.cpp
)

target_include_directories(OfficerAnimation PRIVATE
    external/glad/include
)

target_link_libraries(OfficerAnimation PRIVATE
    glfw
    OpenGL::GL
)

add_custom_command(TARGET OfficerAnimation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/officer_atlas.png
        $<TARGET_FILE_DIR:OfficerAnimation>
)