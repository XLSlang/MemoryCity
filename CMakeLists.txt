cmake_minimum_required(VERSION 3.16)
project(MemoryCity C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
find_package(Lua REQUIRED)

set(SOURCES
    src/main.c
    src/dsl.c
    src/json.c
    src/game.c
    src/render.c
    src/ui.c
    src/input/mouse.c

    src/event/event.c
    src/item/item.c
    src/story/story.c
    src/character/character.c
    src/lua.c

    # 新增：地图与存档
    src/map/terrain.c
    src/map/chunk.c
    src/save/save.c
)

add_executable(MemoryCity ${SOURCES})

target_include_directories(MemoryCity PRIVATE
    include
    src
    src/input
    src/map
    src/save
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(MemoryCity PRIVATE
    PkgConfig::GTK4
    ${LUA_LIBRARIES}
)

add_custom_command(TARGET MemoryCity POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:MemoryCity>/assets
)

set_target_properties(MemoryCity PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)