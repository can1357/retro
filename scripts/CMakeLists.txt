cmake_minimum_required(VERSION 3.17)
project(RetroTablegen)

include(FindPythonInterp)
execute_process(
	COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/tablegen.py" "${CMAKE_CURRENT_LIST_DIR}/../"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)