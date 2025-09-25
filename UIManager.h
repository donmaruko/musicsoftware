#pragma once

#include "MusicTypes.h"
#include <QObject>
#include <QWidget>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QTextEdit>
#include <QTimer>
#include <deque>

class UIManager : public QObject {
    Q_OBJECT

public:
    explicit UIManager(QMainWindow* mainWindow, QObject* parent = nullptr);
    ~UIManager() = default;

    // UI Setup
    void setupUI();
    void populateKeySignatureCombo(const std::vector<MusicTypes::KeySignature>& keySignatures);
    
    // Display updates
    void updateDeviceStatus(const QString& deviceName, bool connected);
    void updateNoteDisplay(const QString& noteText);
    void updateChordDisplay(const QString& chordText);
    void updateRomanNumeralDisplay(const QString& romanText, bool isNonDiatonic);
    void addMidiLogEntry(const QString& entry);
    void clearDisplays();
    
    // Timer control
    void startClearTimer();
    void stopClearTimer();
    
    // Getters
    int getCurrentKeySignatureIndex() const;

signals:
    void keySignatureChanged(int index);

private slots:
    void onKeySignatureChanged();
    void clearDisplayDelayed();

private:
    QMainWindow* mainWindow;
    
    // Main layout components
    QWidget* centralWidget;
    QHBoxLayout* mainHorizontalLayout;
    QWidget* leftPanel;
    QWidget* rightPanel;
    QVBoxLayout* leftLayout;
    QVBoxLayout* rightLayout;
    
    // Control components
    QGroupBox* keySelectionGroup;
    QHBoxLayout* controlsLayout;
    QComboBox* keySignatureCombo;
    
    // Display components
    QLabel* deviceLabel;
    QLabel* noteLabel;
    QLabel* chordLabel;
    QLabel* romanNumeralLabel;
    
    // MIDI log components
    QGroupBox* midiLogGroup;
    QTextEdit* midiLogDisplay;
    std::deque<QString> midiLogEntries;
    static const int MAX_LOG_ENTRIES = 50;  // Increased from 10 to fill more space
    
    // Timer for clearing displays
    QTimer* clearTimer;
    
    // Helper methods
    void setupLeftPanel();
    void setupRightPanel();
    void setupControlPanel();
    void setupDisplayLabels();
    void setupMidiLog();
    void updateMidiLogDisplay();
};