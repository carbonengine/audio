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

set(Wwise_ROOT "${BRANCH_ROOT_DIR}/vendor/wwise/2021.1.14.8108")

if(WIN32)
	# Hack to allow building Audio with v143 toolset. This will ensure that when we link against dependencies, we will link against the v141 version. This is necessary because our build configuration assumes that all components are built using the same version. However, things built against a newer 14X toolset should be able to link against things that were built using an older 14X toolset.
	set(_OLD_CCP_VENDOR_LIB_PATH ${CCP_VENDOR_LIB_PATH})
	set(_OLD_CCP_VENDOR_BIN_PATH ${CCP_VENDOR_BIN_PATH})
	set(CCP_VENDOR_LIB_PATH "lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/v141/")
	set(CCP_VENDOR_BIN_PATH "bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/v141/")
endif()
# Find v141 dependencies here.
find_package(Blue CONFIG REQUIRED)
find_package(CcpMath CONFIG REQUIRED)
find_package(IntelTBB REQUIRED)
find_package(Wwise REQUIRED)
if(WIN32)
	# Restore sanity after toolset hacking. After this we can use find_package
# to find any v143 dependencies.
	set(CCP_VENDOR_LIB_PATH ${_OLD_CCP_VENDOR_LIB_PATH})
set(CCP_VENDOR_BIN_PATH ${_OLD_CCP_VENDOR_BIN_PATH})
endif()

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

if (WIN32)
    include(CTest)
    message(STATUS ${IntelTBB_ROOT})
    set(_COPY_TOOLSET v141)  # This is a hack to copy the v141 binaries over until the v143 toolset update is done properly.

    # Copy required binaries for CarbonAudio tests into CarbonAudioTest folder
    add_custom_command(TARGET CarbonAudio POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Python_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/python27.dll"
            $<TARGET_FILE_DIR:CarbonAudio>
    )

    add_custom_command(TARGET CarbonAudio POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Blue_DIR}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/$<IF:$<CONFIG:Release>,blue.dll,blue_$<LOWER_CASE:$<CONFIG>>.dll>"
            $<TARGET_FILE_DIR:CarbonAudio>
    )

    add_custom_command(TARGET CarbonAudio POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${IntelTBB_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/tbb12.dll"
            $<TARGET_FILE_DIR:CarbonAudio>
    )

    add_custom_command(TARGET CarbonAudio POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Exefile_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/Exefile.exe"
            $<TARGET_FILE_DIR:CarbonAudio>
    )
    add_custom_command(TARGET CarbonAudio POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ExefileConsole_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${_COPY_TOOLSET}/ExefileConsole.exe"
            $<TARGET_FILE_DIR:CarbonAudio>
    )

    # Discover and run Python tests as a single CTest test.
    set(EXEFILE_CONSOLE_EXECUTABLE "$<TARGET_FILE_DIR:CarbonAudio>/ExefileConsole.exe")
    add_test(NAME PythonTests
            COMMAND ${EXEFILE_CONSOLE_EXECUTABLE} /buildflavor=$<LOWER_CASE:$<CONFIG>> /lib=${BRANCH_ROOT_DIR}/carbon/common/stdlib\\\;${CMAKE_CURRENT_SOURCE_DIR}/python\\\;${BRANCH_ROOT_DIR}/packages /bin=$<TARGET_FILE_DIR:CarbonAudio> /py run_tests.py
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/audio2/test
            COMMAND_EXPAND_LISTS
    )
endif()
