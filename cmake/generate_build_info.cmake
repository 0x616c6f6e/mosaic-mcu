if(NOT DEFINED OUTPUT_FILE OR OUTPUT_FILE STREQUAL "")
    message(FATAL_ERROR "OUTPUT_FILE is required")
endif()

get_filename_component(output_directory "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${output_directory}")
string(TIMESTAMP build_timestamp "%Y-%m-%d %H:%M:%S")

file(WRITE "${OUTPUT_FILE}"
    "#ifndef MOSAIC_BUILD_INFO_H\n"
    "#define MOSAIC_BUILD_INFO_H\n"
    "\n"
    "#define MOSAIC_BUILD_TIMESTAMP \"${build_timestamp}\"\n"
    "\n"
    "#endif\n"
)
