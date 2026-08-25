set(APP_BUNDLE "${CMAKE_INSTALL_PREFIX}/NetAnim.app")

find_program(MACDEPLOYQT_EXECUTABLE macdeployqt)

if(NOT MACDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "macdeployqt not found. Please set QT_PATH or ensure it's in PATH.")
endif()

message(STATUS "Running macdeployqt on ${APP_BUNDLE}")

execute_process(
        COMMAND "${MACDEPLOYQT_EXECUTABLE}" "${APP_BUNDLE}" -verbose=2
        RESULT_VARIABLE macdeploy_result
)

if(NOT macdeploy_result EQUAL 0)
    message(FATAL_ERROR "macdeployqt failed with code ${macdeploy_result}")
endif()