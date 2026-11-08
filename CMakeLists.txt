cmake_minimum_required(VERSION 3.16)
project(MemoryCity C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 查找 GTK4
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

# 查找 Lua
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
    
    # 加入 lua 文件
    src/lua.c
)

add_executable(MemoryCity ${SOURCES})

target_include_directories(MemoryCity PRIVATE 
    include 
    src 
    src/input
    ${LUA_INCLUDE_DIR} # 包含 Lua 头文件
)

target_link_libraries(MemoryCity PRIVATE 
    PkgConfig::GTK4
    ${LUA_LIBRARIES}   # 链接 Lua 库
)

# 复制 assets（含 events/items/story/chars/scripts）到可执行目录
add_custom_command(TARGET MemoryCity POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:MemoryCity>/assets
)