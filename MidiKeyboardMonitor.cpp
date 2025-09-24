#include "MidiKeyboardMonitor.h"
#include <QApplication>
#include <iostream>
#include <iomanip>
#include <algorithm>

MidiKeyboardMonitor::MidiKeyboardMonitor(QWidget *parent)
    : QMainWindow(parent)
    , centralWidget(nullptr)
    , layout(nullptr)
    , deviceLabel(nullptr)
    , noteLabel(nullptr)
    , chordLabel(nullptr)
    , clearTimer(new QTimer(this))
    , midiProcessTimer(new QTimer(this))
    , midiIn(nullptr)
{
    setupUI();
    setupChordPatterns();
    setupMidi();
    
    // Connect timers
    connect(clearTimer, &QTimer::timeout, this, &MidiKeyboardMonitor::clearDisplay);
    connect(midiProcessTimer, &QTimer::timeout, this, &MidiKeyboardMonitor::processPendingMidiMessages);
    
    clearTimer->setSingleShot(true);
    midiProcessTimer->start(10); // Process MIDI messages every 10ms
}

MidiKeyboardMonitor::~MidiKeyboardMonitor()
{
    if (midiIn && midiIn->isPortOpen()) {
        midiIn->closePort();
    }
}

void MidiKeyboardMonitor::setupUI()
{
    setWindowTitle("MIDI Keyboard Monitor - Chord Detection");
    setMinimumSize(500, 400);
    resize(700, 500);
    
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    layout = new QVBoxLayout(centralWidget);
    layout->setAlignment(Qt::AlignCenter);
    
    // Device connection label
    deviceLabel = new QLabel("No MIDI device connected", this);
    deviceLabel->setAlignment(Qt::AlignCenter);
    deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; margin-bottom: 20px; }");
    layout->addWidget(deviceLabel);
    
    // Note display label
    noteLabel = new QLabel("Press keys...", this);
    noteLabel->setAlignment(Qt::AlignCenter);
    
    QFont noteFont;
    noteFont.setPointSize(36);
    noteFont.setBold(true);
    noteLabel->setFont(noteFont);
    noteLabel->setStyleSheet("QLabel { color: #2E8B57; margin-bottom: 15px; }");
    
    layout->addWidget(noteLabel);
    
    // Chord display label
    chordLabel = new QLabel("", this);
    chordLabel->setAlignment(Qt::AlignCenter);
    
    QFont chordFont;
    chordFont.setPointSize(28);
    chordFont.setBold(true);
    chordLabel->setFont(chordFont);
    chordLabel->setStyleSheet("QLabel { color: #FF6347; margin-top: 10px; }");
    
    layout->addWidget(chordLabel);
    
    setStyleSheet("QMainWindow { background-color: white; }");
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
    chordPatterns["min"] = {0, 3, 7};
    chordPatterns["min7"] = {0, 3, 7, 10};
    chordPatterns["min9"] = {0, 3, 7, 10, 14};
    chordPatterns["min6"] = {0, 3, 7, 9};
    chordPatterns["minMaj7"] = {0, 3, 7, 11};
    
    // Dominant chords
    chordPatterns["7"] = {0, 4, 7, 10};
    chordPatterns["9"] = {0, 4, 7, 10, 14};
    chordPatterns["11"] = {0, 4, 7, 10, 14, 17};
    chordPatterns["13"] = {0, 4, 7, 10, 14, 17, 21};
    
    // Diminished chords
    chordPatterns["dim"] = {0, 3, 6};
    chordPatterns["dim7"] = {0, 3, 6, 9};
    chordPatterns["half-dim7"] = {0, 3, 6, 10};
    
    // Augmented chords
    chordPatterns["aug"] = {0, 4, 8};
    chordPatterns["aug7"] = {0, 4, 8, 10};
    
    // Suspended chords
    chordPatterns["sus2"] = {0, 2, 7};
    chordPatterns["sus4"] = {0, 5, 7};
    chordPatterns["7sus2"] = {0, 2, 7, 10};
    chordPatterns["7sus4"] = {0, 5, 7, 10};
    
    // Altered dominants
    chordPatterns["7b5"] = {0, 4, 6, 10};
    chordPatterns["7#5"] = {0, 4, 8, 10};
    chordPatterns["7b9"] = {0, 4, 7, 10, 13};
    chordPatterns["7#9"] = {0, 4, 7, 10, 15};
    chordPatterns["7#11"] = {0, 4, 7, 10, 18};
    
    // Simple intervals
    chordPatterns["2nd"] = {0, 1};
    chordPatterns["2nd"] = {0, 2}; // Major 2nd
    chordPatterns["3rd"] = {0, 3}; // Minor 3rd  
    chordPatterns["3rd"] = {0, 4}; // Major 3rd
    chordPatterns["4th"] = {0, 5}; // Perfect 4th
    chordPatterns["tritone"] = {0, 6}; // Tritone
    chordPatterns["5th"] = {0, 7}; // Perfect 5th
    chordPatterns["6th"] = {0, 8}; // Minor 6th
    chordPatterns["6th"] = {0, 9}; // Major 6th
    chordPatterns["7th"] = {0, 10}; // Minor 7th
    chordPatterns["7th"] = {0, 11}; // Major 7th
    chordPatterns["octave"] = {0, 12}; // Octave
}

void MidiKeyboardMonitor::setupMidi()
{
    try {
        midiIn = std::make_unique<RtMidiIn>();
        
        unsigned int nPorts = midiIn->getPortCount();
        std::cout << "Available MIDI input ports: " << nPorts << std::endl;
        
        if (nPorts == 0) {
            deviceLabel->setText("No MIDI input devices found!");
            deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 20px; }");
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
        deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 20px; }");
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
            deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: green; margin-bottom: 20px; }");
            
            std::cout << "Successfully connected to: " << portName << " (Port " << targetPort << ")" << std::endl;
        }
    } catch (RtMidiError &error) {
        std::cerr << "Error connecting to MIDI device: " << error.getMessage() << std::endl;
        deviceLabel->setText("Connection Error: " + QString::fromStdString(error.getMessage()));
        deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 20px; }");
    }
}

void MidiKeyboardMonitor::midiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData)
{
    MidiKeyboardMonitor *monitor = static_cast<MidiKeyboardMonitor*>(userData);
    
    // Thread-safe: add message to queue instead of processing directly
    QMutexLocker locker(&monitor->midiQueueMutex);
    monitor->midiMessageQueue.push_back({timeStamp, *message});
}

void MidiKeyboardMonitor::processPendingMidiMessages()
{
    std::vector<MidiMessage> messages;
    
    // Get all pending messages
    {
        QMutexLocker locker(&midiQueueMutex);
        messages = midiMessageQueue;
        midiMessageQueue.clear();
    }
    
    // Process all messages
    for (const auto& msg : messages) {
        if (msg.data.size() == 0) continue;
        
        // Print raw MIDI data for debugging
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
            
            if (isNoteOn) {
                activeNotes.insert(noteNumber);
                std::cout << "Note ON: " << midiNoteToNoteName(noteNumber).toStdString() 
                         << " (MIDI: " << (int)noteNumber << ", Velocity: " << (int)velocity << ")" << std::endl;
            }
            else if (isNoteOff) {
                activeNotes.erase(noteNumber);
                std::cout << "Note OFF: " << midiNoteToNoteName(noteNumber).toStdString() 
                         << " (MIDI: " << (int)noteNumber << ")" << std::endl;
            }
            
            // Update display
            if (!activeNotes.empty()) {
                // Show active notes
                QString notesList;
                for (int note : activeNotes) {
                    if (!notesList.isEmpty()) notesList += " + ";
                    notesList += midiNoteToNoteName(note);
                }
                noteLabel->setText(notesList);
                
                // Detect and show chord
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

QString MidiKeyboardMonitor::detectChord()
{
    if (activeNotes.size() < 2) return "";
    
    // Convert to sorted vector for easier processing
    std::vector<int> notes(activeNotes.begin(), activeNotes.end());
    std::sort(notes.begin(), notes.end());
    
    // Handle simple intervals (2 notes)
    if (notes.size() == 2) {
        int interval = notes[1] - notes[0];
        QString rootNote = midiNoteToNoteName(notes[0]);
        
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
        
        std::string rootNoteName = midiNoteToNoteName(rootNote).toStdString();
        std::string chordName = getChordName(intervals, rootNoteName);
        
        if (!chordName.empty()) {
            return QString::fromStdString(chordName);
        }
    }
    
    // If no standard chord found, show note count
    return QString("Cluster (%1 notes)").arg(activeNotes.size());
}

std::string MidiKeyboardMonitor::getChordName(const std::vector<int>& intervals, const std::string& rootNote)
{
    // Check against known chord patterns
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

QString MidiKeyboardMonitor::midiNoteToNoteName(int midiNote)
{
    const QString noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    int noteIndex = midiNote % 12;
    int octave = (midiNote / 12) - 1;
    
    return noteNames[noteIndex] + QString::number(octave);
}