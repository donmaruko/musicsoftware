#include "MidiKeyboardMonitor.h"
#include <iostream>

MidiKeyboardMonitor::MidiKeyboardMonitor(QWidget *parent)
    : QMainWindow(parent)
    , currentKeySignatureIndex(0)
{
    initializeComponents();
    connectSignals();
    
    // Set up UI
    uiManager->setupUI();
    uiManager->populateKeySignatureCombo(theoryEngine->getKeySignatures());
    
    // Start MIDI monitoring
    midiManager->startDeviceMonitoring();
    
    std::cout << "Starting Keyboard Monitor..." << std::endl;
}

MidiKeyboardMonitor::~MidiKeyboardMonitor() {
    // Ensure proper cleanup order to prevent segfaults
    std::cout << "Shutting down Keyboard Monitor..." << std::endl;
    
    // Stop MIDI monitoring first
    if (midiManager) {
        midiManager->stopDeviceMonitoring();
    }
    
    // Components will be cleaned up automatically due to smart pointers
    // but we explicitly reset them to control the order
    midiManager.reset();
    chordAnalyzer.reset();
    uiManager.reset();
}

void MidiKeyboardMonitor::initializeComponents() {
    // Get theory engine instance (singleton)
    theoryEngine = &MusicTheoryEngine::instance();
    
    // Create component instances
    midiManager = std::make_unique<MidiManager>(this);
    chordAnalyzer = std::make_unique<ChordAnalyzer>(theoryEngine);
    uiManager = std::make_unique<UIManager>(this, this);
}

void MidiKeyboardMonitor::connectSignals() {
    // Connect MIDI manager signals
    connect(midiManager.get(), &MidiManager::deviceConnected,
            this, &MidiKeyboardMonitor::onDeviceConnected);
    connect(midiManager.get(), &MidiManager::deviceDisconnected,
            this, &MidiKeyboardMonitor::onDeviceDisconnected);
    connect(midiManager.get(), &MidiManager::noteEvent,
            this, &MidiKeyboardMonitor::onNoteEvent);
    connect(midiManager.get(), &MidiManager::midiError,
            this, &MidiKeyboardMonitor::onMidiError);
    
    // Connect UI manager signals
    connect(uiManager.get(), &UIManager::keySignatureChanged,
            this, &MidiKeyboardMonitor::onKeySignatureChanged);
}

void MidiKeyboardMonitor::onDeviceConnected(const QString& deviceName) {
    uiManager->updateDeviceStatus(deviceName, true);
    uiManager->addMidiLogEntry("MIDI Connected: " + deviceName);
    std::cout << "Device connected: " << deviceName.toStdString() << std::endl;
}

void MidiKeyboardMonitor::onDeviceDisconnected() {
    uiManager->updateDeviceStatus("", false);
    uiManager->addMidiLogEntry("MIDI Disconnected");
    midiManager->clearActiveNotes();
    std::cout << "Device disconnected" << std::endl;
}

void MidiKeyboardMonitor::onNoteEvent(const MusicTypes::MidiEvent& event) {
    const MusicTypes::KeySignature& currentKey = theoryEngine->getKeySignature(currentKeySignatureIndex);
    
    // Add to MIDI log
    QString logEntry = formatMidiLogEntry(event, currentKey);
    uiManager->addMidiLogEntry(logEntry);
    
    // Update displays
    updateDisplays();
}

void MidiKeyboardMonitor::onMidiError(const QString& error) {
    uiManager->addMidiLogEntry("MIDI Error: " + error);
    std::cerr << "MIDI Error: " << error.toStdString() << std::endl;
}

void MidiKeyboardMonitor::onKeySignatureChanged(int index) {
    currentKeySignatureIndex = index;
    const MusicTypes::KeySignature& key = theoryEngine->getKeySignature(index);
    std::cout << "Key signature changed to: " << key.name << std::endl;
    
    // Update displays with current notes in new key
    updateDisplays();
}

void MidiKeyboardMonitor::updateDisplays() {
    const std::set<int>& activeNotes = midiManager->getActiveNotes();
    const MusicTypes::KeySignature& currentKey = theoryEngine->getKeySignature(currentKeySignatureIndex);
    
    if (activeNotes.empty()) {
        // Only start clear timer when there are no active notes
        uiManager->startClearTimer();
        return;
    }
    
    // Stop any pending clear timer since we have active notes
    uiManager->stopClearTimer();
    
    // Build note list display
    QString notesList;
    for (int note : activeNotes) {
        if (!notesList.isEmpty()) notesList += " + ";
        notesList += theoryEngine->midiNoteToNoteNameInKey(note, currentKey);
    }
    uiManager->updateNoteDisplay(notesList);
    
    // Analyze and display chord information
    QString chordName = chordAnalyzer->analyzeNotes(activeNotes, currentKey);
    uiManager->updateChordDisplay(chordName);
    
    // For 3+ notes, show Roman numeral analysis
    if (activeNotes.size() >= 3) {
        std::vector<int> notes(activeNotes.begin(), activeNotes.end());
        std::sort(notes.begin(), notes.end());
        
        MusicTypes::ChordAnalysis analysis = chordAnalyzer->analyzeChord(notes, currentKey);
        
        QString romanDisplay = analysis.romanNumeral;
        if (!analysis.functionName.isEmpty() && analysis.functionName != "Non-functional") {
            romanDisplay += " (" + analysis.functionName + ")";
        }
        
        // Add additional info for non-diatonic chords
        if (analysis.isNonDiatonic && !analysis.accidentalNotes.empty()) {
            romanDisplay += " - non-diatonic";
        }
        
        uiManager->updateRomanNumeralDisplay(romanDisplay, analysis.isNonDiatonic);
    } else {
        // Clear Roman numeral for intervals
        uiManager->updateRomanNumeralDisplay("", false);
    }
}

QString MidiKeyboardMonitor::formatMidiLogEntry(const MusicTypes::MidiEvent& event, const MusicTypes::KeySignature& key) const {
    QString noteName = theoryEngine->midiNoteToNoteNameInKey(event.noteNumber, key);
    QString eventType = (event.type == MusicTypes::MidiEventType::NoteOn) ? "ON" : "OFF";
    return noteName + " " + eventType + " vel: " + QString::number(event.velocity);
}