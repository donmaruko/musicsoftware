#pragma once

#include <QMainWindow>
#include "MusicTypes.h"
#include "MusicTheoryEngine.h"
#include "MidiManager.h"
#include "ChordAnalyzer.h"
#include "UIManager.h"
#include <memory>

class MidiKeyboardMonitor : public QMainWindow
{
    Q_OBJECT

public:
    MidiKeyboardMonitor(QWidget *parent = nullptr);
    ~MidiKeyboardMonitor();

private slots:
    // MIDI event handlers
    void onDeviceConnected(const QString& deviceName);
    void onDeviceDisconnected();
    void onNoteEvent(const MusicTypes::MidiEvent& event);
    void onMidiError(const QString& error);
    
    // UI event handlers
    void onKeySignatureChanged(int index);

private:
    // Core components
    std::unique_ptr<MidiManager> midiManager;
    std::unique_ptr<ChordAnalyzer> chordAnalyzer;
    std::unique_ptr<UIManager> uiManager;
    MusicTheoryEngine* theoryEngine; // Singleton reference
    
    // Current state
    int currentKeySignatureIndex;
    
    // Methods
    void initializeComponents();
    void connectSignals();
    void updateDisplays();
    QString formatMidiLogEntry(const MusicTypes::MidiEvent& event, const MusicTypes::KeySignature& key) const;
};