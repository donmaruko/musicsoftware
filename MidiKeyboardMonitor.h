#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QFont>
#include <QString>
#include <QMutex>
#include <QComboBox>
#include <QGroupBox>
#include <RtMidi.h>
#include <memory>
#include <string>
#include <set>
#include <vector>
#include <map>

class MidiKeyboardMonitor : public QMainWindow
{
    Q_OBJECT

public:
    MidiKeyboardMonitor(QWidget *parent = nullptr);
    ~MidiKeyboardMonitor();

private slots:
    void clearDisplay();
    void processPendingMidiMessages();
    void onKeySignatureChanged();
    void checkForMidiDevices();

private:
    // GUI components
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QHBoxLayout *controlsLayout;
    QGroupBox *keySelectionGroup;
    QComboBox *keySignatureCombo;
    QLabel *deviceLabel;
    QLabel *noteLabel;
    QLabel *chordLabel;
    QTimer *clearTimer;
    QTimer *midiProcessTimer;
    QTimer *deviceCheckTimer;
    
    // MIDI components
    std::unique_ptr<RtMidiIn> midiIn;
    bool midiConnected;
    std::string lastConnectedDevice;
    
    // Thread-safe MIDI message queue
    struct MidiMessage {
        double timeStamp;
        std::vector<unsigned char> data;
    };
    std::vector<MidiMessage> midiMessageQueue;
    QMutex midiQueueMutex;
    
    // Key signature and music theory
    struct KeySignature {
        std::string name;
        std::vector<int> sharps;  // MIDI note numbers that should be sharp
        std::vector<int> flats;   // MIDI note numbers that should be flat
        int tonic;                // Root note of the key (0-11)
        bool isMajor;
    };
    
    std::vector<KeySignature> keySignatures;
    int currentKeyIndex;
    
    // Chord detection
    std::set<int> activeNotes;
    std::map<std::string, std::vector<int>> chordPatterns;
    
    // Methods
    void setupUI();
    void setupMidi();
    void setupChordPatterns();
    void setupKeySignatures();
    void attemptMidiConnection();
    void disconnectMidi();
    QString midiNoteToNoteName(int midiNote);
    QString midiNoteToNoteNameInKey(int midiNote, const KeySignature& key);
    QString detectChord();
    std::string getChordName(const std::vector<int>& intervals, const std::string& rootNote);
    
    // Static callback for RtMidi (must be static)
    static void midiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData);
};