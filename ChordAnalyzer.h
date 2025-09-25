#pragma once

#include "MusicTypes.h"
#include "MusicTheoryEngine.h"
#include <QString>
#include <vector>
#include <set>

class ChordAnalyzer {
public:
    explicit ChordAnalyzer(MusicTheoryEngine* theoryEngine);
    
    // Main analysis functions
    QString analyzeNotes(const std::set<int>& activeNotes, const MusicTypes::KeySignature& key);
    MusicTypes::ChordAnalysis analyzeChord(const std::vector<int>& notes, const MusicTypes::KeySignature& key);
    
    // Interval analysis
    QString analyzeInterval(int note1, int note2, const MusicTypes::KeySignature& key);
    
private:
    MusicTheoryEngine* theoryEngine;
    
    // Helper methods
    QString findBestChordInterpretation(const std::vector<int>& notes, const MusicTypes::KeySignature& key, 
                                       std::string& outChordQuality, int& outRootNote);
    QString calculateInversionFigure(const std::string& chordQuality, int bassNote, int rootNote);
    QString detectSecondaryDominant(int rootNoteClass, const std::string& chordQuality, const MusicTypes::KeySignature& key);
    QString getRomanNumeralForDiatonicChord(int scaleDegree, const std::string& chordQuality, const MusicTypes::KeySignature& key);
    
    // Chord quality matching
    std::string matchChordPattern(const std::vector<int>& intervals);
};