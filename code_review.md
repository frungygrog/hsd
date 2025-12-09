# Hitsound DAW - Code Review Notes

> **Status**: Ongoing review of architectural concerns and potential improvements

---

## Architecture Concerns

### 1. No separation between View and Controller logic
- `TimelineView` directly manipulates `Project` data AND handles all input
- A thin controller layer would make testing easier and improve separation of concerns

### 2. Validation is duplicated
- `ProjectValidator::Validate()` exists in `ProjectValidator.h`
- `TimelineView::ValidateHitsounds()` does similar logic inline
- These should be unified into a single source of truth

### 3. Track hierarchy uses raw pointers everywhere
- `Track*` pointers become invalid when vectors resize (e.g., adding tracks)
- The clipboard, selection, and drag systems all hold `Track*` which is fragile
- Consider using stable IDs or `std::list` for tracks

---

## Minor Issues

### 4. Magic numbers scattered throughout
- While `Constants.h` exists, there are still hardcoded values like `40`, `80`, `130` in multiple files
- Some files use constants, others use literals for the same values

### 5. Inconsistent callback naming
- `OnLoopPointsChanged` vs `OnZoomChanged` vs `OnTracksModified`
- Some are null-checked before calling, some aren't

### 6. No error handling in audio loading
- `LoadMasterTrack()` silently fails if file doesn't exist
- `SampleRegistry::getReader()` returns nullptr with no logging

### 7. Comments reference outdated plans
- `Track.h` has a "Seconds or Milliseconds?" comment that's been resolved but not cleaned up
- `Commands.h` has "Assume it's at the end" comments that are now irrelevant since IDs were added

---

## Missing Features (Potential Tech Debt)

### 8. Solo logic isn't implemented
- `Track::solo` exists but `EventPlaybackSource` doesn't check it
- Mute works, solo does not

### 9. No project dirty state tracking
- `UndoManager::EnsureCleanState()` is empty
- No "unsaved changes" warning on close

### 10. Preset system is incomplete
- `CreatePresetDialog` is a placeholder that does nothing
- Only "Generic" preset exists, hardcoded in `MainFrame::ApplyPreset()`

---

## Priority Matrix

| Priority | Issue | Reason |
|----------|-------|--------|
| 🔴 High | Track pointer stability (#3) | Can cause crashes when modifying tracks |
| 🟠 Medium | Unify validation (#2) | Code duplication, potential for inconsistency |
| 🟠 Medium | Implement solo (#8) | Feature exists in UI but doesn't work |
| 🟡 Low | Magic numbers (#4) | Maintenance burden |
| 🟡 Low | Dirty state tracking (#9) | UX improvement |
| 🟡 Low | Complete preset system (#10) | Feature incomplete |

---

## Resolved Issues

- [x] Header-only implementations split into `.h/.cpp` pairs
- [x] `TrackList.cpp` refactored/split
- [x] `TimelineView.cpp` refactored/split
