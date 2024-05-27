cmake_minimum_required(VERSION 3.22)
project(GreenCitrus LANGUAGES C VERSION 0.2.3)
set(CMAKE_C_STANDARD 23)


add_executable(${PROJECT_NAME}
        source/main.c
)
#target_include_directories(${PROJECT_NAME} PRIVATE include/)

target_link_libraries(${PROJECT_NAME} -lcitro2d -lcitro3d)

target_compile_options(${PROJECT_NAME} PRIVATE
        -Oz -Wall -mword-relocations -ffunction-sections -Wl,--gc-sections -fdata-sections -flto -fomit-frame-pointer
)

ctr_generate_smdh(${PROJECT_NAME}.smdh
        NAME "${PROJECT_NAME}"
        DESCRIPTION "v${PROJECT_VERSION} Citrus de-yellow'izer"
        AUTHOR "Pugemon"
        ICON "${CMAKE_CURRENT_LIST_DIR}/icon_cg.png"
)

ctr_create_3dsx(${PROJECT_NAME}
        SMDH ${PROJECT_NAME}.smdh
)
