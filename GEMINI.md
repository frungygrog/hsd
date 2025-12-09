# Hitsound DAW - Complete Project Context

> **Purpose**: This document provides comprehensive technical context for AI agents to fully understand the Hitsound DAW project. It is designed to be verbose and exhaustive, covering every major system, data structure, workflow, and design decision in the codebase.

---

## 1. Project Overview

**Hitsound DAW** is a desktop digital audio workstation (DAW) purpose-built for creating and editing osu! hitsound maps. It combines:
- **wxWidgets** for the native user interface (cross-platform UI controls)
- **JUCE** for high-performance audio playback, sample management, and device I/O

### 1.1 Core Philosophy

| Principle | Description |
|-----------|-------------|
| **Native Performance** | Written in C++20 with direct OS UI controls via wxWidgets. No web technologies or interpreted layers. |
| **Audio Accuracy** | Low-latency playback using JUCE's `AudioDeviceManager` with WASAPI/ASIO support. Events trigger samples with sub-block timing precision. |
| **osu!-Native** | Built specifically to parse, visualize, edit, and export `.osu` format files (v14 stable, v128 lazer). |
| **Hierarchical Track Model** | Tracks are organized as parent containers with volume-specific child tracks, matching how osu! hitsounding is typically structured. |

### 1.2 Technology Stack

| Component | Technology | Version |
|-----------|------------|---------|
| Build System | CMake | 3.20+ |
| C++ Standard | C++20 | Required |
| UI Framework | wxWidgets | master branch (3.2.6+) |
| Audio Framework | JUCE | 8.0.0 |
| Platform | Windows | Primary target |

---

## 2. Directory Structure

```
/hsd                                        # Project Root
├── GEMINI.md                               # This file - AI agent context
├── osu_documentation.md                    # Official .osu file format specification
├── gemini_old.md                           # Deprecated context file (backup)
│
└── hitsound-daw-wx/                        # Main application source
    ├── CMakeLists.txt                      # Build configuration
    ├── Resources/                          # Default sample files and icons
    │   ├── normal-hitnormal.wav            # Default hitsound samples
    │   ├── soft-hitwhistle.wav             # (all 12 combinations)
    │   ├── drum-hitclap.wav
    │   └── icon/*.svg                      # UI icons (loaded dynamically)
    │
    └── src/
        ├── main.cpp                        # Application entry point (wxApp)
        ├── Constants.h                     # Global constants (track heights, timers, defaults)
        │
        ├── model/                          # Data models
        │   ├── Track.h                     # Track and Event structures
        │   ├── Project.h                   # Project container with metadata
        │   ├── SampleTypes.h               # SampleSet and SampleType enums
        │   ├── SampleRef.h                 # Sample reference for audio lookup
        │   ├── HitObject.h/.cpp            # Intermediate parsing structure
        │   ├── Command.h/.cpp              # Undo/Redo command base class + UndoManager
        │   ├── Commands.h/.cpp             # Concrete command implementations
        │   └── ProjectValidator.h/.cpp     # Validation logic for events
        │
        ├── audio/                          # Audio engine
        │   ├── AudioEngine.h/.cpp          # JUCE device management, transport, mixing
        │   ├── EventPlaybackSource.h/.cpp  # Sample triggering for track events
        │   └── SampleRegistry.h/.cpp       # Sample file caching and lookup
        │
        ├── io/                             # File I/O
        │   ├── OsuParser.h/.cpp            # .osu file parser
        │   └── ProjectSaver.h/.cpp         # .osu file exporter
        │
        └── ui/                             # User interface
            ├── MainFrame.h/.cpp            # Main application window
            ├── TimelineView.h/.cpp         # Timeline canvas (events, waveform, grid)
            ├── TrackList.h/.cpp            # Left panel track controls
            ├── TransportPanel.h/.cpp       # Playback controls, tools, volume sliders
            ├── AddTrackDialog.h/.cpp       # Dialog for adding/editing tracks
            ├── AddGroupingDialog.h/.cpp    # Dialog for adding/editing groupings
            ├── ProjectSetupDialog.h/.cpp   # Initial project selection dialog
            ├── PresetDialog.h/.cpp         # Preset loading dialog
            ├── CreatePresetDialog.h/.cpp   # Preset creation placeholder dialog
            └── ValidationErrorsDialog.h/.cpp # Validation error display
```

---

## 3. Data Model (model/)

### 3.1 Core Types

#### SampleSet (SampleTypes.h)
```cpp
enum class SampleSet {
    Normal,  // 0 - Mapped to 1 in .osu
    Soft,    // 1 - Mapped to 2 in .osu
    Drum     // 2 - Mapped to 3 in .osu
};
```

#### SampleType (SampleTypes.h)
```cpp
enum class SampleType {
    HitNormal,      // Base sample (always plays)
    HitWhistle,     // Addition - whistle sound
    HitFinish,      // Addition - cymbal/finish sound
    HitClap,        // Addition - clap sound
    
    // Reserved for future slider support
    SliderSlide,
    SliderTick,
    SliderWhistle
};
```

### 3.2 Event Structure (Track.h)

```cpp
struct Event {
    uint64_t id;                          // Unique ID for reliable undo/redo matching
    double time;                          // Timestamp in SECONDS (not milliseconds)
    double volume = 1.0;                  // Volume multiplier (0.0 - 1.0)
    ValidationState validationState;     // Valid, Invalid, or Warning
};
```

**Key Points:**
- `id` is auto-generated from `g_nextEventId` atomic counter
- `time` is in seconds internally (converted from/to milliseconds for .osu files)
- `volume` is stored as a float (0.0-1.0) representing percentage

#### ValidationState (Track.h)
```cpp
enum class ValidationState {
    Valid,    // Blue color - properly configured hitsound
    Invalid,  // Red color - conflicting banks or duplicate normals
    Warning   // Pink color - addition without backing hitnormal
};
```

### 3.3 SampleLayer Structure (Track.h)

```cpp
struct SampleLayer {
    SampleSet bank;
    SampleType type;
};
```

Used for **grouping tracks** that play multiple samples simultaneously (e.g., a snare sound with hitnormal + clap).

### 3.4 Track Structure (Track.h)

```cpp
struct Track {
    std::string name;                     // Display name
    
    // Sample metadata
    SampleSet sampleSet = SampleSet::Normal;
    SampleType sampleType = SampleType::HitNormal;
    std::string customFilename;           // Custom sample filename (if overridden)
    
    // Composite layers (for groupings)
    std::vector<SampleLayer> layers;      // If non-empty, these are played instead of sampleSet/Type
    
    // Playback properties
    double gain = 1.0;                    // Volume multiplier (0.0-1.0)
    bool mute = false;
    bool solo = false;
    
    std::vector<Event> events;            // Timeline events
    
    // Hierarchy
    std::vector<Track> children;          // Child tracks (volume layers)
    bool isExpanded = false;              // UI collapsed/expanded state
    int primaryChildIndex = 0;            // Default child for collapsed event placement
    
    // Flags
    bool isGrouping = false;              // True if this is a composite track
    bool isChildTrack = false;            // True if nested inside a parent
};
```

**Hierarchy Model:**
- **Parent Tracks**: Represent a specific sound (e.g., "soft-hitnormal")
- **Child Tracks**: Represent volume layers (e.g., "soft-hitnormal (60%)")
- **Groupings**: Special parent tracks that play multiple samples simultaneously

### 3.5 Project Structure (Project.h)

```cpp
struct Project {
    std::vector<Track> tracks;
    
    struct TimingPoint {
        double time;          // Milliseconds from start
        double beatLength;    // Milliseconds per beat (for BPM calculation)
        int sampleSet;        // 0=auto, 1=normal, 2=soft, 3=drum
        double volume;        // 0-100
        bool uninherited;     // True = red line (BPM), False = green line (SV)
    };
    std::vector<TimingPoint> timingPoints;
    
    double bpm = 120.0;                   // Calculated from first uninherited point
    double offset = 0.0;                  // First timing point time
    
    // Metadata
    std::string artist;
    std::string title;
    std::string version;                  // Difficulty name
    std::string creator;                  // Mapper name
    
    SampleSet defaultSampleSet;           // From [General] SampleSet
    
    // File paths
    std::string audioFilename;            // Relative path to audio file
    std::string projectDirectory;         // Absolute path to project folder
    std::string projectFilePath;          // Absolute path to .osu file
};
```

---

## 4. Audio Engine (audio/)

### 4.1 AudioEngine (AudioEngine.h/.cpp)

The central audio manager that coordinates playback.

**Inheritance:**
- Inherits from `juce::HighResolutionTimer` for loop boundary checking

**Key Members:**
```cpp
juce::AudioDeviceManager deviceManager;     // Hardware I/O
juce::AudioSourcePlayer audioSourcePlayer;  // Connects to device
juce::MixerAudioSource mixer;               // Combines all sources
juce::AudioFormatManager formatManager;     // File format support

juce::AudioTransportSource masterTransport; // Master audio playback
EventPlaybackSource eventPlaybackSource;    // Event-triggered samples
SampleRegistry sampleRegistry;              // Sample cache

double masterOffset = -0.029;               // Latency compensation (seconds)
```

**Key Methods:**
| Method | Description |
|--------|-------------|
| `initialize()` | Sets up audio device, connects mixer, starts loop timer |
| `LoadMasterTrack(path)` | Loads the backing audio track (mp3/wav) |
| `GetWaveform(numSamples)` | Generates waveform peaks for visualization |
| `SetTracks(tracks*)` | Provides track pointer to EventPlaybackSource |
| `NotifyTracksChanged()` | Signals that UI modified tracks (thread-safe snapshot update) |
| `SetMasterVolume(float)` | Controls backing track volume (0.0-1.0) |
| `SetEffectsVolume(float)` | Controls hitsound volume (0.0-1.0) |
| `SetLoopPoints(start, end)` | Configures loop region |

**Thread Safety:**
- Audio runs on high-priority audio thread
- UI thread must call `NotifyTracksChanged()` after any track modification
- `EventPlaybackSource` maintains a thread-safe snapshot via `juce::SpinLock`

### 4.2 EventPlaybackSource (EventPlaybackSource.h/.cpp)

Custom `juce::AudioSource` that triggers samples when the playhead passes events.

**Key Concepts:**

1. **Snapshot Pattern**: The audio thread reads from `tracksSnapshot`, not the live UI tracks
2. **Voice System**: Active samples are stored as `Voice` objects with position/gain
3. **Grouping Support**: Grouping tracks trigger all children's samples
4. **Layer Support**: Tracks with `layers` play all layer samples simultaneously

**Sample Trigger Logic (simplified):**
```cpp
// For each track/event
if (eventStartSample >= currentSample && eventStartSample < endSample) {
    // Trigger sample from SampleRegistry
    if (track.layers.empty()) {
        playSample(track.sampleSet, track.sampleType);
    } else {
        for (const auto& layer : track.layers) {
            playSample(layer.bank, layer.type);
        }
    }
}
```

### 4.3 SampleRegistry (SampleRegistry.h/.cpp)

Caches and provides access to sample files.

**Two Sample Sources:**
1. **Project Samples**: Custom samples specified in beatmap
2. **Default Samples**: Standard osu! samples from Resources folder

**Default Sample Naming:**
```
{set}-hit{type}.wav
Examples: normal-hitnormal.wav, soft-hitwhistle.wav, drum-hitclap.wav
```

---

## 5. File I/O (io/)

### 5.1 OsuParser (OsuParser.h/.cpp)

Parses `.osu` files into the Project model.

**Sections Parsed:**
| Section | Data Extracted |
|---------|----------------|
| `[General]` | AudioFilename, SampleSet (global default) |
| `[Metadata]` | Title, Artist, Creator, Version |
| `[TimingPoints]` | BPM, volume, sample set per timing section |
| `[HitObjects]` | Events with timing, sample set, volume |

**Resolution Logic:**
1. Parse HitObjects to get timing and hitsound flags
2. Resolve sample set using: Object → Timing Point → Project Default
3. Group events by `{SampleSet}-{SampleType}` to create parent tracks
4. Sub-group by volume percentage to create child tracks

**Key Method:**
```cpp
static Project parse(const juce::File& file);
static bool CreateHitsoundDiff(const juce::File& ref, const juce::File& target);
```

`CreateHitsoundDiff()` creates a new hitsound difficulty from a reference, copying timing points but with empty HitObjects.

### 5.2 ProjectSaver (ProjectSaver.h/.cpp)

Exports the Project model back to `.osu` format.

**Key Features:**
1. Sets Creator to "hsd" (application watermark)
2. Merges simultaneous events into single HitObjects with combined hitsound bitmask
3. Only exports uninherited timing points (red lines)
4. Uses position `256,192` for all hit objects (center of playfield)

**HitObject Bitmask:**
```
Bit 0: (unused in output - Normal is implicit)
Bit 1: Whistle
Bit 2: Finish
Bit 3: Clap
```

---

## 6. User Interface (ui/)

### 6.1 MainFrame (MainFrame.h/.cpp)

The main application window that orchestrates all components.

**Layout:**
```
┌─────────────────────────────────────────────────────────┐
│ TransportPanel (height: ~90px)                          │
│ [Play][Stop][Loop] | 00:00:000 | Bank: Snap: | Vol Vol  │
├──────────────┬──────────────────────────────────────────┤
│              │                                          │
│   TrackList  │              TimelineView                │
│   (250px)    │     (scrollable canvas with events)      │
│              │                                          │
└──────────────┴──────────────────────────────────────────┘
```

**Key Responsibilities:**
- File menu: Open, Save, Save As
- Edit menu: Undo, Redo, Cut, Copy, Paste, Delete, Select All
- Transport menu: Play/Stop, Rewind
- Track menu: Load Preset (Generic), Create Preset

**Scroll Synchronization:**
- TrackList mirrors TimelineView's vertical scroll position
- Implemented via event listeners and `SetVerticalScrollOffset()`

### 6.2 TimelineView (TimelineView.h/.cpp)

The main canvas for event visualization and editing. Extends `wxScrolledWindow`.

**Code Organization:**
TimelineView uses extracted helper methods for maintainability:

| Category | Methods |
|----------|--------|
| **Painting** | `DrawRuler()`, `DrawMasterTrack()`, `DrawTrackBackgrounds()`, `DrawGrid()`, `DrawTimingPoints()`, `DrawEvents()`, `DrawLoopRegion()`, `DrawDragGhosts()`, `DrawMarquee()`, `DrawPlayhead()` |
| **Mouse Events** | `HandleLeftDown()`, `HandleDragging()`, `HandleLeftUp()`, `HandleRightDown()` |
| **Event Placement** | `PlaceEvent()` - handles auto-hitnormal logic |

**Coordinate System:**
- X-axis: Time (pixels = time × pixelsPerSecond)
- Y-axis: Track rows (header + visible tracks)

**Layout Constants (from Constants.h):**
```cpp
RulerHeight = 30;
MasterTrackHeight = 100;
HeaderHeight = 130;           // Ruler + Master
ParentTrackHeight = 80;
ChildTrackHeight = 40;
```

**Tool Types:**
```cpp
enum class ToolType {
    Select,  // Click/drag to select, drag to move
    Draw     // Click to place, right-click to delete
};
```

**Key Features:**

| Feature | Description |
|---------|-------------|
| **Waveform Display** | Master track shows audio waveform |
| **Grid Lines** | Beat-aligned vertical lines based on gridDivisor |
| **Timing Points** | Green vertical lines at BPM changes |
| **Loop Region** | Draggable region in ruler (green overlay) |
| **Marquee Selection** | Box selection with Ctrl+drag for toggle |
| **Ghost Dragging** | Visual preview of moved events during drag |
| **Validation Colors** | Events colored by validation state |

**Snapping:**
```cpp
double snapStep = beatLength / gridDivisor;
// gridDivisor: 1, 2, 3, 4, 6, 8, 12, or 16
```

**Auto-Hitnormal Placement:**
When `defaultHitnormalBank` is set and placing an addition:
1. Find or create a hitnormal track with matching bank and volume
2. Check if any hitnormal exists at that timestamp (any bank)
3. If not, add both hitnormal and addition events together
4. Uses `AddMultipleEventsCommand` for atomic undo

**Callbacks:**
- `OnLoopPointsChanged` - Loop region changed
- `OnZoomChanged` - Zoom level changed
- `OnTracksModified` - Events modified (for audio sync)
- `OnPlayheadScrubbed` - User dragged playhead

### 6.3 TrackList (TrackList.h/.cpp)

Left panel showing track hierarchy and controls.

**Code Organization:**
TrackList uses extracted helper methods for maintainability:

| Category | Methods |
|----------|--------|
| **Painting** | `DrawHeader()`, `DrawTrackRow()`, `DrawTrackControls()`, `DrawSlider()`, `DrawPrimarySelector()`, `DrawAbbreviation()`, `DrawDropIndicator()` |
| **Hit Testing** | `FindTrackAtY()`, `HandleHeaderClick()`, `HandleTrackClick()` |
| **Context Menus** | `ShowParentContextMenu()`, `ShowChildContextMenu()` |
| **Track Operations** | `AddChildToTrack()`, `EditTrack()`, `DeleteTrack()` |
| **Utilities** | `GetAbbreviation()`, `UpdateTrackNameWithVolume()` |

**Layout per Track:**

**Parent Track (80px):**
```
┌────────────────────────────────────────┐
│ [+/-] Track Name                       │
│ [M] [S]                    [P] [60%▼]  │
│                                  n-hn  │
└────────────────────────────────────────┘
```

**Child Track (40px):**
```
┌────────────────────────────────────────┐
│ • track-name (60%)                     │
│    ═══════════○══════════              │
└────────────────────────────────────────┘
```

**Controls:**
| Control | Description |
|---------|-------------|
| `[+/-]` | Expand/collapse button |
| `[M]` | Mute toggle |
| `[S]` | Solo toggle |
| `[P] [%▼]` | Primary child selector (for collapsed parent) |
| Volume Slider | For child tracks - adjusts gain |
| Abbreviation | e.g., "n-hn", "s-hw + s-hc" |

**Header Buttons:**
- "Add Track" - Opens AddTrackDialog
- "Add Grouping" - Opens AddGroupingDialog

**Context Menu:**
- Parent: "Add Child", "Edit", "Delete"
- Child: "Delete"

**Track Reordering:**
- Drag tracks to reorder
- Visual insertion line shows drop target

### 6.4 TransportPanel (TransportPanel.h/.cpp)

Transport and tool controls at the top of the window.

**Layout:**
```
[▶][■][🔁] | 00:00:000 | Bank: [dropdown] | Song [slider]
 Tools: [▢][✏]         | Snap: [dropdown] | Effects [slider]
                                          | 100%
```

**Controls:**
| Control | Purpose |
|---------|---------|
| Play/Stop | Toggle playback (changes icon to pause while playing) |
| Stop | Stop and rewind to beginning |
| Loop | Toggle loop mode |
| Time Display | Current playhead position (MM:SS:ms) |
| Bank Dropdown | Default bank for auto-hitnormal (None/Normal/Soft/Drum) |
| Snap Dropdown | Grid divisor (1/1, 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16) |
| Song Slider | Master track volume (default 100%) |
| Effects Slider | Hitsound volume (default 60%) |
| Zoom Display | Current zoom percentage (gray text, bottom-right) |

### 6.5 Dialogs

#### AddTrackDialog
Fields: Name, Bank (dropdown), Type (dropdown), Volume (slider)
- Edit mode: Pre-populates fields, changes button to "Save"

#### AddGroupingDialog  
Fields: Name, HitNormal Bank, Additions Bank, Whistle/Finish/Clap checkboxes, Volume
- Creates composite tracks with multiple sample layers

#### ProjectSetupDialog
- Shown when opening a folder
- Lists all .osu files
- Options: "Open Existing" or "Start From Scratch"

#### PresetDialog / CreatePresetDialog
- Placeholder dialogs for preset management

#### ValidationErrorsDialog
- Shows validation errors before save
- Option to ignore and save anyway

---

## 7. Command System (model/Command.h, Commands.h)

Implements undo/redo using the Command pattern.

### 7.1 UndoManager

```cpp
class UndoManager {
    void PushCommand(std::unique_ptr<Command> cmd);  // Execute and add to history
    void Undo();                                      // Revert last command
    void Redo();                                      // Re-apply undone command
    bool CanUndo() / CanRedo();
    std::string GetUndoDescription() / GetRedoDescription();
    
    // Max history: 100 commands
};
```

### 7.2 Command Implementations

| Command | Description |
|---------|-------------|
| `AddEventCommand` | Single event placement |
| `AddMultipleEventsCommand` | Multiple events (e.g., auto-hitnormal) |
| `RemoveEventsCommand` | Delete selected events |
| `MoveEventsCommand` | Drag-move events (time and/or track) |
| `PasteEventsCommand` | Paste clipboard events |

**All commands use unique event IDs for reliable matching during undo/redo.**

---

## 8. Validation System (model/ProjectValidator.h)

Validates hitsound configurations per timestamp.

**Rules:**
1. **Max 1 HitNormal per timestamp** - Multiple normals = Invalid (red)
2. **Additions must share same bank** - Mixed banks = Invalid (red)
3. **Additions should be backed by HitNormal** - No normal = Warning (pink)

**Implementation:**
```cpp
static std::vector<ValidationError> Validate(Project& project);
```
- Groups events by millisecond timestamp
- Checks all three rules
- Updates `event.validationState` for each event

**Called on:**
- Project load
- Event add/remove/move
- Before save

---

## 9. Presets

### 9.1 Generic Preset

Loaded via Track → Load Preset → Generic.

**Tracks Created:**
| Name | Bank | Type |
|------|------|------|
| Kick | Normal | HitNormal |
| Hi-Hat | Drum | HitNormal |
| Default | Soft | HitNormal |
| Whistle | Soft | HitWhistle |
| Clap | Soft | HitClap |
| Crash | Soft | HitFinish |

**Grouping Created:**
| Name | Layers |
|------|--------|
| Snare | soft-hitnormal + soft-hitclap |

**Default Volume:** 60% for all tracks

---

## 10. osu! File Format Reference

See `osu_documentation.md` for the complete official specification.

### 10.1 Key Format Details

**Sample Set Values:**
```
0 = Auto (use timing point or project default)
1 = Normal
2 = Soft
3 = Drum
```

**HitSound Bitmask:**
```
0 = Normal (default, always plays)
2 = Whistle
4 = Finish
8 = Clap
```

**HitSample Format:**
```
normalSet:additionSet:index:volume:filename
```

**HitObject Format:**
```
x,y,time,type,hitSound,hitSample
256,192,12000,1,2,1:2:0:80:
         │    │ │ │ │ │  └─ volume
         │    │ │ │ │ └─ sample index
         │    │ │ │ └─ addition set (2=soft)
         │    │ │ └─ normal set (1=normal)
         │    │ └─ hitsound bitmask (2=whistle)
         │    └─ type (1=circle)
         └─ timestamp in milliseconds
```

### 10.2 Resolution Priority

When determining which sample to play:
1. HitObject's explicit sample set
2. Timing point's sample set
3. Project's default sample set (from [General])

---

## 11. Known Coordinate Systems

### 11.1 Timeline Coordinates

```cpp
int timeToX(double time) const {
    return static_cast<int>(time * pixelsPerSecond);
}

double xToTime(int x) const {
    return static_cast<double>(x) / pixelsPerSecond;
}
```

**Defaults:**
```cpp
pixelsPerSecond = 100.0;  // 100% zoom
MinPixelsPerSecond = 10.0;
MaxPixelsPerSecond = 5000.0;
```

### 11.2 Track Y Positions

```cpp
y = headerHeight;  // 130 = 30 (ruler) + 100 (master)
for each visible track:
    height = isChildTrack ? 40 : 80;
    y += height;
```

---

## 12. Technical Constraints & Conventions

### 12.1 String Encoding
- wxWidgets: `wxString` (use `FromUTF8()` / `ToStdString()` for conversion)
- JUCE: `juce::String`
- Model: `std::string`

### 12.2 Memory Management
- **Audio**: Raw pointers for JUCE readers (managed by SampleRegistry)
- **UI**: wxWidgets handles window lifetime
- **Commands**: `std::unique_ptr<Command>` in UndoManager
- **Tracks**: Value semantics (copied into Project::tracks vector)

### 12.3 Thread Safety
- UI thread: All wxWidgets operations
- Audio thread: `getNextAudioBlock()` runs here
- **SpinLock** protects `tracksSnapshot` in EventPlaybackSource
- **NEVER** access UI tracks from audio thread directly

### 12.4 Time Units
- **Internal**: Seconds (double)
- **osu! files**: Milliseconds (integer)
- **Always convert**: `ms / 1000.0` on parse, `time * 1000.0` on save

---

## 13. Building the Project

```powershell
cd hitsound-daw-wx
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Dependencies are fetched automatically via CMake `FetchContent`:
- wxWidgets from GitHub master
- JUCE 8.0.0

**Output:** `hitsound-daw-wx.exe` in build directory

---

## 14. Important Notes for AI Agents

> [!IMPORTANT]
> **Do not attempt to build the project upon completing a task.** The user will build and test on their own accord.

> [!TIP]
> When you complete a task, update this GEMINI.md file if you've made significant architectural changes or added new features that would be important for context.

> [!NOTE]
> **Have zero regard for context window or token usage when reading files.** Understanding as much as possible about the project is the highest priority. Reading entire files is preferred over partial reads.

### 15.1 Common File Locations

| Purpose | File |
|---------|------|
| Add new command | `model/Commands.h` + `model/Commands.cpp` |
| Add new UI control | `ui/TransportPanel.cpp` |
| Add track property | `model/Track.h` |
| Modify parsing | `io/OsuParser.cpp` |
| Modify export | `io/ProjectSaver.cpp` |
| Add validation rule | `model/ProjectValidator.h` + `model/ProjectValidator.cpp` |
| Add constants | `Constants.h` |

### 15.2 When Adding Features

1. **New Data Field**: Add to Track.h or Project.h
2. **UI for Field**: Update TrackList.cpp (display) and appropriate dialog
3. **Persistence**: Update OsuParser.cpp (load) and ProjectSaver.cpp (save)
4. **Undo Support**: Create Command in Commands.h if interactive
5. **Audio Impact**: Call `audioEngine.NotifyTracksChanged()` after modifications

### 15.3 Common Patterns

**Adding an event with undo:**
```cpp
auto refreshFn = [this]() { ValidateHitsounds(); Refresh(); };
undoManager.PushCommand(std::make_unique<AddEventCommand>(track, event, refreshFn));
```

**Thread-safe track update:**
```cpp
// After modifying tracks in UI
audioEngine.NotifyTracksChanged();
```

**Snapping to grid:**
```cpp
double snappedTime = timelineView->SnapToGrid(rawTime);
```

---

*Last Updated: December 2024*
