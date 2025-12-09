# Hitsound DAW (wx + JUCE) - Project Context
This document provides a comprehensive technical overview of the Hitsound DAW project, intended for AI agents to fully contextualize themselves with the codebase and its data.

## 1. Project Overview
**Hitsound DAW** is a desktop application designed for creating and editing osu! hitsound maps. It combines **wxWidgets** for the user interface and **JUCE** for the high-performance audio engine.

### Core Philosophy
- **Native Performance**: C++17 with direct OS UI controls (wxWidgets).
- **Audio Accuracy**: Low-latency playback using JUCE's audio device manager.
- **Osu-Native**: Built specifically to parse, visualize, and edit `.osu` format files (v128+).
---

## 2. File Structure & Key Paths
/ ├── Wham! - Last Christmas (Catawba)/ # Sample Project Data │
  │ ├──Wham! - Last Christmas (Catawba) [Special].osu # The source of truth map file │ 
  │ ├──audio.mp3 # Backing track 
  │ └── bg.png # Background image 
  ├── hitsound-daw-wx/ # Source Code 
  │ ├── src/ │ │ ├── main.cpp # Entry point (wxApp) 
  │ │ ├── audio/ # Audio Engine 
  │ │ │ ├── AudioEngine.cpp # JUCE Mixer & Transport management 
  │ │ │ ├── EventPlaybackSource.cpp # Custom AudioSource for triggering samples 
  │ │ │ └── SampleRegistry.cpp # Sample cache management 
  │ │ ├── io/ # File I/O 
  │ │ │ └── OsuParser.cpp # .osu parser & object-to-track mapper 
  │ │ ├── model/ # Data Models 
  │ │ │ ├── Project.h # Root model 
  │ │ │ └── Track.h # Track & Event structures 
  │ │ └── ui/ # User Interface 
  │ │ ├── MainFrame.cpp # Main Window Frame 
  │ │ ├── TimelineView.cpp # Custom wxScrolledWindow for timeline rendering 
  │ │ ├── TrackList.cpp # Left panel track controls 
  │ │ ├── TransportPanel.cpp # Playback & Zoom controls
  │ │ ├── AddTrackDialog.cpp/.h # Dialog for adding/editing standard tracks
  │ │ ├── AddGroupingDialog.cpp/.h # Dialog for adding/editing groupings
  │ │ ├── ProjectSetupDialog.cpp/.h # Project configuration dialog
  │ │ └── PresetDialog.cpp/.h # Dialog for loading track presets


---

## 3. Architecture & Data Flow
### A. Data Model (`model/`)
1.  **Project (`Project.h`)**:
    *   Holds metadata (Artist, Title, Creator, Version).
    *   `timingPoints`: Vector of bpm/offset data.
    *   `tracks`: Vector of `Track` objects.
    *   Note: `defaultSampleSet` is derived from the `[General]` section of the .osu file.
2.  **Track (`Track.h`)**:
    *   **Hierarchical**: Tracks can have `children`.
    *   **Logic**:
        *   **Parent Track**: Represents a specific Sound (e.g., "Normal-HitNormal").
        *   **Child Track**: Represents a Volume Layer (e.g., "60%").
    *   **Event**: Stores `time` (seconds) and `volume` (0.0-1.0).
    *   **ValidationState**: Tri-state enum (`Valid`, `Invalid`, `Warning`) for event validation feedback.
    *   **Sample Info**: `sampleSet` (Normal/Soft/Drum), `sampleType` (HitNormal/Whistle/Finish/Clap), `customFilename`.

### B. I/O & Parsing (`io/OsuParser.cpp`)
The parser transforms the flat `.osu` file format into the hierarchical Track model.
1.  **Sections Parsed**:
    *   `[General]`: AudioFilename, SampleSet (Global Default).
    *   `[Metadata]`: Title, Artist, Creator, Version.
    *   `[TimingPoints]`: Uninherited points define BPM/Offset. Inherited points define volume changes.
    *   `[HitObjects]`: The core gameplay data.
2.  **Resolution Logic**:
    *   Iterates through `HitObjects`.
    *   Determines volume/sampleSet based on the active TimingPoint at `time`.
    *   **Grouping**: Creates a unique key `Set-Type-Filename`.
    *   **Sub-Grouping**: Groups events by `Volume` into child tracks (e.g., "Soft-HitWhistle (60%)").
    *   *Critical*: Handles osu! fallback rules (e.g., if a HitObject has no specific addition, it falls back to the timing point's sample set, and finally to the Project's default).

### C. Audio Engine (`audio/`)
Uses JUCE for the audio backend.
*   `AudioEngine`: Manages `juce::AudioDeviceManager` and `juce::MixerAudioSource`.
*   `EventPlaybackSource`: A custom `juce::AudioSource`.
    *   Reads `Track` events.
    *   When the playhead passes an event time, it requests the sample from `SampleRegistry` and mixes it into the buffer.
*   **Latency Handling**: Uses JUCE's low latency ASIO/WASAPI drivers where available.

### D. Rendering (`ui/TimelineView.cpp`)
A custom `wxScrolledWindow` that performs high-performance 2D drawing.
*   **Coordinates**:
    *   `x`: Pixels represents Time. `x = time * pixelsPerSecond`.
    *   `y`: Pixels represents logical Track Rows.
*   **Optimization**:
    *   Uses `wxAutoBufferedPaintDC` for flicker-free double buffering.
    *   Calculates visible time range data (`visStart`, `visEnd`) to only draw visible events.
*   **Interaction**:
    *   **Selection**: Supports Click, Ctrl+Click, and Marquee selection.
    *   **Drag & Drop**: Implements "Ghost" dragging visualization before committing changes via `UndoManager`.
    *   **Looping**: Dragging the ruler creates a loop region.
*   **Validation Colors**:
    *   **Valid**: Blue (normal), Yellow (selected)
    *   **Invalid**: Red (normal), Orange (selected) - e.g., conflicting addition banks
    *   **Warning**: Pink (normal), Light Pink (selected) - e.g., addition without hitnormal backing
*   **Default Bank Selector**: Dropdown in TransportPanel (None/Normal/Soft/Drum). When set, placing an addition auto-places a matching hitnormal via `FindOrCreateHitnormalTrack()`.

### E. Track Management (`ui/TrackList.cpp`)
The TrackList panel provides track organization and context menu operations.
*   **Header Buttons**: "Add Track" and "Add Grouping" buttons in ruler area.
*   **Context Menus**:
    *   **Parent Track/Grouping**: Right-click shows "Add Child", "Edit", "Delete".
    *   **Child Track**: Right-click shows "Delete" only.
*   **Edit Dialogs**:
    *   `AddTrackDialog`: Configure name, bank (Normal/Soft/Drum), type (HitNormal/Whistle/Clap/Finish), volume. Supports edit mode via `SetEditMode(true)` and `SetValues()`.
    *   `AddGroupingDialog`: Configure name, HitNormal bank, additions bank, addition types (Whistle/Finish/Clap checkboxes), volume. Supports edit mode similarly.
*   **Track Deletion**: Parent deletion removes all children; child deletion only removes that child and adjusts `primaryChildIndex`.

### F. Presets (`ui/PresetDialog.cpp`, `MainFrame::ApplyPreset`)
Accessible via "Track" → "Load Preset..." menu. Presets add predefined track/grouping collections.
*   **Generic Preset** (60% volume):
    *   Tracks: Kick (n-hn), Hi-Hat (d-hn), Default (s-hn), Whistle (s-hw), Clap (s-hc), Crash (s-hf)
    *   Grouping: Snare (s-hn + s-hc)
---

## 4. "Last Christmas" Data Analysis
The "Last Christmas" project demonstrates the complexity the system must handle.
### The .osu File
*   **Format**: v128.
*   **Metadata**: Title "Last Christmas", Artist "Wham!", Creator "Catawba".
*   **Timing**:
    *   Multiple BPM changes and volume automation.
    *   Inherited timing points (negative beatLength) control volume (e.g., 60%, 40%, 75%).
*   **Hit Objects**:
    *   Standard HitCircles, Sliders, and Spinners.
    *   Complex hitsounding: Custom filenames (referenced but usually empty in standard maps), multiple additions (Whistle + Finish).

### Observed Behavior
*   The parser correctly identified the volume changes (60, 40, 75, etc.) and likely created separate child tracks for them.
*   Example parsed hierarchy:
    *   **Drum-HitNormal**
        *   60%
        *   75%
    *   **Soft-HitWhistle**
        *   40%
        *   60%
---

## 5. Technical Constraints & Conventions
*   **String Encoding**: wxWidgets uses `wxString`. Conversions to/from `std::string` use `FromUTF8()`/`ToStdString()`.
*   **Memory Management**:
    *   Audio: Raw pointers often used for audio callbacks (performance).
    *   UI: wxWidgets handles window destruction.
    *   Model: `std::unique_ptr` used for Commands (Undo/Redo).
*   **Thread Safety**: UI runs on Main Thread. Audio runs on High-Priority Audio Thread. `EventPlaybackSource` must be lock-free or use spinlocks to avoid audio glitches.

## 6. Known Coordinate Systems
*   **Timeline X**: `double time (seconds)`.
*   **Timeline Y**: Calculated dynamically based on visible tracks.
    *   Parent Track Height: 80px.
    *   Child Track Height: 40px.
    *   Header Height: ~20-30px (Ruler).
	
## IMPORTANT: Notes for agent.
Do not attempt to build the project upon completing a task. The user will do so on their own accord.

When you complete a task given, update the GEMINI.md (if you deem it as neccessary)

When reading files/running other tool calls, have zero regard for your context window and/or token usage. There is no need to be efficient, knowing as much as possible about the project is most important.