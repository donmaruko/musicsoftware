#include "ChordAnalyzer.h"
#include <algorithm>
#include <iostream>

ChordAnalyzer::ChordAnalyzer(MusicTheoryEngine* theoryEngine)
    : theoryEngine(theoryEngine)
{
}

QString ChordAnalyzer::analyzeNotes(const std::set<int>& activeNotes, const MusicTypes::KeySignature& key) {
    if (activeNotes.empty()) {
        return "";
    }
    
    std::vector<int> notes(activeNotes.begin(), activeNotes.end());
    std::sort(notes.begin(), notes.end());
    
    // Handle single notes - don't show chord analysis
    if (notes.size() == 1) {
        return ""; // Return empty string for single notes
    }
    
    // Handle simple intervals (2 notes)
    if (notes.size() == 2) {
        return analyzeInterval(notes[0], notes[1], key);
    }
    
    // For chords (3+ notes), do comprehensive analysis
    MusicTypes::ChordAnalysis analysis = analyzeChord(notes, key);
    return analysis.chordName;
}

QString ChordAnalyzer::analyzeInterval(int note1, int note2, const MusicTypes::KeySignature& key) {
    int interval = note2 - note1;
    QString rootNote = theoryEngine->midiNoteToNoteNameInKey(note1, key);
    
    switch (interval) {
        case 1: return rootNote + " minor 2nd";
        case 2: return rootNote + " major 2nd";
        case 3: return rootNote + " minor 3rd";
        case 4: return rootNote + " major 3rd";
        case 5: return rootNote + " perfect 4th";
        case 6: return rootNote + " tritone";
        case 7: return rootNote + " perfect 5th";
        case 8: return rootNote + " minor 6th";
        case 9: return rootNote + " major 6th";
        case 10: return rootNote + " minor 7th";
        case 11: return rootNote + " major 7th";
        case 12: return rootNote + " octave";
        default: return rootNote + " +" + QString::number(interval) + " semitones";
    }
}

MusicTypes::ChordAnalysis ChordAnalyzer::analyzeChord(const std::vector<int>& notes, const MusicTypes::KeySignature& key) {
    MusicTypes::ChordAnalysis analysis;
    analysis.bassNote = notes[0]; // Lowest note is bass
    analysis.isNonDiatonic = false;
    analysis.isSecondaryDominant = false;
    
    // Find accidental notes first
    analysis.accidentalNotes = theoryEngine->findAccidentalNotes(notes, key);
    if (!analysis.accidentalNotes.empty()) {
        analysis.isNonDiatonic = true;
    }
    
    // Find best chord interpretation
    std::string bestChordQuality;
    int bestRootNote;
    QString chordName = findBestChordInterpretation(notes, key, bestChordQuality, bestRootNote);
    
    if (bestChordQuality.empty()) {
        // Couldn't identify chord
        analysis.chordName = QString("Cluster (%1 notes)").arg(notes.size());
        analysis.romanNumeral = "?";
        analysis.functionName = "";
        return analysis;
    }
    
    analysis.rootNote = bestRootNote;
    analysis.chordName = chordName;
    
    // Calculate inversion figure
    analysis.inversionFigure = calculateInversionFigure(bestChordQuality, analysis.bassNote, analysis.rootNote);
    
    // Check if chord is diatonic to the key
    int rootNoteClass = analysis.rootNote % 12;
    bool isDiatonic = theoryEngine->isChordDiatonic(rootNoteClass, bestChordQuality, key);
    
    if (!isDiatonic || analysis.isNonDiatonic) {
        analysis.isNonDiatonic = true;
        
        // Check for secondary dominants
        QString secondaryTarget = detectSecondaryDominant(rootNoteClass, bestChordQuality, key);
        if (!secondaryTarget.isEmpty()) {
            analysis.isSecondaryDominant = true;
            analysis.secondaryTarget = secondaryTarget;
            analysis.romanNumeral = "V" + analysis.inversionFigure + "/" + secondaryTarget;
            analysis.functionName = "Secondary Dominant";
        } else {
            // Non-diatonic but not a secondary dominant
            int scaleDegree = theoryEngine->getScaleDegree(rootNoteClass, key);
            if (scaleDegree != -1) {
                QString baseRoman = getRomanNumeralForDiatonicChord(scaleDegree, "", key);
                analysis.romanNumeral = baseRoman + analysis.inversionFigure;
            } else {
                analysis.romanNumeral = "Non-diatonic" + analysis.inversionFigure;
            }
            analysis.functionName = "Non-functional";
        }
    } else {
        // Diatonic chord
        int scaleDegree = theoryEngine->getScaleDegree(rootNoteClass, key);
        if (scaleDegree != -1) {
            QString baseRoman = getRomanNumeralForDiatonicChord(scaleDegree, "", key);
            analysis.romanNumeral = baseRoman + analysis.inversionFigure;
            analysis.functionName = theoryEngine->getFunctionName(scaleDegree, key);
        }
    }
    
    return analysis;
}

QString ChordAnalyzer::findBestChordInterpretation(const std::vector<int>& notes, const MusicTypes::KeySignature& key, 
                                                  std::string& outChordQuality, int& outRootNote) {
    const auto& chordPatterns = theoryEngine->getChordPatterns();
    
    // Try all possible root notes to find the best chord interpretation
    for (int rootNote : notes) {
        std::vector<int> intervals;
        for (int note : notes) {
            int interval = (note - rootNote) % 12;
            if (interval < 0) interval += 12;
            intervals.push_back(interval);
        }
        std::sort(intervals.begin(), intervals.end());
        
        std::string chordQuality = matchChordPattern(intervals);
        
        if (!chordQuality.empty()) {
            outChordQuality = chordQuality;
            outRootNote = rootNote;
            
            QString rootNoteName = theoryEngine->midiNoteToNoteNameInKey(rootNote, key);
            QString chordName = rootNoteName + " " + QString::fromStdString(chordQuality);
            
            // Add slash notation if bass != root
            if (notes[0] != rootNote) {
                QString bassNoteName = theoryEngine->midiNoteToNoteNameInKey(notes[0], key);
                chordName += "/" + bassNoteName;
            }
            
            return chordName;
        }
    }
    
    outChordQuality = "";
    outRootNote = notes[0];
    return "";
}

std::string ChordAnalyzer::matchChordPattern(const std::vector<int>& intervals) {
    const auto& chordPatterns = theoryEngine->getChordPatterns();
    
    for (const auto& pattern : chordPatterns) {
        std::vector<int> chordIntervals = pattern.second;
        std::sort(chordIntervals.begin(), chordIntervals.end());
        
        if (intervals.size() == chordIntervals.size()) {
            bool match = true;
            for (size_t i = 0; i < intervals.size(); i++) {
                if (intervals[i] != chordIntervals[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return pattern.first;
            }
        }
    }
    
    return "";
}

QString ChordAnalyzer::calculateInversionFigure(const std::string& chordQuality, int bassNote, int rootNote) {
    if (bassNote == rootNote) {
        // Root position - add quality indicators only for root position
        if (chordQuality == "dim7") {
            return "°⁷";
        } else if (chordQuality == "ø7") {
            return "ø⁷";
        } else if (chordQuality.find("7") != std::string::npos) {
            return "⁷";
        }
        return ""; // Regular triads and dim triads get no figure in root position
    }
    
    // Calculate the interval from root to bass
    int rootClass = rootNote % 12;
    int bassClass = bassNote % 12;
    int bassInterval = (bassClass - rootClass + 12) % 12;
    
    // Check if bass is the third (major third=4, minor third=3)
    if (bassInterval == 3 || bassInterval == 4) {
        // First inversion - third in bass
        if (chordQuality.find("7") != std::string::npos || 
            chordQuality == "dim7" || chordQuality == "ø7") {
            return "⁶₅"; // Seventh chord first inversion
        } else {
            return "⁶"; // Triad first inversion
        }
    }
    // Check if bass is the fifth (perfect fifth=7, diminished fifth=6 for dim chords)
    else if (bassInterval == 7 || (bassInterval == 6 && chordQuality == "dim")) {
        // Second inversion - fifth in bass
        if (chordQuality.find("7") != std::string::npos || 
            chordQuality == "dim7" || chordQuality == "ø7") {
            return "₄³"; // Seventh chord second inversion
        } else {
            return "₆₄"; // Triad second inversion
        }
    }
    // Check if bass is the seventh (for 7th chords only)
    else if ((bassInterval == 10 || bassInterval == 11 || bassInterval == 9) && 
            (chordQuality.find("7") != std::string::npos || 
             chordQuality == "dim7" || chordQuality == "ø7")) {
        // Third inversion - seventh in bass
        return "₄₂";
    }
    
    return ""; // Unknown inversion
}

QString ChordAnalyzer::detectSecondaryDominant(int rootNoteClass, const std::string& chordQuality, const MusicTypes::KeySignature& key) {
    // Only major and dominant 7th chords can be secondary dominants
    if (!(chordQuality == "maj" || chordQuality == "7" || chordQuality == "9" || chordQuality == "maj7")) {
        return "";
    }
    
    // Check if this chord's root is a perfect fifth above any diatonic scale degree
    const int diatonicDegrees[] = {1, 2, 3, 4, 5, 6, 7};
    
    for (int degree : diatonicDegrees) {
        // Calculate the note for this scale degree
        int scaleNote = key.tonic;
        if (key.isMajor) {
            const int majorScaleSteps[] = {0, 2, 4, 5, 7, 9, 11}; // Semitone steps from tonic
            scaleNote = (key.tonic + majorScaleSteps[degree - 1]) % 12;
        } else {
            const int minorScaleSteps[] = {0, 2, 3, 5, 7, 8, 10}; // Natural minor
            scaleNote = (key.tonic + minorScaleSteps[degree - 1]) % 12;
        }
        
        // Check if rootNoteClass is a perfect fifth above this scale degree
        int expectedDominant = (scaleNote + 7) % 12;
        if (rootNoteClass == expectedDominant) {
            // Found a secondary dominant!
            if (key.isMajor) {
                const QString majorRomanNumerals[] = {"", "I", "ii", "iii", "IV", "V", "vi", "vii°"};
                return majorRomanNumerals[degree];
            } else {
                const QString minorRomanNumerals[] = {"", "i", "ii°", "♭III", "iv", "v", "♭VI", "♭VII"};
                return minorRomanNumerals[degree];
            }
        }
    }
    
    return "";
}

QString ChordAnalyzer::getRomanNumeralForDiatonicChord(int scaleDegree, const std::string& chordQuality, const MusicTypes::KeySignature& key) {
    if (scaleDegree < 1 || scaleDegree > 7) return "?";
    
    QString romanNumerals[8]; // Index 0 unused, 1-7 for scale degrees
    
    if (key.isMajor) {
        // Major key: I, ii, iii, IV, V, vi, vii°
        romanNumerals[1] = "I";
        romanNumerals[2] = "ii";
        romanNumerals[3] = "iii";
        romanNumerals[4] = "IV";
        romanNumerals[5] = "V";
        romanNumerals[6] = "vi";
        romanNumerals[7] = "vii°";
        
        if (chordQuality == "ø7") {
            romanNumerals[7] = "vii"; // Half-diminished gets no ° in Roman numeral
        } else {
            romanNumerals[7] = "vii°"; // Fully diminished gets °
        }
    } else {
        // Minor key: i, ii°, ♭III, iv, v, ♭VI, ♭VII
        romanNumerals[1] = "i";
        if (chordQuality == "ø7") {
            romanNumerals[2] = "ii"; // Half-diminished gets no ° in Roman numeral
        } else {
            romanNumerals[2] = "ii°"; // Fully diminished gets °
        }
        romanNumerals[3] = "♭III";
        romanNumerals[4] = "iv";
        romanNumerals[5] = "v";
        romanNumerals[6] = "♭VI";
        romanNumerals[7] = "♭VII";
        
        // Handle raised 7th (leading tone) creating major V
        if (scaleDegree == 5 && (chordQuality == "maj" || chordQuality == "7")) {
            romanNumerals[5] = "V";
        }
    }
    
    return romanNumerals[scaleDegree];
}