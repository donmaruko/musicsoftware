#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QFont>
#include <QString>
#include <QMutex>
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

private:
    // GUI components
    QWidget *centralWidget;
    QVBoxLayout *layout;
    QLabel *deviceLabel;
    QLabel *noteLabel;
    QLabel *chordLabel;
    QTimer *clearTimer;
    QTimer *midiProcessTimer;
    
    // MIDI components
    std::unique_ptr<RtMidiIn> midiIn;
    
    // Thread-safe MIDI message queue
    struct MidiMessage {
        double timeStamp;
        std::vector<unsigned char> data;
    };
    std::vector<MidiMessage> midiMessageQueue;
    QMutex midiQueueMutex;
    
    // Chord detection
    std::set<int> activeNotes;
    std::map<std::string, std::vector<int>> chordPatterns;
    
    // Methods
    void setupUI();
    void setupMidi();
    void setupChordPatterns();
    void connectToFirstMidiDevice();
    QString midiNoteToNoteName(int midiNote);
    QString detectChord();
    std::string getChordName(const std::vector<int>& intervals, const std::string& rootNote);
    
    // Static callback for RtMidi (must be static)
    static void midiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData);
};