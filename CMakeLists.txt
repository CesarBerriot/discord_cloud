cmake_minimum_required(VERSION 3.31)
project(discord_cloud)

set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)

set(DISCORD_CLOUD_DEFAULT_TOKEN "" CACHE STRING "A default bot token that gets built into the executable.")

block()
  include(FetchContent)
  FetchContent_Declare(fetch_github_content GIT_REPOSITORY https://github.com/CesarBerriot/fetch_github_content.git GIT_TAG 1.0.0)
  FetchContent_MakeAvailable(fetch_github_content)
  include(${fetch_github_content_SOURCE_DIR}/fetch_github_content.cmake)

  fetch_github_content(
    discover_sources CesarBerriot 1.1.0 INCLUDE_DEFAULT
    dial CesarBerriot 2.2.2
    lzav avaneev 5.3
    cxxhttp CesarBerriot 1.0.0 INCLUDE_DEFAULT
    json nlohmann v3.12.0
  )

  file(COPY ${lzav_SOURCE_DIR}/lzav.h DESTINATION ${lzav_SOURCE_DIR}/include)
  add_library(lzav INTERFACE)
  target_include_directories(lzav INTERFACE ${lzav_SOURCE_DIR}/include)
endblock()

add_executable(discord_cloud WIN32)
discover_sources(discord_cloud)
target_include_directories(discord_cloud PRIVATE source)
dial_link_target(discord_cloud)
cxxhttp_setup_target(discord_cloud)
target_include_directories(discord_cloud PRIVATE source)
target_compile_definitions(
  discord_cloud PRIVATE
  MODULE_NAME="Discord Cloud"
  DEFAULT_TOKEN="${DISCORD_CLOUD_DEFAULT_TOKEN}"
  NOMINMAX # disable win32 min/max macros
)
target_link_libraries(
  discord_cloud PRIVATE
  lzav
  nlohmann_json
)
if(MINGW)
  target_link_options(discord_cloud PRIVATE -static)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(discord_cloud PRIVATE -s)
  endif()
endif()