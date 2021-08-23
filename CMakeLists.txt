project(audio2 VERSION 1.0.0)

add_library(audio2 SHARED
	AudListener_Blue.cpp
    AudActionLog.cpp
	AudActionLog_Blue.cpp
	AudManager.cpp
	AudConstants.cpp
	AudManager_Blue.cpp
	AudEmitter.cpp
	AudParameter.cpp
	AudEmitterDoppler.cpp
	AudParameter_Blue.cpp
	AudEmitterDoppler_Blue.cpp
	AudPosition.cpp
	AudEmitterMulti.cpp
	AudPosition_Blue.cpp
	AudEmitterMulti_Blue.cpp
	AudResource.cpp
	AudEmitterPersisted.cpp
	AudSettings_Blue.cpp
	AudEmitterPersisted_Blue.cpp
	AudUIPlayer.cpp
	AudEmitter_Blue.cpp
	AudUIPlayer_Blue.cpp
	AudEventCurve.cpp
	AudEventCurve_Blue.cpp
	LogBridge.cpp
	AudEventKey.cpp
	AudEventKey_Blue.cpp
	AudGameObjResource.cpp
	audio2.cpp
	AudGameObjResource_Blue.cpp
	stdafx.cpp
	AudListener.cpp
	AudioInputMgr.cpp
	AudioInputMgr_Blue.cpp
	DebugUtilities.cpp
	LowLevelIO/AkFileLocationBase.cpp
	LowLevelIO/AkFilePackage.cpp
	LowLevelIO/AkFilePackageLowLevelIO.inl
	LowLevelIO/AkFilePackageLUT.cpp
)

target_sources(audio2 PRIVATE
    AudActionLog.h
	AudConstants.h
	AudEmitter.h
	AudEmitterDoppler.h
	AudEmitterMulti.h
	AudEmitterPersisted.h
	AudEventCurve.h
	AudEventKey.h
	AudGameObjResource.h
	Audio2.h
	AudioInputMgr.h
	AudListener.h
	AudManager.h
	AudParameter.h
	AudPosition.h
	AudResource.h
	AudSettings.h
	AudUIPlayer.h
	autoversion.h
	DebugUtilities.h
	LogBridge.h
	resource.h
	stdafx.h
	Utilities.h
	Vector3.h
	version.h
	LowLevelIO/AkFileLocationBase.h
	LowLevelIO/AkFilePackage.h
	LowLevelIO/AkFilePackageLUT.h
)


# Add Platform specific code
if(WIN32)
	target_sources(audio2 PRIVATE
		LowLevelIO/Win32/AkDefaultIOHookBlocking.cpp
		LowLevelIO/Win32/AkDefaultIOHookBlocking.h
		LowLevelIO/Win32/AkFileHelpers.h
		LowLevelIO/Win32/AkFilePackageLowLevelIOBlocking.h
	)
elseif(APPLE)
	target_sources(audio2 PRIVATE
		LowLevelIO/POSIX/AkDefaultIOHookBlocking.cpp
		LowLevelIO/POSIX/AkDefaultIOHookBlocking.h
		LowLevelIO/POSIX/AkFileHelpers.h
		LowLevelIO/POSIX/AkFilePackageLowLevelIOBlocking.h
	)
else()
	message(FATAL_ERROR "Audio2 does not support platform specified.")
endif()



target_link_libraries(audio2
		PUBLIC
			Blue
		PRIVATE
			BlueExposure
			CcpMath
			Wwise::Wwise
		)
if(APPLE)
	target_link_libraries(audio2 PRIVATE "-framework AudioToolbox -framework CoreAudio -framework CoreFoundation")
endif()

target_precompile_headers(audio2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h)
target_include_directories(audio2
		PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/.
		)

if(APPLE)
	set_target_properties(
			audio2
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

set_target_properties(audio2 PROPERTIES OUTPUT_NAME "_audio2")

target_compile_definitions(audio2 PRIVATE BLUE_OVERRIDE_VECTOR_TYPES=1)
target_compile_definitions(audio2 PUBLIC AUDIO2BUILD)

set_target_properties(audio2 PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME _audio2)

