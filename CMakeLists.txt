# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(cp32wr)

# execute_process(
#     COMMAND bash -c "source ./script/compress_server.sh"
#     WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
# )

# #Salva os arquivos na flash interna do ESP32
# spiffs_create_partition_image(storage0 "flash_partitions/server" FLASH_IN_PROJECT)
# # spiffs_create_partition_image(storage1 "flash_partitions/data" FLASH_IN_PROJECT)
