cmake_minimum_required(VERSION 3.16)
project(platformer-sdl3)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib)
# set_property(TARGET neo PROPERTY WIN32_EXECUTABLE TRUE)

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
# add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)
aux_source_directory(src SRC_LIST)
include_directories(include)

add_executable(platformer-sdl3 ${SRC_LIST})
target_link_libraries(platformer-sdl3 PRIVATE SDL3::SDL3)
# target_link_libraries(platformer-sdl3 PRIVATE SDL3_image::SDL3_image SDL3::SDL3)

add_custom_command(TARGET platformer-sdl3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:platformer-sdl3>
)