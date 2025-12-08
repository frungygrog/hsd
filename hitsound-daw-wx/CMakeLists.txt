cmake_minimum_required(VERSION 3.20)
project(hitsound-daw-wx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- wxWidgets ---
# Using v3.2.6 or master to avoid "compatibility with CMake < 3.5" errors on simplified environments
include(FetchContent)

set(wxBUILD_SHARED ON CACHE BOOL "Build wxWidgets as shared library" FORCE)
set(wxBUILD_SAMPLES OFF CACHE BOOL "Disable wxWidgets samples" FORCE)
set(wxBUILD_TESTS OFF CACHE BOOL "Disable wxWidgets tests" FORCE)
set(wxBUILD_DEMOS OFF CACHE BOOL "Disable wxWidgets demos" FORCE)
set(wxUSE_WEBVIEW ON CACHE BOOL "Enable WebView" FORCE)

FetchContent_Declare(
    wxWidgets
    GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
    GIT_TAG        master
)
# MakeAvailable will trigger configuration of the subproject
FetchContent_MakeAvailable(wxWidgets)

# --- JUCE ---
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(JUCE)

# --- Main Executable ---
add_executable(hitsound-daw-wx
    src/main.cpp
    src/ui/MainFrame.cpp
    src/ui/MainFrame.h
    src/ui/TimelineView.cpp
    src/ui/TimelineView.h
    src/ui/TrackList.cpp
    src/ui/TrackList.h
    src/ui/TransportPanel.cpp
    src/ui/TransportPanel.h
    src/ui/AddTrackDialog.cpp
    src/ui/AddTrackDialog.h
    src/ui/AddGroupingDialog.cpp
    src/ui/AddGroupingDialog.h
    src/audio/AudioEngine.cpp
    src/audio/AudioEngine.h
    src/audio/EventPlaybackSource.cpp
    src/audio/EventPlaybackSource.h
    src/audio/SampleRegistry.cpp
    src/audio/SampleRegistry.h
    src/model/SampleRef.h
    src/model/SampleTypes.h
    src/io/OsuParser.cpp
    src/io/OsuParser.h
    src/model/Track.h
    src/model/HitObject.h
    src/model/Project.h
)

# Link wxWidgets
target_link_libraries(hitsound-daw-wx PRIVATE wx::core wx::base)

# Link JUCE Modules
target_link_libraries(hitsound-daw-wx PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_events 
)

# Ensure Windows subsystems
set_target_properties(hitsound-daw-wx PROPERTIES WIN32_EXECUTABLE ON)
