# Copyright © 2026 CCP ehf.

include(CMakeFindDependencyMacro)
include(${CMAKE_CURRENT_LIST_DIR}/carbon-audio.cmake)

find_dependency(carbon-blue            CONFIG REQUIRED)
find_dependency(carbon-trinityaudioapi CONFIG REQUIRED)
