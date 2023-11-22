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

find_package(Blue CONFIG REQUIRED)
find_package(CcpMath CONFIG REQUIRED)
find_package(IntelTBB REQUIRED)
find_package(Wwise REQUIRED)

set(PUBLIC_HEADER_FILES_INCLUDE
	Include/ITr2AudEmitter.h
	include/IAudioInputMgr.h
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
	src/AudioInputMgr.cpp
	src/AudioInputMgr_Blue.cpp
	src/AudStaticDataRepository.cpp
	src/AudStaticDataRepository_Blue.cpp
	src/DebugUtilities.cpp
	src/LowLevelIO/AkFileLocationBase.cpp
	src/LowLevelIO/AkFilePackage.cpp
	src/LowLevelIO/AkFilePackageLowLevelIO.inl
	src/LowLevelIO/AkFilePackageLUT.cpp
)

target_sources(CarbonAudio PRIVATE
    src/AudActionLog.h
	src/AudConstants.h
	src/AudEmitter.h
	src/AudEventCurve.h
	src/AudEventKey.h
	src/AudGameObjResource.h
	src/Audio2.h
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
	src/LowLevelIO/AkFileLocationBase.h
	src/LowLevelIO/AkFilePackage.h
	src/LowLevelIO/AkFilePackageLUT.h
	${PUBLIC_HEADER_FILES_INCLUDE}
)


# Add Platform specific code
if(WIN32)
	target_sources(CarbonAudio PRIVATE
		src/LowLevelIO/Win32/AkDefaultIOHookBlocking.cpp
		src/LowLevelIO/Win32/AkDefaultIOHookBlocking.h
		src/LowLevelIO/Win32/AkFileHelpers.h
		src/LowLevelIO/Win32/AkFilePackageLowLevelIOBlocking.h
	)
elseif(APPLE)
	target_sources(CarbonAudio PRIVATE
		src/LowLevelIO/POSIX/AkDefaultIOHookBlocking.cpp
		src/LowLevelIO/POSIX/AkDefaultIOHookBlocking.h
		src/LowLevelIO/POSIX/AkFileHelpers.h
		src/LowLevelIO/POSIX/AkFilePackageLowLevelIOBlocking.h
	)
else()
	message(FATAL_ERROR "CarbonAudio does not support platform specified.")
endif()



target_link_libraries(CarbonAudio
		PUBLIC
			Blue
		PRIVATE
			BlueExposure
			CcpMath
			Wwise::Wwise
            Intel::TBB
		)
if(APPLE)
	target_link_libraries(CarbonAudio PRIVATE "-framework AudioToolbox -framework CoreAudio -framework CoreFoundation")
endif()

target_precompile_headers(CarbonAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h)
target_include_directories(CarbonAudio
		PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/include
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/src
		)

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
target_compile_definitions(CarbonAudio PUBLIC CARBONAUDIOBUILD)

set_target_properties(CarbonAudio PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME _audio2)

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
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	option(BUILD_TESTING "Build tests. Enabled by default." ON)
	include(CTest)

	if(WIN32)
		set(INTERPRETER_EXTENSION "bat")
	elseif(APPLE)
		set(INTERPRETER_EXTENSION "sh")
	else()
		message(FATAL_ERROR "CarbonAudio does not support platform specified.")
	endif()

	# Discover and run Python tests as a single CTest test.
	set(PYTHON_INTERPRETER_PATH "${BRANCH_ROOT_DIR}/eve/client/pythonInterpreter.${INTERPRETER_EXTENSION}")
	set(PYTHON_TEST PythonTests)
	add_test(NAME ${PYTHON_TEST}
			COMMAND ${PYTHON_INTERPRETER_PATH} run_tests.py
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/audio2/test
			COMMAND_EXPAND_LISTS
	)
	set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:CarbonAudio>;BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
endif()