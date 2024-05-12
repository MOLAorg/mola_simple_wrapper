# Minimum CMake vesion: limited by CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS
cmake_minimum_required(VERSION 3.4)

if("$ENV{ROS_VERSION}" STREQUAL "2")
	set(DETECTED_ROS2 TRUE)
endif()

# Tell CMake we'll use C++ for use in its tests/flags
project(mola_simple_wrapper LANGUAGES CXX)

# MOLA CMake scripts: "mola_xxx()"
find_package(mola_common REQUIRED)

# find dependencies:
find_package(mrpt-maps REQUIRED)
find_package(mrpt-tclap REQUIRED)
find_mola_package(mola_kernel)

# CLI apps
# ---------------------------------
find_package(mola_input_kitti_dataset) # optional
find_package(mola_input_kitti360_dataset) # optional
find_package(mola_input_rawlog) # optional
find_package(mola_input_mulran_dataset) # optional
find_package(mola_input_rosbag2) # optional
find_package(mola_input_paris_luco_dataset) # optional

# --------------------------
# Dependency: DLib
# --------------------------
set(EMBEDDED_dlib_SOURCE_DIR "${${PROJECT_NAME}_SOURCE_DIR}/3rdparty/dlib-19.24")
set(EMBEDDED_dlib_BUILD_DIR "${${PROJECT_NAME}_BINARY_DIR}/3rdparty/dlib/")
set(EMBEDDED_dlib_INSTALL_DIR "${${PROJECT_NAME}_BINARY_DIR}/3rdparty/dlib/install/")
set(EMBEDDED_dlib_DIR "${EMBEDDED_dlib_INSTALL_DIR}/lib/cmake/dlib/")

if (TRUE)
	message(STATUS "--- Running CMake on external submodule 'dlib'...")
	file(MAKE_DIRECTORY "${EMBEDDED_dlib_BUILD_DIR}")
	if(NOT ${CMAKE_VERSION} VERSION_LESS "3.15")
		set(echo_flag COMMAND_ECHO STDOUT)
	endif()
	execute_process(COMMAND
		${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" "${EMBEDDED_dlib_SOURCE_DIR}"
		-DDLIB_NO_GUI_SUPPORT=ON
		-DCMAKE_POSITION_INDEPENDENT_CODE=ON
		-DCMAKE_INSTALL_PREFIX=${EMBEDDED_dlib_INSTALL_DIR}
	RESULT_VARIABLE result
	WORKING_DIRECTORY "${EMBEDDED_dlib_BUILD_DIR}"
	${echo_flag}
	)
	if(result)
		message(FATAL_ERROR "CMake step for dlib failed: ${result}")
	endif()

	execute_process(COMMAND
		${CMAKE_COMMAND} --build ${EMBEDDED_dlib_BUILD_DIR} --parallel --target install
	RESULT_VARIABLE result
	${echo_flag}
	)
	if(result)
		message(FATAL_ERROR "CMake make install step for dlib failed: ${result}")
	endif()
	message(STATUS "--- End running CMake")

	# Search again:
	set(dlib_DIR "${EMBEDDED_dlib_DIR}" CACHE PATH "Path to dlib CMake config file" FORCE)
	mark_as_advanced(dlib_DIR)
	find_package(dlib CONFIG REQUIRED)

    # install the embedded copy too (we need dlib-config.cmake, etc.)
	execute_process(COMMAND
		${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" "${EMBEDDED_dlib_SOURCE_DIR}"
		-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
	RESULT_VARIABLE result
	WORKING_DIRECTORY "${EMBEDDED_dlib_BUILD_DIR}"
	${echo_flag}
	)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} --build \"${EMBEDDED_dlib_BUILD_DIR}\" --target install)")
endif()

if (NOT dlib_FOUND)
    message(FATAL_ERROR "dlib not found, neither as system library nor git submodule. Check error messages above for possible reasons.")
endif()


# SiMpLE:
# ----------------------------
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/upstream/src ${CMAKE_CURRENT_BINARY_DIR}/simple)
#target_link_libraries(mola-lidar-odometry-cli-simple kiss_icp::pipeline)

# Find the required packages.
find_package(nanoflann REQUIRED)
find_package(TBB REQUIRED)
#find_package(dlib CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)
 
# Add an executable called simple.
# simple MOLA CLI interface:
mola_add_executable(
	TARGET  mola-lidar-odometry-cli-simple
	SOURCES
		apps/mola-lidar-odometry-cli-simple.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/upstream/src/ObjectiveFunction.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/upstream/src/ConfigParser.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/upstream/src/Scan.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/upstream/src/Map.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/upstream/src/Register.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/upstream/src/utils.cpp
	LINK_LIBRARIES
		mrpt::maps
		mrpt::tclap
		mola::mola_kernel
		nanoflann::nanoflann
		dlib::dlib
		TBB::tbb
		#yaml-cpp::yaml-cpp
)
target_include_directories(mola-lidar-odometry-cli-simple PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/upstream/src)

# MOLA dataset input modules:
if (mola_input_kitti_dataset_FOUND)
target_link_libraries(mola-lidar-odometry-cli-simple mola::mola_input_kitti_dataset)
target_compile_definitions(mola-lidar-odometry-cli-simple PRIVATE HAVE_MOLA_INPUT_KITTI)
endif()

if (mola_input_kitti360_dataset_FOUND)
target_link_libraries(mola-lidar-odometry-cli-simple mola::mola_input_kitti360_dataset)
target_compile_definitions(mola-lidar-odometry-cli-simple PRIVATE HAVE_MOLA_INPUT_KITTI360)
endif()

if (mola_input_mulran_dataset_FOUND)
target_link_libraries(mola-lidar-odometry-cli-simple mola::mola_input_mulran_dataset)
target_compile_definitions(mola-lidar-odometry-cli-simple PRIVATE HAVE_MOLA_INPUT_MULRAN)
endif()

if (mola_input_rawlog_FOUND)
target_link_libraries(mola-lidar-odometry-cli-simple mola::mola_input_rawlog)
target_compile_definitions(mola-lidar-odometry-cli-simple PRIVATE HAVE_MOLA_INPUT_RAWLOG)
endif()

if (mola_input_rosbag2_FOUND)
target_link_libraries(mola-lidar-odometry-cli-simple mola::mola_input_rosbag2)
target_compile_definitions(mola-lidar-odometry-cli-simple PRIVATE HAVE_MOLA_INPUT_ROSBAG2)
endif()

if (mola_input_paris_luco_dataset_FOUND)
target_link_libraries(mola-lidar-odometry-cli-simple mola::mola_input_paris_luco_dataset)
target_compile_definitions(mola-lidar-odometry-cli-simple PRIVATE HAVE_MOLA_INPUT_PARIS_LUCO)
endif()

# -----------------------------------------------------------------------------
#  ROS2
# -----------------------------------------------------------------------------
if (DETECTED_ROS2)
	# find dependencies
	find_package(ament_cmake REQUIRED)

	#ament_target_dependencies(MY_TARGET
	#  rclcpp...
	#)

	ament_package()
endif()
