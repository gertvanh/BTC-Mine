cmake_minimum_required(VERSION 3.16)
project(BtcMineSimulator CXX)

set(CMAKE_CXX_STANDARD 17)

# Reuse SDL2 + toolchain from Pool-display-simulator when available
set(POOL_SIM_ROOT "${CMAKE_SOURCE_DIR}/../../Pool-display-simulator")
set(SDL2_DIR "${POOL_SIM_ROOT}/SDL2-2.30.12/x86_64-w64-mingw32")
set(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/..")

set(SDL2_INCLUDE "${SDL2_DIR}/include")
set(SDL2_LIB_DIR "${SDL2_DIR}/lib")

add_executable(btc_mine_sim
    src/main.cpp
    mock/arduino_impl.cpp
    mock/TFT_eSPI.cpp
    ${PROJECT_ROOT}/src/app.cpp
)

target_include_directories(btc_mine_sim PRIVATE
    ${CMAKE_SOURCE_DIR}/mock
    ${PROJECT_ROOT}/include
    ${SDL2_INCLUDE}
)

target_compile_definitions(btc_mine_sim PRIVATE
    SIM_BUILD=1
    SDL_MAIN_HANDLED
    TFT_WIDTH=240
    TFT_HEIGHT=320
)

target_link_libraries(btc_mine_sim PRIVATE
    "${SDL2_LIB_DIR}/libSDL2.a"
    -lmingw32
    -lsetupapi
    -limm32
    -lwinmm
    -lversion
    -lole32
    -loleaut32
    -lgdi32
    -lcomdlg32
)

add_custom_command(TARGET btc_mine_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${SDL2_DIR}/bin/SDL2.dll"
        "$<TARGET_FILE_DIR:btc_mine_sim>/SDL2.dll"
    COMMENT "SDL2.dll kopieren naar build map"
)
