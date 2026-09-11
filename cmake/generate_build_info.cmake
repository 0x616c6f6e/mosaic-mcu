foreach(required_variable OUTPUT_FILE TEMPLATE_FILE PROJECT_DIR
                          FIRMWARE_VERSION FIRMWARE_BUILD_TYPE)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

get_filename_component(output_directory "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${output_directory}")
string(TIMESTAMP FIRMWARE_BUILD_TIME "%Y-%m-%d %H:%M:%S")

set(FIRMWARE_GIT_DESCRIBE "unknown")
set(FIRMWARE_GIT_HASH "unknown")
set(FIRMWARE_GIT_BRANCH "unknown")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" describe --abbrev=7 --always --tags --dirty
        WORKING_DIRECTORY "${PROJECT_DIR}"
        RESULT_VARIABLE git_describe_result
        OUTPUT_VARIABLE FIRMWARE_GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=7 HEAD
        WORKING_DIRECTORY "${PROJECT_DIR}"
        RESULT_VARIABLE git_hash_result
        OUTPUT_VARIABLE FIRMWARE_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${PROJECT_DIR}"
        RESULT_VARIABLE git_branch_result
        OUTPUT_VARIABLE FIRMWARE_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT git_describe_result EQUAL 0)
        set(FIRMWARE_GIT_DESCRIBE "unknown")
    endif()
    if(NOT git_hash_result EQUAL 0)
        set(FIRMWARE_GIT_HASH "unknown")
    endif()
    if(NOT git_branch_result EQUAL 0)
        set(FIRMWARE_GIT_BRANCH "unknown")
    endif()
endif()

foreach(value FIRMWARE_VERSION FIRMWARE_BUILD_TYPE FIRMWARE_BUILD_TIME
              FIRMWARE_GIT_DESCRIBE FIRMWARE_GIT_HASH FIRMWARE_GIT_BRANCH)
    string(REPLACE "\\" "\\\\" ${value}_C "${${value}}")
    string(REPLACE "\"" "\\\"" ${value}_C "${${value}_C}")
endforeach()

configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)
message(STATUS
    "Firmware: version=${FIRMWARE_VERSION}, type=${FIRMWARE_BUILD_TYPE}, "
    "built=${FIRMWARE_BUILD_TIME}, git=${FIRMWARE_GIT_DESCRIBE}, "
    "branch=${FIRMWARE_GIT_BRANCH}")
