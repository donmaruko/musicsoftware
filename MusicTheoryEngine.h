#pragma once

#include "MusicTypes.h"
#include <QString>
#include <vector>
#include <map>
#include <memory>

class MusicTheoryEngine {
public:
    static MusicTheoryEngine& instance();
    
    // Key signature management
    const std::vector<MusicTypes::KeySignature>& getKeySignatures() const;
    const MusicTypes::KeySignature& getKeySignature(int index) const;
    int getKeySignatureCount() const;
    
    // Note conversion
    QString midiNoteToNoteNameInKey(int midiNote, const MusicTypes::KeySignature& key) const;
    QString midiNoteToNoteName(int midiNote) const;
    
    // Scale and theory analysis
    int getScaleDegree(int noteClass, const MusicTypes::KeySignature& key) const;
    QString getFunctionName(int scaleDegree, const MusicTypes::KeySignature& key) const;
    QString getRomanNumeralForScaleDegree(int scaleDegree, const MusicTypes::KeySignature& key, const std::string& chordQuality) const;
    
    // Chord pattern access
    const std::map<std::string, std::vector<int>>& getChordPatterns() const;
    
    // Utility functions
    bool isChordDiatonic(int rootNoteClass, const std::string& chordQuality, const MusicTypes::KeySignature& key) const;
    std::vector<int> findAccidentalNotes(const std::vector<int>& notes, const MusicTypes::KeySignature& key) const;

private:
    MusicTheoryEngine();
    ~MusicTheoryEngine() = default;
    MusicTheoryEngine(const MusicTheoryEngine&) = delete;
    MusicTheoryEngine& operator=(const MusicTheoryEngine&) = delete;
    
    void initializeKeySignatures();
    void initializeChordPatterns();
    
    std::vector<MusicTypes::KeySignature> keySignatures;
    std::map<std::string, std::vector<int>> chordPatterns;
};