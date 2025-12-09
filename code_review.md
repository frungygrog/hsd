# Hitsound DAW - Code Review Notes

> **Status**: Ongoing review of architectural concerns and potential improvements

---

## Architecture Concerns

### ~~1. No separation between View and Controller logic~~ ✅ RESOLVED
- Created `TimelineController` class to handle business logic
- `TimelineView` now delegates Undo/Redo, selection focus, and UndoManager access to controller
- Improved separation of concerns for testability and maintainability

### ~~2. Validation is duplicated~~ ✅ RESOLVED
- `TimelineView::ValidateHitsounds()` now delegates to `ProjectValidator::Validate()`
- Single source of truth for validation logic

### ~~3. Track hierarchy uses raw pointers everywhere~~ ✅ RESOLVED
- Selection, clipboard, and drag systems now use stable `{trackId, eventId}` pairs
- `Track.h` now has `uint64_t id` field with global atomic counter
- `TimelineView::FindTrackById()` resolves IDs to current Track references

---

## Minor Issues

### ~~4. Magic numbers scattered throughout~~ ✅ RESOLVED
- TrackList.cpp, TimelineView.cpp, and MainFrame.cpp now use `TrackLayout::` constants from `Constants.h`
- Local anonymous namespace constants now alias the centralized definitions

### ~~5. Inconsistent callback naming~~ ✅ RESOLVED
- All callbacks (`OnLoopPointsChanged`, `OnZoomChanged`, `OnTracksModified`, `OnPlayheadScrubbed`) now consistently null-checked before calling
- Naming convention is consistent with `On` prefix

### ~~6. No error handling in audio loading~~ ✅ RESOLVED
- `AudioEngine::LoadMasterTrack()` now logs file not found and reader creation failures
- `SampleRegistry::addSample()` and `loadDefaultSamples()` now log missing files and load failures
- Uses JUCE's `DBG()` for debug output

### ~~7. Comments reference outdated plans~~ ✅ RESOLVED
- Cleaned up "Seconds or Milliseconds?" comment in `Track.h`

---

## Missing Features (Potential Tech Debt)

### ~~8. Solo logic isn't implemented~~ ✅ RESOLVED
- `EventPlaybackSource::getNextAudioBlock()` now checks for soloed tracks
- When any track has `solo=true`, only soloed tracks play

### ~~9. No project dirty state tracking~~ ✅ RESOLVED
- `UndoManager::MarkClean()` marks current state as saved
- `UndoManager::IsDirty()` checks if there are unsaved changes
- `MainFrame::OnClose()` shows Save/Discard/Cancel dialog when dirty

### ~~10. Preset system is incomplete~~ ✅ RESOLVED
- CreatePresetDialog saves current tracks as .preset files
- PresetDialog dynamically loads both built-in and custom presets
- Presets stored in user data directory (`%APPDATA%/hitsound-daw-wx/Presets/`)

---

## Priority Matrix

| Priority | Issue | Reason |
|----------|-------|--------|
| ✅ ~~High~~ | ~~Track pointer stability (#3)~~ | **RESOLVED** - Uses ID-based selection |
| ✅ ~~Medium~~ | ~~Unify validation (#2)~~ | **RESOLVED** - Already unified |
| ✅ ~~Medium~~ | ~~Implement solo (#8)~~ | **RESOLVED** - Solo now works |
| ✅ ~~Low~~ | ~~Magic numbers (#4)~~ | **RESOLVED** - Uses Constants.h |
| ✅ ~~Low~~ | ~~Dirty state tracking (#9)~~ | **RESOLVED** - Unsaved warning |
| ✅ ~~Low~~ | ~~Complete preset system (#10)~~ | **RESOLVED** - Save/Load presets |
| ✅ ~~Low~~ | ~~Callback naming (#5)~~ | **RESOLVED** - Consistent null-checks |
| ✅ ~~Low~~ | ~~Audio error handling (#6)~~ | **RESOLVED** - DBG logging |
| ✅ ~~Deferred~~ | ~~View/Controller separation (#1)~~ | **RESOLVED** - TimelineController created |

---

## Resolved Issues

- [x] Header-only implementations split into `.h/.cpp` pairs
- [x] `TrackList.cpp` refactored/split
- [x] `TimelineView.cpp` refactored/split
- [x] Track pointer stability (#3) - ID-based selection system
- [x] Validation unified (#2) - confirmed already delegates to ProjectValidator
- [x] Outdated comments (#7) - cleaned up in Track.h
- [x] Solo logic (#8) - implemented in EventPlaybackSource
- [x] Magic numbers (#4) - consolidated to Constants.h
- [x] Dirty state tracking (#9) - unsaved changes warning
- [x] Preset system (#10) - save/load presets
- [x] Callback naming (#5) - consistent null-checking
- [x] Audio error handling (#6) - DBG logging added
