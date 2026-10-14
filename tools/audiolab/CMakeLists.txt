# Copyright © 2026 CCP ehf.

# The Audio Lab: a native app that runs CarbonAudio through CarbonAudioHost, without Python or exefile.
# Built with -DCARBON_AUDIO_BUILD_TOOLS=ON, next to _audio2 so it finds Blue, Python and the other runtime libraries.
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

ccp_add_executable(AudioLab
	main.cpp
	BlueBoot.cpp
	BlueBoot.h
	LabApp.cpp
	LabApp.h
	LabChecks.cpp
	LabChecks.h
	LabEnginePanel.cpp
	LabEnginePanel.h
	LabLogPanel.cpp
	LabLogPanel.h
	LabPage.cpp
	LabPage.h
	LabScene.cpp
	LabScene.h
	LabSceneCanvas.cpp
	LabSceneCanvas.h
	LabSceneView.cpp
	LabSceneView.h
	LabSelection.cpp
	LabSelection.h
	LabSelectionPanel.cpp
	LabSelectionPanel.h
	LabSplash.cpp
	LabSplash.h
	LabSightlines.cpp
	LabSightlines.h
	LabUi.cpp
	LabUi.h
	LabWidgets.cpp
	LabWidgets.h
	LabWindow.cpp
	LabWindow.h
	PageOcclusion.cpp
	PagePortals.cpp
	PageSandbox.cpp
	Platform.cpp
	Platform.h
	SoundBankFolder.cpp
	SoundBankFolder.h
	WwiseAssertHook.cpp
	WwiseLink.cpp
	WwiseLink.h
)

# AkAutobahn is the Wwise SDK's WAAPI client, for the link to Wwise Authoring; it does not need the sound engine.
target_link_libraries(AudioLab PRIVATE
	CarbonAudio
	imgui::imgui
	nlohmann_json::nlohmann_json
	Wwise::Wwise
	AkAutobahn
)

target_compile_definitions(AudioLab PRIVATE
	# The lab's own Wwise project, with its rooms' reverbs and its speech, and the SoundBanks it generates; the test
	# project stays the Python tests' own.
	AUDIOLAB_DEFAULT_SOUNDBANKS="${CMAKE_CURRENT_SOURCE_DIR}/Wwise/SoundBanks"
	AUDIOLAB_WWISE_PROJECT="${CMAKE_CURRENT_SOURCE_DIR}/Wwise/AudioLab/AudioLab.wproj"
)
if(MSVC)
	# The paths above and the lab's strings are UTF-8, which MSVC would otherwise read in the ANSI code page: a checkout
	# path with other characters would not be found.
	target_compile_options(AudioLab PRIVATE /utf-8)
endif()

# The window: Win32 and Direct3D 11 on Windows, Cocoa and Metal on macOS, both through Dear ImGui's own backends.
if(WIN32)
	target_compile_definitions(AudioLab PRIVATE UNICODE=1 _UNICODE=1 NOMINMAX=1 WIN32_LEAN_AND_MEAN=1)
	target_link_libraries(AudioLab PRIVATE d3d11 dxgi shell32)
elseif(APPLE)
	target_sources(AudioLab PRIVATE LabWindow_Mac.mm)
	set_source_files_properties(LabWindow_Mac.mm PROPERTIES COMPILE_FLAGS -fobjc-arc)
	target_link_libraries(AudioLab PRIVATE
		"-framework Cocoa"
		"-framework Carbon"
		"-framework GameController"
		"-framework Metal"
		"-framework QuartzCore"
	)
endif()

set_target_properties(AudioLab PROPERTIES
	OUTPUT_NAME "audiolab"
	RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:CarbonAudio>
)
if(APPLE)
	set_target_properties(AudioLab PROPERTIES BUILD_RPATH "@loader_path")
endif()

if(BUILD_TESTING)
	# Each page's checks, run without a window: audiolab_<page> runs `--check <page>`. Local only: CI does not build
	# the tools.
	foreach(page sandbox occlusion portals)
		add_test(NAME audiolab_${page} COMMAND AudioLab --check ${page})
	endforeach()
endif()
