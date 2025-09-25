#pragma once

#include "MusicTypes.h"
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <RtMidi.h>
#include <memory>
#include <set>
#include <vector>
#include <string>
#include <atomic>

class MidiManager : public QObject {
    Q_OBJECT

public:
    explicit MidiManager(QObject* parent = nullptr);
    ~MidiManager();

    // Connection management
    bool isConnected() const;
    const std::string& getConnectedDeviceName() const;
    void startDeviceMonitoring();
    void stopDeviceMonitoring();
    
    // Note state
    const std::set<int>& getActiveNotes() const;
    void clearActiveNotes();

signals:
    void deviceConnected(const QString& deviceName);
    void deviceDisconnected();
    void noteEvent(const MusicTypes::MidiEvent& event);
    void midiError(const QString& error);

private slots:
    void checkForMidiDevices();
    void processPendingMidiMessages();

private:
    // MIDI components
    std::unique_ptr<RtMidiIn> midiIn;
    bool midiConnected;
    std::string lastConnectedDevice;
    
    // Safety flag for destruction
    std::atomic<bool> isDestroying;
    
    // Active notes tracking
    std::set<int> activeNotes;
    
    // Thread-safe message queue
    std::vector<MusicTypes::MidiMessage> midiMessageQueue;
    QMutex midiQueueMutex;
    
    // Timers
    QTimer* deviceCheckTimer;
    QTimer* midiProcessTimer;
    
    // Methods
    void setupMidi();
    void attemptMidiConnection();
    void disconnectMidi();
    MusicTypes::MidiEvent parseMidiMessage(const std::vector<unsigned char>& data) const;
    
    // Static callback for RtMidi
    static void midiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData);
};