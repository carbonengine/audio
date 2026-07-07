# Copyright © 2021 CCP ehf.

cmake_minimum_required(VERSION 3.16)
project(CarbonAudio VERSION 1.0.0)

include(cmake/CcpTargetConfigurations.cmake)
include(cmake/CcpBuildConfigurations.cmake)
include(cmake/CcpPackageConfigHelpers.cmake)

find_package(carbon-blue            CONFIG REQUIRED)
find_package(carbon-math            CONFIG REQUIRED)
find_package(carbon-trinityaudioapi CONFIG REQUIRED)
find_package(Wwise                  CONFIG REQUIRED)
find_package(TBB                    CONFIG REQUIRED)
find_package(Python3 COMPONENTS Development Interpreter REQUIRED)

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
	src/AudGeometry.cpp
	src/AudGeometry_Blue.cpp
	src/SpatialAudioSettings.cpp
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
	src/DebugUtilities.cpp
	src/SoundPrioritization.cpp
	src/Components/StretchAudio.cpp
	src/Components/StretchAudio_Blue.cpp
    src/LowLevelIO/AudPathResolver.cpp
    src/LowLevelIO/LowLevelIOHook.cpp
    src/WAAPI/WaapiManager.cpp
    src/WAAPI/WaapiManager_Blue.cpp
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
	src/AudGeometry.h
	src/SpatialAudioSettings.h
	src/autoversion.h
	src/DebugUtilities.h
	src/LogBridge.h
	src/resource.h
	src/stdafx.h
	src/Utilities.h
	src/Vector3.h
	src/version.h
	src/Components/StretchAudio.h
    src/LowLevelIO/FileHelpers.h
    src/LowLevelIO/AudPathResolver.h
    src/LowLevelIO/LowLevelIOHook.h
	src/IPrioritizedObject.h
	src/SoundPrioritization.h
    src/WAAPI/WaapiManager.h
	${PUBLIC_HEADER_FILES_INCLUDE}
)

set(WWISE_LIBRARIES
	Ak3DAudioBedMixerFX AkAudioInputSource AkAutobahn AkCompressorFX AkConvolutionReverbFX
	AkDelayFX AkFlangerFX AkGainFX AkGuitarDistortionFX AkHarmonizerFX
	AkMatrixReverbFX AkMemoryMgr AkMeterFX AkOpusDecoder AkParametricEQFX
	AkPeakLimiterFX AkPitchShifterFX AkRecorderFX AkRoomVerbFX AkSilenceSource AkSineSource
	AkSoundEngine AkSpatialAudio AkStereoDelayFX AkStreamMgr AkSynthOneSource AkTimeStretchFX
	AkToneSource AkTremoloFX AkVorbisDecoder AkSidechainFX MasteringSuiteFX
)

if(WIN32)
	list(APPEND WWISE_LIBRARIES $<IF:$<CONFIG:Release>,,wsock32>)
endif()
list(APPEND WWISE_LIBRARIES $<IF:$<CONFIG:Release>,,CommunicationCentral>)

target_link_libraries(CarbonAudio
		PUBLIC
			Blue
			CcpTrinityAudioAPI
		PRIVATE
			TBB::tbb
			CcpMath
			Wwise::Wwise
			${WWISE_LIBRARIES}
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
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
			$<INSTALL_INTERFACE:include>
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/src
		)

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
    option(INSTALL_TO_MONOLITH "Install this package to a perforce branch vendor folder - provided for backwards compatability" OFF)
    if(INSTALL_TO_MONOLITH)
        install(
            TARGETS CarbonAudio
            EXPORT CarbonAudioTargets
            CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
            ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}
            LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}
            RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}
        )

        # Generate target configuration for the monolith
        configure_ccp_vendor_config_file(
            TARGET CarbonAudio
            DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/CarbonAudioConfig.cmake"
            DEPENDENCIES "Blue;CcpTrinityAudioAPI"
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CarbonAudioConfig.cmake DESTINATION .)
    else()
        install(
            TARGETS CarbonAudio
            EXPORT CarbonAudioTargets
            CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
        )

        install(
            EXPORT CarbonAudioTargets
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/carbon-audio
            FILE carbon-audio.cmake
        )

        install(
            FILES carbon-audioConfig.cmake
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/carbon-audio
        )
    endif()

    # Install rule for available public headers
    install(DIRECTORY include DESTINATION . FILES_MATCHING PATTERN "*.h")

    # Install python package
    install(DIRECTORY python/ DESTINATION python
            FILES_MATCHING
            PATTERN "*.py"
            PATTERN "package.yaml"
            PATTERN "*.bnk"
            PATTERN "*.json"
    )

    option(BUILD_TESTING "Build tests. Enabled by default." ON)
    if(BUILD_TESTING)
        include(CTest)

        set(ENV{PYTHONPATH} ${CMAKE_CURRENT_SOURCE_DIR}/python)
        execute_process(COMMAND ${Python3_EXECUTABLE} discover.py
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

        if(DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE MATCHES "Release")
            set(EXEFILE_TOOL_NAME exefile_${CMAKE_BUILD_TYPE})
        else()
            set(EXEFILE_TOOL_NAME exefile)
        endif()
        find_program(exefile_tool ${EXEFILE_TOOL_NAME} NO_CACHE)

        if(WIN32)
            add_custom_command(TARGET CarbonAudio POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy ${exefile_tool} $<TARGET_FILE_DIR:CarbonAudio>
            )
            set(exefile_tool $<TARGET_FILE_DIR:CarbonAudio>/${EXEFILE_TOOL_NAME})
            set(TESTS_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/python)
        else()
            set(TESTS_WORKING_DIRECTORY $<TARGET_FILE_DIR:CarbonAudio>)
        endif()

        # Filter out command line arguments and only keep actual test names.
        # MacOS was not properly handling test discovery and taking all arguments as tests.
        string(REPLACE "\n" ";" PYTHON_TESTS_RAW "${PYTHON_TESTS_STR}")
        set(PYTHON_TESTS "")
        foreach(LINE ${PYTHON_TESTS_RAW})
            string(STRIP "${LINE}" LINE_STRIPPED)
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
                    COMMAND ${exefile_tool} /inherit /buildflavor=$<LOWER_CASE:$<CONFIG>> /py -m audiotests -v ${PYTHON_TEST}
                    WORKING_DIRECTORY ${TESTS_WORKING_DIRECTORY}
            )
            if(WIN32)
                set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT
                    "PYTHONPATH=$<TARGET_FILE_DIR:CarbonAudio>\;${CMAKE_CURRENT_SOURCE_DIR}/python\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/python\;${Python3_STDLIB};BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
            elseif(APPLE)
                set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT
                    "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/python:$<TARGET_FILE_DIR:CarbonAudio>:${CMAKE_CURRENT_SOURCE_DIR}/python:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/python3.12/lib-dynload:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/python:${Python3_STDLIB};BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
            endif()
        endforeach()
    endif()
endif()
