#include "UIManager.h"
#include <QTextCursor>
#include <QFont>

UIManager::UIManager(QMainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , mainWindow(mainWindow)
    , centralWidget(nullptr)
    , mainHorizontalLayout(nullptr)
    , leftPanel(nullptr)
    , rightPanel(nullptr)
    , leftLayout(nullptr)
    , rightLayout(nullptr)
    , keySelectionGroup(nullptr)
    , controlsLayout(nullptr)
    , keySignatureCombo(nullptr)
    , deviceLabel(nullptr)
    , noteLabel(nullptr)
    , chordLabel(nullptr)
    , romanNumeralLabel(nullptr)
    , midiLogGroup(nullptr)
    , midiLogDisplay(nullptr)
    , clearTimer(new QTimer(this))
{
    connect(clearTimer, &QTimer::timeout, this, &UIManager::clearDisplayDelayed);
    clearTimer->setSingleShot(true);
}

void UIManager::setupUI() {
    mainWindow->setWindowTitle("MIDI Keyboard Monitor");
    mainWindow->setMinimumSize(900, 500);
    mainWindow->resize(1100, 600);
    
    centralWidget = new QWidget(mainWindow);
    mainWindow->setCentralWidget(centralWidget);
    
    // Main horizontal layout
    mainHorizontalLayout = new QHBoxLayout(centralWidget);
    mainHorizontalLayout->setSpacing(10);
    
    setupLeftPanel();
    setupRightPanel();
    
    // Add panels to main layout
    mainHorizontalLayout->addWidget(leftPanel);
    mainHorizontalLayout->addWidget(rightPanel);
    
    mainWindow->setStyleSheet("QMainWindow { background-color: white; }");
}

void UIManager::setupLeftPanel() {
    // Left panel for MIDI log (30% of width)
    leftPanel = new QWidget(mainWindow);
    leftPanel->setMaximumWidth(280);
    leftPanel->setMinimumWidth(220);
    leftLayout = new QVBoxLayout(leftPanel);
    
    setupMidiLog();
    leftLayout->addStretch();
}

void UIManager::setupRightPanel() {
    // Right panel for main content (70% of width)
    rightPanel = new QWidget(mainWindow);
    rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(15);
    
    setupControlPanel();
    setupDisplayLabels();
}

void UIManager::setupControlPanel() {
    // Control panel
    keySelectionGroup = new QGroupBox("Settings", rightPanel);
    keySelectionGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; font-size: 14px; padding: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; }"
    );
    
    controlsLayout = new QHBoxLayout(keySelectionGroup);
    
    // Key signature selection
    keySignatureCombo = new QComboBox(rightPanel);
    keySignatureCombo->setStyleSheet(
        "QComboBox { font-size: 13px; padding: 5px; min-width: 150px; }"
    );
    
    connect(keySignatureCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UIManager::onKeySignatureChanged);
    
    controlsLayout->addWidget(keySignatureCombo);
    controlsLayout->addStretch();
    
    rightLayout->addWidget(keySelectionGroup);
}

void UIManager::setupDisplayLabels() {
    // Device status label
    deviceLabel = new QLabel("No Controller Found", rightPanel);
    deviceLabel->setAlignment(Qt::AlignCenter);
    deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 10px; font-weight: bold; }");
    rightLayout->addWidget(deviceLabel);
    
    rightLayout->addStretch(1);
    
    // Note display label
    noteLabel = new QLabel("Connect MIDI Controller", rightPanel);
    noteLabel->setAlignment(Qt::AlignCenter);
    
    QFont noteFont;
    noteFont.setPointSize(36);
    noteFont.setBold(true);
    noteLabel->setFont(noteFont);
    noteLabel->setStyleSheet("QLabel { color: #888; margin: 15px; }");
    
    rightLayout->addWidget(noteLabel);
    
    // Chord display label
    chordLabel = new QLabel("", rightPanel);
    chordLabel->setAlignment(Qt::AlignCenter);
    
    QFont chordFont;
    chordFont.setPointSize(28);
    chordFont.setBold(true);
    chordLabel->setFont(chordFont);
    chordLabel->setStyleSheet("QLabel { color: #FF6347; margin: 15px; }");
    
    rightLayout->addWidget(chordLabel);
    
    // Roman numeral analysis label
    romanNumeralLabel = new QLabel("", rightPanel);
    romanNumeralLabel->setAlignment(Qt::AlignCenter);
    
    QFont romanFont;
    romanFont.setPointSize(24);
    romanFont.setBold(true);
    romanNumeralLabel->setFont(romanFont);
    romanNumeralLabel->setStyleSheet("QLabel { color: #4682B4; margin: 15px; }");
    
    rightLayout->addWidget(romanNumeralLabel);
    
    rightLayout->addStretch(1);
}

void UIManager::setupMidiLog() {
    // MIDI Log Group
    midiLogGroup = new QGroupBox("MIDI Log", leftPanel);
    midiLogGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; font-size: 14px; padding: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; }"
    );
    
    QVBoxLayout* logLayout = new QVBoxLayout(midiLogGroup);
    
    // MIDI log display
    midiLogDisplay = new QTextEdit(midiLogGroup);
    midiLogDisplay->setReadOnly(true);
    midiLogDisplay->setMaximumHeight(400);
    midiLogDisplay->setStyleSheet(
        "QTextEdit { "
        "font-family: 'Courier New', monospace; "
        "font-size: 12px; "
        "background-color: #f8f8f8; "
        "border: 1px solid #ccc; "
        "}"
    );
    
    logLayout->addWidget(midiLogDisplay);
    leftLayout->addWidget(midiLogGroup);
}

void UIManager::populateKeySignatureCombo(const std::vector<MusicTypes::KeySignature>& keySignatures) {
    keySignatureCombo->clear();
    for (const auto& key : keySignatures) {
        keySignatureCombo->addItem(QString::fromStdString(key.name));
    }
    keySignatureCombo->setCurrentIndex(0);
}

void UIManager::updateDeviceStatus(const QString& deviceName, bool connected) {
    if (connected) {
        QString deviceText = QString("Connected: %1").arg(deviceName);
        deviceLabel->setText(deviceText);
        deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: green; margin-bottom: 10px; font-weight: bold; }");
        
        noteLabel->setText("Press keys");
        noteLabel->setStyleSheet("QLabel { color: #2E8B57; margin: 15px; }");
    } else {
        deviceLabel->setText("No Controller Found");
        deviceLabel->setStyleSheet("QLabel { font-size: 14px; color: red; margin-bottom: 10px; font-weight: bold; }");
        
        noteLabel->setText("Connect MIDI controller");
        noteLabel->setStyleSheet("QLabel { color: #888; margin: 15px; }");
        
        chordLabel->setText("");
        romanNumeralLabel->setText("");
    }
}

void UIManager::updateNoteDisplay(const QString& noteText) {
    noteLabel->setText(noteText);
    noteLabel->setStyleSheet("QLabel { color: #2E8B57; margin: 15px; }");
}

void UIManager::startClearTimer() {
    clearTimer->start(100); // Clear after 100ms of no activity
}

void UIManager::stopClearTimer() {
    clearTimer->stop();
}

void UIManager::updateChordDisplay(const QString& chordText) {
    chordLabel->setText(chordText);
}

void UIManager::updateRomanNumeralDisplay(const QString& romanText, bool isNonDiatonic) {
    romanNumeralLabel->setText(romanText);
    
    // Style the Roman numeral based on whether it's diatonic
    if (isNonDiatonic) {
        romanNumeralLabel->setStyleSheet("QLabel { color: #FF8C00; margin: 15px; }"); // Amber/orange for non-diatonic
    } else {
        romanNumeralLabel->setStyleSheet("QLabel { color: #4682B4; margin: 15px; }"); // Blue for diatonic
    }
}

void UIManager::addMidiLogEntry(const QString& entry) {
    midiLogEntries.push_back(entry);
    
    // Keep only the most recent entries
    while (midiLogEntries.size() > MAX_LOG_ENTRIES) {
        midiLogEntries.pop_front();
    }
    
    updateMidiLogDisplay();
}

void UIManager::updateMidiLogDisplay() {
    QString logText;
    for (const QString& entry : midiLogEntries) {
        logText += entry + "\n";
    }
    
    midiLogDisplay->setPlainText(logText);
    
    // Scroll to bottom to show newest entries
    QTextCursor cursor = midiLogDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    midiLogDisplay->setTextCursor(cursor);
}

void UIManager::clearDisplays() {
    noteLabel->setText("Press keys");
    noteLabel->setStyleSheet("QLabel { color: #2E8B57; margin: 15px; }");
    chordLabel->setText("");
    romanNumeralLabel->setText("");
}

int UIManager::getCurrentKeySignatureIndex() const {
    return keySignatureCombo->currentIndex();
}

void UIManager::onKeySignatureChanged() {
    emit keySignatureChanged(keySignatureCombo->currentIndex());
}

void UIManager::clearDisplayDelayed() {
    clearDisplays();
}