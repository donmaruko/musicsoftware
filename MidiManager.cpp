#include "MidiManager.h"
#include <iostream>
#include <QMutexLocker>
#include <thread>
#include <chrono>

MidiManager::MidiManager(QObject* parent)
    : QObject(parent)
    , midiIn(nullptr)
    , midiConnected(false)
    , isDestroying(false)
    , deviceCheckTimer(new QTimer(this))
    , midiProcessTimer(new QTimer(this))
{
    setupMidi();
    
    connect(deviceCheckTimer, &QTimer::timeout, this, &MidiManager::checkForMidiDevices);
    connect(midiProcessTimer, &QTimer::timeout, this, &MidiManager::processPendingMidiMessages);
    
    midiProcessTimer->start(10); // Process MIDI messages every 10ms
}

MidiManager::~MidiManager() {
    std::cout << "MidiManager destructor called" << std::endl;
    
    // Set destroying flag FIRST to prevent callback execution
    isDestroying = true;
    
    // Stop timers
    if (deviceCheckTimer) {
        deviceCheckTimer->stop();
    }
    if (midiProcessTimer) {
        midiProcessTimer->stop();
    }
    
    // Disconnect MIDI
    disconnectMidi();
    
    // Clear any pending messages
    QMutexLocker locker(&midiQueueMutex);
    midiMessageQueue.clear();
    
    std::cout << "MidiManager destructor finished" << std::endl;
}

void MidiManager::setupMidi() {
    try {
        midiIn = std::make_unique<RtMidiIn>();
    } catch (RtMidiError& error) {
        std::cerr << "RtMidi initialization error: " << error.getMessage() << std::endl;
        emit midiError(QString::fromStdString(error.getMessage()));
    }
}

bool MidiManager::isConnected() const {
    return midiConnected;
}

const std::string& MidiManager::getConnectedDeviceName() const {
    return lastConnectedDevice;
}

const std::set<int>& MidiManager::getActiveNotes() const {
    return activeNotes;
}

void MidiManager::clearActiveNotes() {
    activeNotes.clear();
}

void MidiManager::startDeviceMonitoring() {
    deviceCheckTimer->start(1000); // Check for devices every second
    checkForMidiDevices(); // Initial check
}

void MidiManager::stopDeviceMonitoring() {
    deviceCheckTimer->stop();
}

void MidiManager::checkForMidiDevices() {
    if (!midiIn) return;
    
    try {
        unsigned int nPorts = midiIn->getPortCount();
        
        bool deviceFound = false;
        std::string foundDeviceName;
        
        // Check for any MIDI devices
        for (unsigned int i = 0; i < nPorts; i++) {
            std::string portName = midiIn->getPortName(i);
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
        
    } catch (RtMidiError& error) {
        std::cerr << "Error checking MIDI devices: " << error.getMessage() << std::endl;
        if (midiConnected) {
            disconnectMidi();
        }
        emit midiError(QString::fromStdString(error.getMessage()));
    }
}

void MidiManager::attemptMidiConnection() {
    if (!midiIn) return;
    
    try {
        unsigned int nPorts = midiIn->getPortCount();
        
        if (nPorts == 0) {
            midiConnected = false;
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
        midiIn->setCallback(&MidiManager::midiCallback, this);
        midiIn->ignoreTypes(false, false, false);
        
        midiConnected = true;
        lastConnectedDevice = bestDeviceName;
        
        emit deviceConnected(QString::fromStdString(bestDeviceName));
        
        std::cout << "Successfully connected to: " << bestDeviceName << " (Port " << targetPort << ")" << std::endl;
        
    } catch (RtMidiError& error) {
        std::cerr << "Error connecting to MIDI device: " << error.getMessage() << std::endl;
        midiConnected = false;
        emit midiError(QString::fromStdString(error.getMessage()));
    }
}

void MidiManager::disconnectMidi() {
    try {
        std::cout << "Disconnecting MIDI..." << std::endl;
        
        // Set the destroying flag to prevent new callbacks
        isDestroying = true;
        
        if (midiIn && midiIn->isPortOpen()) {
            std::cout << "Closing MIDI port..." << std::endl;
            // Cancel the callback first
            midiIn->cancelCallback();
            // Then close the port
            midiIn->closePort();
            std::cout << "MIDI port closed" << std::endl;
        }
        
        midiConnected = false;
        activeNotes.clear();
        
        // Wait a brief moment to ensure no callbacks are still running
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Now it's safe to reset the MIDI input
        if (midiIn) {
            midiIn.reset();
            std::cout << "MIDI input reset" << std::endl;
        }
        
        if (!lastConnectedDevice.empty() && !isDestroying) {
            emit deviceDisconnected();
            lastConnectedDevice.clear();
        }
        
        std::cout << "MIDI disconnected successfully" << std::endl;
        
    } catch (RtMidiError& error) {
        std::cerr << "RtMidi error disconnecting: " << error.getMessage() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Standard exception during MIDI disconnection: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error during MIDI disconnection" << std::endl;
    }
}

void MidiManager::processPendingMidiMessages() {
    if (!midiConnected) return;
    
    std::vector<MusicTypes::MidiMessage> messages;
    
    {
        QMutexLocker locker(&midiQueueMutex);
        messages = midiMessageQueue;
        midiMessageQueue.clear();
    }
    
    for (const auto& msg : messages) {
        if (msg.data.empty()) continue;
        
        MusicTypes::MidiEvent event = parseMidiMessage(msg.data);
        
        if (event.type == MusicTypes::MidiEventType::NoteOn) {
            activeNotes.insert(event.noteNumber);
            emit noteEvent(event);
        } else if (event.type == MusicTypes::MidiEventType::NoteOff) {
            activeNotes.erase(event.noteNumber);
            emit noteEvent(event);
        }
    }
}

MusicTypes::MidiEvent MidiManager::parseMidiMessage(const std::vector<unsigned char>& data) const {
    MusicTypes::MidiEvent event;
    event.type = MusicTypes::MidiEventType::Unknown;
    event.noteNumber = 0;
    event.velocity = 0;
    event.channel = 0;
    
    if (data.size() >= 3) {
        unsigned char status = data[0];
        unsigned char noteNumber = data[1];
        unsigned char velocity = data[2];
        
        event.noteNumber = noteNumber;
        event.velocity = velocity;
        event.channel = status & 0x0F;
        
        bool isNoteOn = ((status & 0xF0) == 0x90) && (velocity > 0);
        bool isNoteOff = ((status & 0xF0) == 0x80) || (((status & 0xF0) == 0x90) && (velocity == 0));
        
        if (isNoteOn) {
            event.type = MusicTypes::MidiEventType::NoteOn;
        } else if (isNoteOff) {
            event.type = MusicTypes::MidiEventType::NoteOff;
        }
    }
    
    return event;
}

void MidiManager::midiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData) {
    MidiManager* manager = static_cast<MidiManager*>(userData);
    
    // Safety check: don't process if manager is being destroyed
    if (!manager || manager->isDestroying.load()) {
        return;
    }
    
    try {
        QMutexLocker locker(&manager->midiQueueMutex);
        if (!manager->isDestroying.load()) {
            manager->midiMessageQueue.push_back({timeStamp, *message});
        }
    } catch (...) {
        // Ignore any exceptions during destruction
    }
}