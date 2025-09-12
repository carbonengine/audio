cmake_minimum_required(VERSION 3.16)
project(CarbonAudio VERSION 1.0.0)

if(NOT DEFINED ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "Missing required environment variable CCP_EVE_PERFORCE_BRANCH_PATH")
elseif(NOT IS_ABSOLUTE $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be an absolute path")
elseif(NOT EXISTS $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH is pointing at a non-existing location $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
elseif(NOT IS_DIRECTORY $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be a directory")
else()
    message(STATUS "Using CCP_EVE_PERFORCE_BRANCH_PATH located at $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
endif()

file(TO_CMAKE_PATH "$ENV{CCP_EVE_PERFORCE_BRANCH_PATH}" BRANCH_ROOT_DIR)
list(INSERT CMAKE_MODULE_PATH 0 ${BRANCH_ROOT_DIR}/cmake)
include(CcpGlobalSettings)
include(CcpPackageConfigHelpers)

set(Wwise_ROOT "${BRANCH_ROOT_DIR}/vendor/wwise/2023.1.13.8732")

if(WIN32)
    # Hack to allow building Audio with v143 toolset. This will ensure that when we link against dependencies, we will link against the v141 version. This is necessary because our build configuration assumes that all components are built using the same version. However, things built against a newer 14X toolset should be able to link against things that were built using an older 14X toolset.
    set(_OLD_CCP_VENDOR_LIB_PATH ${CCP_VENDOR_LIB_PATH})
    set(_OLD_CCP_VENDOR_BIN_PATH ${CCP_VENDOR_BIN_PATH})
    set(CCP_VENDOR_LIB_PATH "lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/v141/")
    set(CCP_VENDOR_BIN_PATH "bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/v141/")
endif()

find_package(Blue CONFIG REQUIRED)
find_package(CcpMath CONFIG REQUIRED)
find_package(IntelTBB REQUIRED)
find_package(Wwise REQUIRED)
find_package(CarbonTrinityAudioAPI REQUIRED)

if(WIN32)
    # Restore sanity after toolset hacking. After this we can use find_package
    # to find any v143 dependencies.
    set(CCP_VENDOR_LIB_PATH ${_OLD_CCP_VENDOR_LIB_PATH})
    set(CCP_VENDOR_BIN_PATH ${_OLD_CCP_VENDOR_BIN_PATH})
endif()

set(PUBLIC_HEADER_FILES_INCLUDE
)

ccp_add_library(CarbonAudio SHARED
	src/AudListener_Blue.cpp
    src/AudActionLog.cpp
	src/AudActionLog_Blue.cpp
	src/AudManager.cpp
	src/AudConstants.cpp
	src/AudManager_Blue.cpp
	src/AudEmitter.cpp
	src/AudParameter.cpp
	src/AudParameter_Blue.cpp
	src/AudPosition.cpp
	src/AudPosition_Blue.cpp
	src/AudSettings_Blue.cpp
	src/AudUIPlayer.cpp
	src/AudEmitter_Blue.cpp
	src/AudUIPlayer_Blue.cpp
	src/AudMusicPlayer.cpp
	src/AudMusicPlayer_Blue.cpp
	src/AudEventCurve.cpp
	src/AudEventCurve_Blue.cpp
	src/LogBridge.cpp
	src/AudEventKey.cpp
	src/AudEventKey_Blue.cpp
	src/AudGameObjResource.cpp
	src/audio2.cpp
	src/AudGameObjResource_Blue.cpp
	src/stdafx.cpp
	src/AudListener.cpp
	src/AudioCurveSetDriver.cpp
	src/AudioCurveSetDriver_Blue.cpp
	src/AudioInputMgr.cpp
	src/AudioInputMgr_Blue.cpp
	src/AudStaticDataRepository.cpp
	src/AudStaticDataRepository_Blue.cpp
	src/CCPFilePackageLowLevelIO.h
	src/DebugUtilities.cpp
	src/LowLevelIO/Common/AkFileLocationBase.cpp
	src/LowLevelIO/Common/AkFilePackage.cpp
	src/LowLevelIO/Common/AkFilePackageLowLevelIO.inl
	src/LowLevelIO/Common/AkFilePackageLUT.cpp
	src/SoundPrioritization.cpp
	src/LowLevelIO/Common/AkMultipleFileLocation.cpp
    src/LowLevelIO/Common/AkGeneratedSoundBanksResolver.cpp
)

target_sources(CarbonAudio PRIVATE
    src/AudActionLog.h
	src/AudConstants.h
	src/AudEmitter.h
	src/AudEventCurve.h
	src/AudEventKey.h
	src/AudGameObjResource.h
	src/Audio2.h
	src/AudioCurveSetDriver.h
	src/AudioInputMgr.h
	src/AudListener.h
	src/AudManager.h
	src/AudParameter.h
	src/AudPosition.h
	src/AudSettings.h
	src/AudStaticDataRepository.h
	src/AudUIPlayer.h
	src/AudMusicPlayer.h
	src/autoversion.h
	src/DebugUtilities.h
	src/LogBridge.h
	src/resource.h
	src/stdafx.h
	src/Utilities.h
	src/Vector3.h
	src/version.h
	src/LowLevelIO/Common/AkFileLocationBase.h
	src/LowLevelIO/Common/AkFilePackage.h
	src/LowLevelIO/Common/AkFilePackageLUT.h
	src/LowLevelIO/Common/AkMultipleFileLocation.h
    src/LowLevelIO/Common/AkGeneratedSoundBanksResolver.h
	src/IPrioritizedObject.h
	src/SoundPrioritization.h
	${PUBLIC_HEADER_FILES_INCLUDE}
)


# Add Platform specific code
if(WIN32)
	target_sources(CarbonAudio PRIVATE
		src/LowLevelIO/Win32/AkDefaultIOHookDeferred.cpp
		src/LowLevelIO/Win32/AkDefaultIOHookDeferred.h
		src/LowLevelIO/Win32/AkFileHelpers.h
	)
elseif(APPLE)
	target_sources(CarbonAudio PRIVATE
		src/LowLevelIO/POSIX/AkDefaultIOHookDeferred.cpp
		src/LowLevelIO/POSIX/AkDefaultIOHookDeferred.h
		src/LowLevelIO/POSIX/AkFileHelpers.h
	)
else()
	message(FATAL_ERROR "CarbonAudio does not support platform specified.")
endif()

target_link_libraries(CarbonAudio
		PUBLIC
			Blue
			CarbonTrinityAudioAPI
		PRIVATE
			Intel::TBB
			CcpMath
			Wwise::Wwise
		)

if(APPLE)
	target_link_libraries(CarbonAudio PRIVATE 
		"-framework AudioToolbox"
		"-framework CoreAudio" 
		"-framework CoreFoundation"
		"-framework AVFoundation"
	)
endif()

target_precompile_headers(CarbonAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h)
target_include_directories(CarbonAudio
		PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/include
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/src
		)

# Platform-specific LowLevelIO include directories
if(WIN32)
	target_include_directories(CarbonAudio PRIVATE 
		${CMAKE_CURRENT_SOURCE_DIR}/src/LowLevelIO/Win32
	)
elseif(APPLE)
	target_include_directories(CarbonAudio PRIVATE 
		${CMAKE_CURRENT_SOURCE_DIR}/src/LowLevelIO/POSIX
	)
endif()

if(APPLE)
	set_target_properties(
			CarbonAudio
			PROPERTIES
			PREFIX ""
			SUFFIX ".so"
	)
endif()

if(WIN32)
    set(WWISE_PLATFORM Win64)
elseif(APPLE)
    set(WWISE_PLATFORM Mac)
else()
    set(WWISE_PLATFORM Linux64)
endif()

set_target_properties(CarbonAudio PROPERTIES OUTPUT_NAME "_audio2")

target_compile_definitions(CarbonAudio 
	PRIVATE 
		BLUE_OVERRIDE_VECTOR_TYPES=1
		TBB_ENABLED=1
)

# Unicode support for Windows
if(WIN32)
	target_compile_definitions(CarbonAudio PRIVATE
		UNICODE=1
		_UNICODE=1
	)
endif()

target_compile_definitions(CarbonAudio PUBLIC CARBONAUDIOBUILD)

set_target_properties(CarbonAudio PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME _audio2)

if(CMAKE_IMPORT_LIBRARY_SUFFIX)
	add_custom_command(TARGET CarbonAudio POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:CarbonAudio> $<TARGET_FILE_DIR:CarbonAudio>
			COMMAND_EXPAND_LISTS
			VERBATIM
	)
endif()


# Provide an install target iff this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
    install(
        TARGETS CarbonAudio
        EXPORT carbonAudioTargets 
        CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
        ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    )
    # Install rule for available public headers
    install(FILES ${PUBLIC_HEADER_FILES_INCLUDE} DESTINATION include)

    # Install a custom CarbonAudioConfig.cmake file, since as a dependency we are
    # basically a header-only library, not something that gets linked against.
    configure_ccp_vendor_config_file(
        TARGET CarbonAudio 
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/CarbonAudioConfig.cmake"
        DEPENDENCIES ""
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CarbonAudioConfig.cmake DESTINATION .)

    # Install python package
    install(DIRECTORY python/ DESTINATION python
            FILES_MATCHING
            PATTERN "*.py"
            PATTERN "package.yaml"
			PATTERN "*.bnk"
			PATTERN "*.json"
    )

	# Unit test related functionality
	include(CTest)

	if(WIN32)
    	set(_COPY_TOOLSET v141)  # This is a hack to copy the v141 binaries over until the v143 toolset update is done properly.

		# Detect Python version
		if(EXISTS "${Python_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/python312.dll")
			set(PYTHON_VERSION 3)
		elseif(EXISTS "${Python_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/python27.dll")
			set(PYTHON_VERSION 2)
		else()
			message(FATAL_ERROR "No compatible Python DLL found at ${Python_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/ - Checked for python27.dll and python312.dll")
		endif()

		if(PYTHON_VERSION EQUAL 3)
			message(STATUS "Python 3.12 detected, copying python312.dll")
			add_custom_command(TARGET CarbonAudio POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different
					"${Python_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/python312.dll"
					$<TARGET_FILE_DIR:CarbonAudio>
			)
			message(STATUS "Python 3.12 build - skipping Blue and TBB DLL copying")
			
			set(PYTHON_EXECUTABLE "${BRANCH_ROOT_DIR}/eve/client/pythonInterpreter.bat")

			# Hardcode scheduler paths to use v141 instead of find_package
			set(SCHEDULER_ROOT "${BRANCH_ROOT_DIR}/vendor/github.com/ccpgames/carbon-scheduler/v1.2.1-0-g4bb93aa")
			set(SCHEDULER_BIN_DIR "${SCHEDULER_ROOT}/bin/Windows/x64/v141")
			set(SCHEDULER_INCLUDE_DIR "${SCHEDULER_ROOT}/include")
			
			# Create imported target manually
			add_library(Scheduler SHARED IMPORTED)
			set_target_properties(Scheduler PROPERTIES
				IMPORTED_LOCATION "${SCHEDULER_BIN_DIR}/_scheduler_$<LOWER_CASE:$<CONFIG>>.pyd"
				INTERFACE_INCLUDE_DIRECTORIES "${SCHEDULER_INCLUDE_DIR}"
			)
			
		elseif(PYTHON_VERSION EQUAL 2)
			message(STATUS "Python 2.7 detected - enforcing strict DLL dependencies")
			
			target_link_libraries(CarbonAudio PRIVATE BlueExposure)

			add_custom_command(TARGET CarbonAudio POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different
					"${Python_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/python27.dll"
					"${Blue_DIR}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/$<IF:$<CONFIG:Release>,blue.dll,blue_$<LOWER_CASE:$<CONFIG>>.dll>"
					"${IntelTBB_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/tbb12.dll"
					"${Exefile_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/Exefile.exe"
					"${ExefileConsole_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/ExefileConsole.exe"
					$<TARGET_FILE_DIR:CarbonAudio>
			)

			set(EXEFILE_CONSOLE_EXECUTABLE "$<TARGET_FILE_DIR:CarbonAudio>/ExefileConsole.exe")
			add_test(NAME PythonTests
				COMMAND ${EXEFILE_CONSOLE_EXECUTABLE} /buildflavor=$<LOWER_CASE:$<CONFIG>> /lib=${BRANCH_ROOT_DIR}/carbon/common/stdlib\\\;${CMAKE_CURRENT_SOURCE_DIR}/tests/python\\\;${BRANCH_ROOT_DIR}/packages /bin=$<TARGET_FILE_DIR:CarbonAudio> /py -m audiotests
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/python
				COMMAND_EXPAND_LISTS
			)
			
			set(PYTHON_EXECUTABLE "${BRANCH_ROOT_DIR}/eve/client/pythonInterpreter.bat")
		endif()
	else()
		set(PYTHON_EXECUTABLE "${BRANCH_ROOT_DIR}/eve/client/pythonInterpreter.sh")
	endif()

	set(ENV{PYTHONPATH} ${CMAKE_CURRENT_SOURCE_DIR}/python)
	set(ENV{"CTEST_OUTPUT_ON_FAILURE"} 1)
	execute_process(COMMAND ${PYTHON_EXECUTABLE} discover.py
			OUTPUT_VARIABLE PYTHON_TESTS_STR
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE
			RESULT_VARIABLE PYTHON_TEST_DISCOVERY_RETVAL
			ERROR_VARIABLE PYTHON_TEST_DISCOVERY_ERRORS
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/python
	)
	if(${PYTHON_TEST_DISCOVERY_RETVAL})
		message(FATAL_ERROR "Error encountered during test discovery:\n=== stderr: ===\n${PYTHON_TEST_DISCOVERY_ERRORS}\n=== stdout: ===\n${PYTHON_TESTS_STR}")
	endif()
	
	# Filter out command line arguments and only keep actual test names
	# This is an unfortunate necessity because MacOS was not properly handling test discovery and taking all arguments as tests.
	string(REPLACE "\n" ";" PYTHON_TESTS_RAW ${PYTHON_TESTS_STR})
	set(PYTHON_TESTS "")
	foreach(LINE ${PYTHON_TESTS_RAW})
		string(STRIP "${LINE}" LINE_STRIPPED)
		# Skip empty lines, argument files (@...), command flags (/...), script names (*.py)
		# Only include lines that look like test names (contain dots for module.class.method)
		if(NOT "${LINE_STRIPPED}" STREQUAL "" AND 
		   NOT "${LINE_STRIPPED}" MATCHES "^@.*" AND 
		   NOT "${LINE_STRIPPED}" MATCHES "^/.*" AND 
		   NOT "${LINE_STRIPPED}" MATCHES ".*\\.py$" AND
		   "${LINE_STRIPPED}" MATCHES ".*\\..*")
			list(APPEND PYTHON_TESTS "${LINE_STRIPPED}")
			message(STATUS "Found valid test ${LINE_STRIPPED}")
		else()
			message(STATUS "Filtering out non-test line: ${LINE_STRIPPED}")
		endif()
	endforeach()
	foreach(PYTHON_TEST ${PYTHON_TESTS})
		add_test(NAME ${PYTHON_TEST}
				COMMAND ${PYTHON_EXECUTABLE} -m audiotests -v ${PYTHON_TEST}
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/python
		)
		if(WIN32)
			set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:CarbonAudio>\;${CMAKE_CURRENT_SOURCE_DIR}/python;BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
		elseif (APPLE)
			set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:CarbonAudio>:${CMAKE_CURRENT_SOURCE_DIR}/python;BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
		endif()
	endforeach()
endif()