# Copyright © 2026 CCP ehf.

# Test-only Blue module for the Python tests. Built with the tests, never installed.
ccp_add_library(CarbonAudioTests SHARED
	FakeObstructionQuery.cpp
)

target_link_libraries(CarbonAudioTests PRIVATE Blue)
target_include_directories(CarbonAudioTests PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Next to _audio2, which is already on the tests' PYTHONPATH.
set_target_properties(CarbonAudioTests PROPERTIES
	OUTPUT_NAME "_audiotests"
	RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:CarbonAudio>
	LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:CarbonAudio>
)
