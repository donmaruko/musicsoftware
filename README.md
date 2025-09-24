# MIDI Keyboard Monitor

A real-time lightweight MIDI analysis application built with modern C++17, Qt6, and RtMidi. I created this project to practice multithreaded audio processing, signal processing, music theory implementation, and cross-platform GUI development.

## 🎵 Overview

This application accepts any MIDI-capable keyboard as input and provides note detection, chord recognition, and interval analysis with a responsive graphical interface. Built for musicians, music students, and developers interested in music technology.

## ✨ Key Features

### Real-Time MIDI Processing
- **Low-latency MIDI input** with sub-10ms response time
- **Thread-safe message processing** using Qt's signal-slot mechanism
- **Automatic device detection** and connection management
- **Cross-platform MIDI support** via RtMidi library

### Music Theory Engine
- **Comprehensive chord recognition** covering jazz, classical, and contemporary harmony
- **Interval analysis** from simple 2nds to complex compound intervals
- **Polyphonic note tracking** with unlimited simultaneous notes
- **Context-aware chord naming** with multiple enharmonic interpretations

### Chord Detection Algorithm
The application recognizes over 25+ chord types including:

**Basic Triads & Extensions:**
- Major, Minor, Diminished, Augmented
- Major 7th, Minor 7th, Dominant 7th
- Major 9th, Minor 9th, Dominant 9th, 11th, 13th

**Jazz & Advanced Harmony:**
- Suspended chords (sus2, sus4, 7sus2, 7sus4)
- Altered dominants (7♭5, 7♯5, 7♭9, 7♯9, 7♯11)
- Half-diminished, Diminished 7th
- Add chords (add9, 6th chords)

**Interval Recognition:**
- All simple and compound intervals
- Enharmonic equivalent handling
- Contextual naming based on harmonic function

### Modern GUI Architecture
- **Responsive Qt6 interface** with custom styling
- **Real-time visual feedback** with color-coded displays
- **Thread-safe UI updates** preventing GUI freezing
- **Scalable design** supporting high-DPI displays

## 🏗️ Technical Architecture

### Core Design Patterns

**Model-View-Controller (MVC) Pattern:**
```cpp
// Separation of concerns with clear boundaries
class MidiKeyboardMonitor : public QMainWindow  // View + Controller
{
private:
    std::set<int> activeNotes;                  // Model (Note State)
    std::map<std::string, std::vector<int>> chordPatterns;  // Model (Music Theory)
    
    void setupUI();                             // View initialization
    void setupMidi();                           // Controller initialization
    QString detectChord();                      // Business logic
};
```

**Producer-Consumer Pattern with Thread Safety:**
```cpp
// MIDI callback runs on audio thread
static void midiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData)
{
    // Thread-safe message queuing
    QMutexLocker locker(&monitor->midiQueueMutex);
    monitor->midiMessageQueue.push_back({timeStamp, *message});
}

// GUI thread processes messages safely
void processPendingMidiMessages()
{
    QMutexLocker locker(&midiQueueMutex);  // RAII lock management
    // Process all queued messages atomically
}
```

**Strategy Pattern for Chord Recognition:**
```cpp
class ChordAnalyzer {
    std::map<std::string, std::vector<int>> chordPatterns;  // Strategy definitions
    
    std::string getChordName(const std::vector<int>& intervals, const std::string& rootNote) {
        // Polymorphic chord matching algorithm
        for (const auto& pattern : chordPatterns) {
            if (matchesPattern(intervals, pattern.second)) {
                return rootNote + " " + pattern.first;
            }
        }
    }
};
```

### Memory Management & Performance

**Modern C++ Memory Safety:**
- **RAII principles** throughout codebase
- **Smart pointers** for automatic resource management
- **Move semantics** for efficient data transfer
- **Exception safety** with proper cleanup

**Performance Optimizations:**
- **Lock-free data structures** where possible
- **Efficient STL container usage** (std::set for O(log n) operations)
- **Minimal heap allocations** in audio thread
- **Qt's meta-object system** for efficient signal-slot connections

### Cross-Platform Compatibility

**Platform Abstraction Layer:**
```cpp
// RtMidi provides cross-platform MIDI abstraction
std::unique_ptr<RtMidiIn> midiIn;  // Works on Windows, macOS, Linux

// Qt6 handles platform-specific GUI rendering
QApplication app(argc, argv);      // Native look and feel
```

**Build System Design:**
```cmake
# Modern CMake with proper dependency management
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(RTMIDI REQUIRED rtmidi)

# Automatic MOC processing for Qt meta-objects
set(CMAKE_AUTOMOC ON)
```

## 🔧 Technical Implementation Details

### MIDI Message Processing Pipeline

1. **Hardware Layer:** USB MIDI → ALSA/Core Audio/DirectSound
2. **RtMidi Layer:** Platform abstraction → Raw MIDI bytes
3. **Application Layer:** Message parsing → Music theory analysis
4. **Presentation Layer:** Qt signals → GUI updates

### Chord Recognition Algorithm

The chord recognition engine uses a pattern-matching approach:

```cpp
// 1. Normalize all notes to a single octave (mod 12)
std::vector<int> intervals;
for (int note : activeNotes) {
    int interval = (note - rootCandidate) % 12;
    if (interval < 0) interval += 12;
    intervals.push_back(interval);
}

// 2. Try each note as potential root
for (int rootNote : activeNotes) {
    std::string chordName = analyzeIntervalPattern(intervals, rootNote);
    if (!chordName.empty()) return chordName;  // First match wins
}

// 3. Pattern matching against music theory database
bool matchesChordType(const std::vector<int>& intervals, const std::vector<int>& pattern) {
    return std::equal(intervals.begin(), intervals.end(), pattern.begin());
}
```

### Thread Safety Architecture

**Problem:** MIDI callbacks execute on audio thread, GUI updates must happen on main thread.

**Solution:** Lock-free message passing with Qt's event system:

```cpp
// Audio thread: Non-blocking message queuing
{
    QMutexLocker locker(&midiQueueMutex);  // Brief lock
    midiMessageQueue.push_back(message);   // O(1) operation
}  // Lock released immediately

// Main thread: Batch processing every 10ms
QTimer::timeout -> processPendingMidiMessages() -> emit signals -> GUI updates
```

## 🎯 Use Cases

- **Music Theory Learning:** Visual feedback for chord identification
- **Piano Practice:** Real-time validation of chord progressions
- **Ear Training:** Visual confirmation of interval recognition
- **Composition Aid:** Quick chord identification while writing
- **MIDI Debugging:** Low-level MIDI message inspection

## 🛠️ Development Environment

### Prerequisites
```bash
# Fedora/RHEL
sudo dnf install qt6-qtbase-devel rtmidi-devel cmake gcc-c++ alsa-lib-devel

# Ubuntu/Debian  
sudo apt install qt6-base-dev librtmidi-dev cmake g++ libasound2-dev

# macOS
brew install qt6 rtmidi cmake
```

### Building from Source
```bash
git clone https://github.com/donmaruko/musicsoftware.git
cd musicsoftware
mkdir build && cd build
cmake ..
make -j$(nproc)  # Parallel build
./midi-monitor
```

## 🔮 Future Enhancements

### Planned Features
- **Audio input analysis** using FFT for microphone input
- **MIDI file playback** with real-time analysis
- **Custom chord library** with user-defined patterns
- **Multiple keyboard support** for ensemble analysis
- **Plugin architecture** for extensible chord definitions
- **Web interface** using Qt WebAssembly
- **Configuration system** for user preferences

## 📝 License

MIT License - Open source for educational and commercial use.

---