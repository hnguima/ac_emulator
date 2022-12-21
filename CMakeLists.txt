# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(temp_sim)

add_custom_target(
    compress_server
    COMMAND ${PYTHON} "./script/compress_server.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

#Salva os arquivos na flash interna do ESP32
# spiffs_create_partition_image(storage1 "flash_partitions/data" FLASH_IN_PROJECT DEPENDS autover)
spiffs_create_partition_image(storage1 "./data" FLASH_IN_PROJECT)
spiffs_create_partition_image(storage0 "./build/http_server" FLASH_IN_PROJECT DEPENDS compress_server)
# spiffs_create_partition_image(storage0 "flash_partitions/server" FLASH_IN_PROJECT)

add_custom_command(TARGET app POST_BUILD
    COMMAND ${PYTHON} "./script/bundle_binaries.py"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)