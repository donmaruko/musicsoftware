#include "MidiKeyboardMonitor.h"
#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QRegularExpression>
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
    , deviceCheckTimer(new QTimer(this))
    , midiIn(nullptr)
    , midiConnected(false)
    , currentKeyIndex(0)
{
    setupKeySignatures();
    setupUI();
    setupChordPatterns();
    setupMidi();
    
    // Connect timers and controls
    connect(clearTimer, &QTimer::timeout, this, &MidiKeyboardMonitor::clearDisplay);
    connect(midiProcessTimer, &QTimer::timeout, this, &MidiKeyboardMonitor::processPendingMidiMessages);
    connect(deviceCheckTimer, &QTimer::timeout, this, &MidiKeyboardMonitor::checkForMidiDevices);
    connect(keySignatureCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MidiKeyboardMonitor::onKeySignatureChanged);
    
    clearTimer->setSingleShot(true);
    midiProcessTimer->start(10);
    deviceCheckTimer->start(1000); // Check for devices every second
    
    // Initial device check
    checkForMidiDevices();
}

MidiKeyboardMonitor::~MidiKeyboardMonitor()
{
    disconnectMidi();
}

void MidiKeyboardMonitor::setupKeySignatures()
{
    // Major keys (following circle of fifths)
    keySignatures = {
        {"C Major", {}, {}, 0, true},
        {"G Major", {6}, {}, 7, true},        // F#
        {"D Major", {6, 1}, {}, 2, true},     // F#, C#
        {"A Major", {6, 1, 8}, {}, 9, true},  // F#, C#, G#
        {"E Major", {6, 1, 8, 3}, {}, 4, true}, // F#, C#, G#, D#
        {"B Major", {6, 1, 8, 3, 10}, {}, 11, true}, // F#, C#, G#, D#, A#
        {"F# Major", {6, 1, 8, 3, 10, 5}, {}, 6, true}, // F#, C#, G#, D#, A#, E#
        {"C# Major", {0, 6, 1, 8, 3, 10, 5}, {}, 1, true}, // All sharps
        
        {"F Major", {}, {10}, 5, true},       // Bb
        {"B♭ Major", {}, {10, 3}, 10, true},  // Bb, Eb
        {"E♭ Major", {}, {10, 3, 8}, 3, true}, // Bb, Eb, Ab
        {"A♭ Major", {}, {10, 3, 8, 1}, 8, true}, // Bb, Eb, Ab, Db
        {"D♭ Major", {}, {10, 3, 8, 1, 6}, 1, true}, // Bb, Eb, Ab, Db, Gb
        {"G♭ Major", {}, {10, 3, 8, 1, 6, 11}, 6, true}, // Bb, Eb, Ab, Db, Gb, Cb
        {"C♭ Major", {}, {0, 10, 3, 8, 1, 6, 11}, 11, true}, // All flats
        
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
        {"B♭ minor", {}, {10, 3, 8, 1, 6}, 10, false}, // Bb, Eb, Ab, Db, Gb
        {"E♭ minor", {}, {10, 3, 8, 1, 6, 11}, 3, false}, // Bb, Eb, Ab, Db, Gb, Cb
        {"A♭ minor", {}, {0, 10, 3, 8, 1, 6, 11}, 8, false}, // All flats
    };
}

void MidiKeyboardMonitor::setupUI()
{
    setWindowTitle("MIDI Keyboard Monitor - Key-Aware Analysis");
    setMinimumSize(650, 500);
    resize(850, 600);
    
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    
    // Control panel
    keySelectionGroup = new QGroupBox("Settings", this);
    keySelectionGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; font-size: 14px; padding: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; }"
    );
    
    controlsLayout = new QHBoxLayout(keySelectionGroup);
    
    // Key signature selection
    keySignatureCombo = new QComboBox(this);
    keySignatureCombo->setStyleSheet(
        "QComboBox { font-size: 13px; padding: 5px; min-width: 150px; }"
    );
    
    for (const auto& key : keySignatures) {
        keySignatureCombo->addItem(QString::fromStdString(key.name));
    }
    keySignatureCombo->setCurrentIndex(0);
    
    controlsLayout->addWidget(keySignatureCombo);
    controlsLayout->addStretch();
    
    mainLayout->addWidget(keySelectionGroup);
    
    // Device status label
    deviceLabel = new QLabel("No Controller Found", this);
    deviceLabel->setAlignment(Qt::AlignCenter);
    deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 10px; font-weight: bold; }");
    mainLayout->addWidget(deviceLabel);
    
    mainLayout->addStretch(1);
    
    // Note display label
    noteLabel = new QLabel("Connect MIDI controller...", this);
    noteLabel->setAlignment(Qt::AlignCenter);
    
    QFont noteFont;
    noteFont.setPointSize(36);
    noteFont.setBold(true);
    noteLabel->setFont(noteFont);
    noteLabel->setStyleSheet("QLabel { color: #888; margin: 15px; }");
    
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
    
    mainLayout->addStretch(1);
    
    setStyleSheet("QMainWindow { background-color: white; }");
}

void MidiKeyboardMonitor::onKeySignatureChanged()
{
    currentKeyIndex = keySignatureCombo->currentIndex();
    std::cout << "Key signature changed to: " << keySignatures[currentKeyIndex].name << std::endl;
    
    if (!activeNotes.empty() && midiConnected) {
        QString notesList;
        for (int note : activeNotes) {
            if (!notesList.isEmpty()) notesList += " + ";
            notesList += midiNoteToNoteNameInKey(note, keySignatures[currentKeyIndex]);
        }
        noteLabel->setText(notesList);
        
        QString chord = detectChord();
        chordLabel->setText(chord);
    }
}

void MidiKeyboardMonitor::checkForMidiDevices()
{
    try {
        // Create temporary MIDI input to check available ports
        std::unique_ptr<RtMidiIn> tempMidiIn = std::make_unique<RtMidiIn>();
        unsigned int nPorts = tempMidiIn->getPortCount();
        
        bool deviceFound = false;
        std::string foundDeviceName;
        
        // Check for any MIDI devices
        for (unsigned int i = 0; i < nPorts; i++) {
            std::string portName = tempMidiIn->getPortName(i);
            // Skip "Midi Through" ports as they're not real devices
            if (portName.find("Midi Through") == std::string::npos) {
                deviceFound = true;
                foundDeviceName = portName;
                break;
            }
        }
        
        // Handle connection state changes
        if (deviceFound && !midiConnected) {
            std::cout << "MIDI device detected: " << foundDeviceName << std::endl;
            attemptMidiConnection();
        } else if (!deviceFound && midiConnected) {
            std::cout << "MIDI device disconnected" << std::endl;
            disconnectMidi();
        }
        
    } catch (RtMidiError &error) {
        std::cerr << "Error checking MIDI devices: " << error.getMessage() << std::endl;
        if (midiConnected) {
            disconnectMidi();
        }
    }
}

void MidiKeyboardMonitor::setupMidi()
{
    try {
        midiIn = std::make_unique<RtMidiIn>();
    } catch (RtMidiError &error) {
        std::cerr << "RtMidi initialization error: " << error.getMessage() << std::endl;
        deviceLabel->setText("MIDI Error: " + QString::fromStdString(error.getMessage()));
        deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 10px; font-weight: bold; }");
    }
}

void MidiKeyboardMonitor::attemptMidiConnection()
{
    if (!midiIn) return;
    
    try {
        unsigned int nPorts = midiIn->getPortCount();
        
        if (nPorts == 0) {
            midiConnected = false;
            deviceLabel->setText("No Controller Found");
            deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 10px; font-weight: bold; }");
            noteLabel->setText("Connect MIDI controller...");
            noteLabel->setStyleSheet("QLabel { color: #888; margin: 15px; }");
            return;
        }
        
        // Find the best MIDI device (prefer actual keyboards over Midi Through)
        int targetPort = -1;
        std::string bestDeviceName;
        
        for (unsigned int i = 0; i < nPorts; i++) {
            std::string portName = midiIn->getPortName(i);
            
            // Prefer specific devices we know about
            if (portName.find("Recital Play") != std::string::npos ||
                portName.find("Keyboard") != std::string::npos ||
                portName.find("Piano") != std::string::npos) {
                targetPort = i;
                bestDeviceName = portName;
                break;
            }
            
            // Skip Midi Through ports
            if (portName.find("Midi Through") == std::string::npos && targetPort == -1) {
                targetPort = i;
                bestDeviceName = portName;
            }
        }
        
        if (targetPort == -1) {
            midiConnected = false;
            return;
        }
        
        // Connect to the selected device
        if (midiIn->isPortOpen()) {
            midiIn->closePort();
        }
        
        midiIn->openPort(targetPort);
        midiIn->setCallback(&MidiKeyboardMonitor::midiCallback, this);
        midiIn->ignoreTypes(false, false, false);
        
        midiConnected = true;
        lastConnectedDevice = bestDeviceName;
        
        QString deviceText = QString("Connected: %1").arg(QString::fromStdString(bestDeviceName));
        deviceLabel->setText(deviceText);
        deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: green; margin-bottom: 10px; font-weight: bold; }");
        
        noteLabel->setText("Press keys...");
        noteLabel->setStyleSheet("QLabel { color: #2E8B57; margin: 15px; }");
        
        std::cout << "Successfully connected to: " << bestDeviceName << " (Port " << targetPort << ")" << std::endl;
        
    } catch (RtMidiError &error) {
        std::cerr << "Error connecting to MIDI device: " << error.getMessage() << std::endl;
        midiConnected = false;
        deviceLabel->setText("Connection Error");
        deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 10px; font-weight: bold; }");
    }
}

void MidiKeyboardMonitor::disconnectMidi()
{
    try {
        if (midiIn && midiIn->isPortOpen()) {
            midiIn->closePort();
        }
        midiConnected = false;
        activeNotes.clear();
        
        deviceLabel->setText("No Controller Found");
        deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 10px; font-weight: bold; }");
        
        noteLabel->setText("Connect MIDI controller...");
        noteLabel->setStyleSheet("QLabel { color: #888; margin: 15px; }");
        
        chordLabel->setText("");
        
    } catch (RtMidiError &error) {
        std::cerr << "Error disconnecting MIDI: " << error.getMessage() << std::endl;
    }
}

void MidiKeyboardMonitor::setupChordPatterns()
{
    chordPatterns["maj"] = {0, 4, 7};
    chordPatterns["maj7"] = {0, 4, 7, 11};
    chordPatterns["maj9"] = {0, 4, 7, 11, 14};
    chordPatterns["6"] = {0, 4, 7, 9};
    chordPatterns["add9"] = {0, 4, 7, 14};
    
    chordPatterns["m"] = {0, 3, 7};
    chordPatterns["m7"] = {0, 3, 7, 10};
    chordPatterns["m9"] = {0, 3, 7, 10, 14};
    chordPatterns["m6"] = {0, 3, 7, 9};
    chordPatterns["mMaj7"] = {0, 3, 7, 11};
    
    chordPatterns["7"] = {0, 4, 7, 10};
    chordPatterns["9"] = {0, 4, 7, 10, 14};
    chordPatterns["11"] = {0, 4, 7, 10, 14, 17};
    chordPatterns["13"] = {0, 4, 7, 10, 14, 17, 21};
    
    chordPatterns["dim"] = {0, 3, 6};
    chordPatterns["dim7"] = {0, 3, 6, 9};
    chordPatterns["ø7"] = {0, 3, 6, 10};
    
    chordPatterns["aug"] = {0, 4, 8};
    chordPatterns["aug7"] = {0, 4, 8, 10};
    
    chordPatterns["sus2"] = {0, 2, 7};
    chordPatterns["sus4"] = {0, 5, 7};
    chordPatterns["7sus2"] = {0, 2, 7, 10};
    chordPatterns["7sus4"] = {0, 5, 7, 10};
    
    chordPatterns["7♭­5"] = {0, 4, 6, 10};
    chordPatterns["7#5"] = {0, 4, 8, 10};
    chordPatterns["7♭9"] = {0, 4, 7, 10, 13};
    chordPatterns["7#9"] = {0, 4, 7, 10, 15};
    chordPatterns["7#11"] = {0, 4, 7, 10, 18};
}

void MidiKeyboardMonitor::midiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData)
{
    MidiKeyboardMonitor *monitor = static_cast<MidiKeyboardMonitor*>(userData);
    
    QMutexLocker locker(&monitor->midiQueueMutex);
    monitor->midiMessageQueue.push_back({timeStamp, *message});
}

void MidiKeyboardMonitor::processPendingMidiMessages()
{
    if (!midiConnected) return;
    
    std::vector<MidiMessage> messages;
    
    {
        QMutexLocker locker(&midiQueueMutex);
        messages = midiMessageQueue;
        midiMessageQueue.clear();
    }
    
    for (const auto& msg : messages) {
        if (msg.data.size() == 0) continue;
        
        if (msg.data.size() >= 3) {
            unsigned char status = msg.data[0];
            unsigned char noteNumber = msg.data[1];
            unsigned char velocity = msg.data[2];
            
            bool isNoteOn = ((status & 0xF0) == 0x90) && (velocity > 0);
            bool isNoteOff = ((status & 0xF0) == 0x80) || (((status & 0xF0) == 0x90) && (velocity == 0));
            
            const KeySignature& currentKey = keySignatures[currentKeyIndex];
            
            if (isNoteOn) {
                activeNotes.insert(noteNumber);
            }
            else if (isNoteOff) {
                activeNotes.erase(noteNumber);
            }
            
            if (!activeNotes.empty()) {
                QString notesList;
                for (int note : activeNotes) {
                    if (!notesList.isEmpty()) notesList += " + ";
                    notesList += midiNoteToNoteNameInKey(note, currentKey);
                }
                noteLabel->setText(notesList);
                noteLabel->setStyleSheet("QLabel { color: #2E8B57; margin: 15px; }");
                
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
    noteLabel->setStyleSheet("QLabel { color: #2E8B57; margin: 15px; }");
    chordLabel->setText("");
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

QString MidiKeyboardMonitor::midiNoteToNoteNameInKey(int midiNote, const KeySignature& key)
{
    const QString sharpNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
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
    
    // If not specified by key signature, use default (prefer sharps)
    if (noteName.isEmpty()) {
        noteName = sharpNames[noteClass];
    }
    
    return noteName + QString::number(octave);
}

QString MidiKeyboardMonitor::midiNoteToNoteName(int midiNote)
{
    const QString noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    int noteIndex = midiNote % 12;
    int octave = (midiNote / 12) - 1;
    
    return noteNames[noteIndex] + QString::number(octave);
}