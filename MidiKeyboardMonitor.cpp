#include "MidiKeyboardMonitor.h"
#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <iostream>
#include <iomanip>
#include <algorithm>

MidiKeyboardMonitor::MidiKeyboardMonitor(QWidget *parent)
    : QMainWindow(parent)
    , centralWidget(nullptr)
    , mainLayout(nullptr)
    , controlsLayout(nullptr)
    , keySelectionGroup(nullptr)
    , keySignatureCombo(nullptr)
    , deviceLabel(nullptr)
    , noteLabel(nullptr)
    , chordLabel(nullptr)
    , clearTimer(new QTimer(this))
    , midiProcessTimer(new QTimer(this))
    , midiIn(nullptr)
    , currentKeyIndex(0)  // Default to C Major
{
    setupKeySignatures();
    setupUI();
    setupChordPatterns();
    setupMidi();
    
    // Connect timers and controls
    connect(clearTimer, &QTimer::timeout, this, &MidiKeyboardMonitor::clearDisplay);
    connect(midiProcessTimer, &QTimer::timeout, this, &MidiKeyboardMonitor::processPendingMidiMessages);
    connect(keySignatureCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MidiKeyboardMonitor::onKeySignatureChanged);
    
    clearTimer->setSingleShot(true);
    midiProcessTimer->start(10);
}

MidiKeyboardMonitor::~MidiKeyboardMonitor()
{
    if (midiIn && midiIn->isPortOpen()) {
        midiIn->closePort();
    }
}

void MidiKeyboardMonitor::setupKeySignatures()
{
    // Major keys (following circle of fifths)
    keySignatures = {
        // C Major (no accidentals)
        {"C Major", {}, {}, 0, true},
        
        // Sharp keys
        {"G Major", {6}, {}, 7, true},        // F#
        {"D Major", {6, 1}, {}, 2, true},     // F#, C#
        {"A Major", {6, 1, 8}, {}, 9, true},  // F#, C#, G#
        {"E Major", {6, 1, 8, 3}, {}, 4, true}, // F#, C#, G#, D#
        {"B Major", {6, 1, 8, 3, 10}, {}, 11, true}, // F#, C#, G#, D#, A#
        {"F# Major", {6, 1, 8, 3, 10, 5}, {}, 6, true}, // F#, C#, G#, D#, A#, E#
        {"C# Major", {0, 6, 1, 8, 3, 10, 5}, {}, 1, true}, // All sharps
        
        // Flat keys
        {"F Major", {}, {10}, 5, true},       // Bb
        {"Bb Major", {}, {10, 3}, 10, true},  // Bb, Eb
        {"Eb Major", {}, {10, 3, 8}, 3, true}, // Bb, Eb, Ab
        {"Ab Major", {}, {10, 3, 8, 1}, 8, true}, // Bb, Eb, Ab, Db
        {"Db Major", {}, {10, 3, 8, 1, 6}, 1, true}, // Bb, Eb, Ab, Db, Gb
        {"Gb Major", {}, {10, 3, 8, 1, 6, 11}, 6, true}, // Bb, Eb, Ab, Db, Gb, Cb
        {"Cb Major", {}, {0, 10, 3, 8, 1, 6, 11}, 11, true}, // All flats
        
        // Natural minor keys
        {"A minor", {}, {}, 9, false},
        {"E minor", {6}, {}, 4, false},       // F#
        {"B minor", {6, 1}, {}, 11, false},   // F#, C#
        {"F# minor", {6, 1, 8}, {}, 6, false}, // F#, C#, G#
        {"C# minor", {6, 1, 8, 3}, {}, 1, false}, // F#, C#, G#, D#
        {"G# minor", {6, 1, 8, 3, 10}, {}, 8, false}, // F#, C#, G#, D#, A#
        {"D# minor", {6, 1, 8, 3, 10, 5}, {}, 3, false}, // F#, C#, G#, D#, A#, E#
        {"A# minor", {0, 6, 1, 8, 3, 10, 5}, {}, 10, false}, // All sharps
        
        {"D minor", {}, {10}, 2, false},      // Bb
        {"G minor", {}, {10, 3}, 7, false},   // Bb, Eb
        {"C minor", {}, {10, 3, 8}, 0, false}, // Bb, Eb, Ab
        {"F minor", {}, {10, 3, 8, 1}, 5, false}, // Bb, Eb, Ab, Db
        {"Bb minor", {}, {10, 3, 8, 1, 6}, 10, false}, // Bb, Eb, Ab, Db, Gb
        {"Eb minor", {}, {10, 3, 8, 1, 6, 11}, 3, false}, // Bb, Eb, Ab, Db, Gb, Cb
        {"Ab minor", {}, {0, 10, 3, 8, 1, 6, 11}, 8, false}, // All flats
    };
}

void MidiKeyboardMonitor::setupUI()
{
    setWindowTitle("MIDI Keyboard Monitor");
    setMinimumSize(600, 500);
    resize(800, 600);
    
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    
    // Key signature selection controls
    keySelectionGroup = new QGroupBox("Key Signature", this);
    keySelectionGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; font-size: 14px; padding: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; }"
    );
    
    controlsLayout = new QHBoxLayout(keySelectionGroup);
    
    keySignatureCombo = new QComboBox(this);
    keySignatureCombo->setStyleSheet(
        "QComboBox { font-size: 13px; padding: 5px; min-width: 150px; }"
    );
    
    for (const auto& key : keySignatures) {
        keySignatureCombo->addItem(QString::fromStdString(key.name));
    }
    keySignatureCombo->setCurrentIndex(0); // Default to C Major
    
    controlsLayout->addWidget(keySignatureCombo);
    controlsLayout->addStretch();
    
    mainLayout->addWidget(keySelectionGroup);
    
    // Device connection label
    deviceLabel = new QLabel("No MIDI device connected", this);
    deviceLabel->setAlignment(Qt::AlignCenter);
    deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; margin-bottom: 10px; }");
    mainLayout->addWidget(deviceLabel);
    
    // Add spacer to center the note/chord display
    mainLayout->addStretch(1);
    
    // Note display label
    noteLabel = new QLabel("Press keys...", this);
    noteLabel->setAlignment(Qt::AlignCenter);
    
    QFont noteFont;
    noteFont.setPointSize(36);
    noteFont.setBold(true);
    noteLabel->setFont(noteFont);
    noteLabel->setStyleSheet("QLabel { color: #2E8B57; margin: 15px; }");
    
    mainLayout->addWidget(noteLabel);
    
    // Chord display label
    chordLabel = new QLabel("", this);
    chordLabel->setAlignment(Qt::AlignCenter);
    
    QFont chordFont;
    chordFont.setPointSize(28);
    chordFont.setBold(true);
    chordLabel->setFont(chordFont);
    chordLabel->setStyleSheet("QLabel { color: #FF6347; margin: 15px; }");
    
    mainLayout->addWidget(chordLabel);
    
    // Add bottom spacer
    mainLayout->addStretch(1);
    
    setStyleSheet("QMainWindow { background-color: white; }");
}

void MidiKeyboardMonitor::onKeySignatureChanged()
{
    currentKeyIndex = keySignatureCombo->currentIndex();
    std::cout << "Key signature changed to: " << keySignatures[currentKeyIndex].name << std::endl;
    
    // Update display with current notes using new key signature
    if (!activeNotes.empty()) {
        QString notesList;
        for (int note : activeNotes) {
            if (!notesList.isEmpty()) notesList += " + ";
            notesList += midiNoteToNoteNameInKey(note, keySignatures[currentKeyIndex]);
        }
        noteLabel->setText(notesList);
        
        // Re-analyze chord with new key context
        QString chord = detectChord();
        chordLabel->setText(chord);
    }
}

void MidiKeyboardMonitor::setupChordPatterns()
{
    // Major chords and extensions
    chordPatterns["maj"] = {0, 4, 7};
    chordPatterns["maj7"] = {0, 4, 7, 11};
    chordPatterns["maj9"] = {0, 4, 7, 11, 14};
    chordPatterns["6"] = {0, 4, 7, 9};
    chordPatterns["add9"] = {0, 4, 7, 14};
    
    // Minor chords and extensions
    chordPatterns["m"] = {0, 3, 7};
    chordPatterns["m7"] = {0, 3, 7, 10};
    chordPatterns["m9"] = {0, 3, 7, 10, 14};
    chordPatterns["m6"] = {0, 3, 7, 9};
    chordPatterns["mMaj7"] = {0, 3, 7, 11};
    
    // Dominant chords
    chordPatterns["7"] = {0, 4, 7, 10};
    chordPatterns["9"] = {0, 4, 7, 10, 14};
    chordPatterns["11"] = {0, 4, 7, 10, 14, 17};
    chordPatterns["13"] = {0, 4, 7, 10, 14, 17, 21};
    
    // Diminished chords
    chordPatterns["dim"] = {0, 3, 6};
    chordPatterns["dim7"] = {0, 3, 6, 9};
    chordPatterns["ø7"] = {0, 3, 6, 10}; // Half-diminished
    
    // Augmented chords
    chordPatterns["aug"] = {0, 4, 8};
    chordPatterns["aug7"] = {0, 4, 8, 10};
    
    // Suspended chords
    chordPatterns["sus2"] = {0, 2, 7};
    chordPatterns["sus4"] = {0, 5, 7};
    chordPatterns["7sus2"] = {0, 2, 7, 10};
    chordPatterns["7sus4"] = {0, 5, 7, 10};
    
    // Altered dominants
    chordPatterns["7♭5"] = {0, 4, 6, 10};
    chordPatterns["7♯5"] = {0, 4, 8, 10};
    chordPatterns["7♭9"] = {0, 4, 7, 10, 13};
    chordPatterns["7♯9"] = {0, 4, 7, 10, 15};
    chordPatterns["7♯11"] = {0, 4, 7, 10, 18};
}

void MidiKeyboardMonitor::setupMidi()
{
    try {
        midiIn = std::make_unique<RtMidiIn>();
        
        unsigned int nPorts = midiIn->getPortCount();
        std::cout << "Available MIDI input ports: " << nPorts << std::endl;
        
        if (nPorts == 0) {
            deviceLabel->setText("No MIDI input devices found!");
            deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 10px; }");
            return;
        }
        
        for (unsigned int i = 0; i < nPorts; i++) {
            std::string portName = midiIn->getPortName(i);
            std::cout << "Port " << i << ": " << portName << std::endl;
        }
        
        connectToFirstMidiDevice();
        
    } catch (RtMidiError &error) {
        std::cerr << "RtMidi error: " << error.getMessage() << std::endl;
        deviceLabel->setText("MIDI Error: " + QString::fromStdString(error.getMessage()));
        deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 10px; }");
    }
}

void MidiKeyboardMonitor::connectToFirstMidiDevice()
{
    try {
        if (midiIn->getPortCount() > 0) {
            int targetPort = 0;
            for (unsigned int i = 0; i < midiIn->getPortCount(); i++) {
                std::string portName = midiIn->getPortName(i);
                if (portName.find("Recital Play") != std::string::npos) {
                    targetPort = i;
                    break;
                }
            }
            
            std::string portName = midiIn->getPortName(targetPort);
            midiIn->openPort(targetPort);
            
            midiIn->setCallback(&MidiKeyboardMonitor::midiCallback, this);
            midiIn->ignoreTypes(false, false, false);
            
            QString deviceText = QString("Connected to: %1 (Port %2)")
                                .arg(QString::fromStdString(portName))
                                .arg(targetPort);
            deviceLabel->setText(deviceText);
            deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: green; margin-bottom: 10px; }");
            
            std::cout << "Successfully connected to: " << portName << " (Port " << targetPort << ")" << std::endl;
        }
    } catch (RtMidiError &error) {
        std::cerr << "Error connecting to MIDI device: " << error.getMessage() << std::endl;
        deviceLabel->setText("Connection Error: " + QString::fromStdString(error.getMessage()));
        deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 10px; }");
    }
}

void MidiKeyboardMonitor::midiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData)
{
    MidiKeyboardMonitor *monitor = static_cast<MidiKeyboardMonitor*>(userData);
    
    QMutexLocker locker(&monitor->midiQueueMutex);
    monitor->midiMessageQueue.push_back({timeStamp, *message});
}

void MidiKeyboardMonitor::processPendingMidiMessages()
{
    std::vector<MidiMessage> messages;
    
    {
        QMutexLocker locker(&midiQueueMutex);
        messages = midiMessageQueue;
        midiMessageQueue.clear();
    }
    
    for (const auto& msg : messages) {
        if (msg.data.size() == 0) continue;
        
        std::cout << "MIDI message: ";
        for (size_t i = 0; i < msg.data.size(); i++) {
            std::cout << std::hex << (int)msg.data[i] << " ";
        }
        std::cout << std::dec << std::endl;
        
        if (msg.data.size() >= 3) {
            unsigned char status = msg.data[0];
            unsigned char noteNumber = msg.data[1];
            unsigned char velocity = msg.data[2];
            
            bool isNoteOn = ((status & 0xF0) == 0x90) && (velocity > 0);
            bool isNoteOff = ((status & 0xF0) == 0x80) || (((status & 0xF0) == 0x90) && (velocity == 0));
            
            const KeySignature& currentKey = keySignatures[currentKeyIndex];
            
            if (isNoteOn) {
                activeNotes.insert(noteNumber);
                std::cout << "Note ON: " << midiNoteToNoteNameInKey(noteNumber, currentKey).toStdString() 
                         << " (MIDI: " << (int)noteNumber << ", Velocity: " << (int)velocity << ")" << std::endl;
            }
            else if (isNoteOff) {
                activeNotes.erase(noteNumber);
                std::cout << "Note OFF: " << midiNoteToNoteNameInKey(noteNumber, currentKey).toStdString() 
                         << " (MIDI: " << (int)noteNumber << ")" << std::endl;
            }
            
            // Update display using key-aware note names
            if (!activeNotes.empty()) {
                QString notesList;
                for (int note : activeNotes) {
                    if (!notesList.isEmpty()) notesList += " + ";
                    notesList += midiNoteToNoteNameInKey(note, currentKey);
                }
                noteLabel->setText(notesList);
                
                QString chord = detectChord();
                chordLabel->setText(chord);
                
                clearTimer->stop();
            } else {
                clearTimer->start(100);
            }
        }
    }
}

void MidiKeyboardMonitor::clearDisplay()
{
    noteLabel->setText("Press keys...");
    chordLabel->setText("");
}

QString MidiKeyboardMonitor::midiNoteToNoteNameInKey(int midiNote, const KeySignature& key)
{
    const QString sharpNames[] = {"C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"};
    const QString flatNames[] = {"C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B"};
    
    int noteClass = midiNote % 12;
    int octave = (midiNote / 12) - 1;
    
    QString noteName;
    
    // Check if this note should be sharp in the current key
    for (int sharpNote : key.sharps) {
        if (noteClass == sharpNote) {
            noteName = sharpNames[noteClass];
            break;
        }
    }
    
    // Check if this note should be flat in the current key  
    if (noteName.isEmpty()) {
        for (int flatNote : key.flats) {
            if (noteClass == flatNote) {
                noteName = flatNames[noteClass];
                break;
            }
        }
    }
    
    // If not specified by key signature, use default (prefer sharps for now)
    if (noteName.isEmpty()) {
        noteName = sharpNames[noteClass];
    }
    
    return noteName + QString::number(octave);
}

QString MidiKeyboardMonitor::midiNoteToNoteName(int midiNote)
{
    const QString noteNames[] = {"C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"};
    
    int noteIndex = midiNote % 12;
    int octave = (midiNote / 12) - 1;
    
    return noteNames[noteIndex] + QString::number(octave);
}

QString MidiKeyboardMonitor::detectChord()
{
    if (activeNotes.size() < 2) return "";
    
    const KeySignature& currentKey = keySignatures[currentKeyIndex];
    std::vector<int> notes(activeNotes.begin(), activeNotes.end());
    std::sort(notes.begin(), notes.end());
    
    // Handle simple intervals (2 notes)
    if (notes.size() == 2) {
        int interval = notes[1] - notes[0];
        QString rootNote = midiNoteToNoteNameInKey(notes[0], currentKey);
        
        switch (interval) {
            case 1: return rootNote + " minor 2nd";
            case 2: return rootNote + " major 2nd";
            case 3: return rootNote + " minor 3rd";
            case 4: return rootNote + " major 3rd";
            case 5: return rootNote + " perfect 4th";
            case 6: return rootNote + " tritone";
            case 7: return rootNote + " perfect 5th";
            case 8: return rootNote + " minor 6th";
            case 9: return rootNote + " major 6th";
            case 10: return rootNote + " minor 7th";
            case 11: return rootNote + " major 7th";
            case 12: return rootNote + " octave";
            default: return rootNote + " +" + QString::number(interval) + " semitones";
        }
    }
    
    // For chords (3+ notes), try all possible root notes
    for (int rootNote : notes) {
        std::vector<int> intervals;
        for (int note : notes) {
            int interval = (note - rootNote) % 12;
            if (interval < 0) interval += 12;
            intervals.push_back(interval);
        }
        std::sort(intervals.begin(), intervals.end());
        
        QString rootNoteName = midiNoteToNoteNameInKey(rootNote, currentKey);
        std::string chordName = getChordName(intervals, rootNoteName.toStdString());
        
        if (!chordName.empty()) {
            return QString::fromStdString(chordName);
        }
    }
    
    return QString("Cluster (%1 notes)").arg(activeNotes.size());
}

std::string MidiKeyboardMonitor::getChordName(const std::vector<int>& intervals, const std::string& rootNote)
{
    for (const auto& pattern : chordPatterns) {
        std::vector<int> chordIntervals = pattern.second;
        std::sort(chordIntervals.begin(), chordIntervals.end());
        
        if (intervals.size() == chordIntervals.size()) {
            bool match = true;
            for (size_t i = 0; i < intervals.size(); i++) {
                if (intervals[i] != chordIntervals[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return rootNote + " " + pattern.first;
            }
        }
    }
    
    return "";
}