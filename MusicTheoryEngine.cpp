#include "MusicTheoryEngine.h"
#include <set>

MusicTheoryEngine& MusicTheoryEngine::instance() {
    static MusicTheoryEngine instance;
    return instance;
}

MusicTheoryEngine::MusicTheoryEngine() {
    initializeKeySignatures();
    initializeChordPatterns();
}

void MusicTheoryEngine::initializeKeySignatures() {
    keySignatures = {
        // Major keys (following circle of fifths)
        {"C Major", {}, {}, 0, true},
        {"G Major", {6}, {}, 7, true},        // F#
        {"D Major", {6, 1}, {}, 2, true},     // F#, C#
        {"A Major", {6, 1, 8}, {}, 9, true},  // F#, C#, G#
        {"E Major", {6, 1, 8, 3}, {}, 4, true}, // F#, C#, G#, D#
        {"B Major", {6, 1, 8, 3, 10}, {}, 11, true}, // F#, C#, G#, D#, A#
        {"F# Major", {6, 1, 8, 3, 10, 5}, {}, 6, true}, // F#, C#, G#, D#, A#, E#
        {"C# Major", {0, 6, 1, 8, 3, 10, 5}, {}, 1, true}, // All sharps
        
        {"F Major", {}, {10}, 5, true},       // Bb
        {"B♭ Major", {}, {10, 3}, 10, true},  // Bb, Eb
        {"E♭ Major", {}, {10, 3, 8}, 3, true}, // Bb, Eb, Ab
        {"A♭ Major", {}, {10, 3, 8, 1}, 8, true}, // Bb, Eb, Ab, Db
        {"D♭ Major", {}, {10, 3, 8, 1, 6}, 1, true}, // Bb, Eb, Ab, Db, Gb
        {"G♭ Major", {}, {10, 3, 8, 1, 6, 11}, 6, true}, // Bb, Eb, Ab, Db, Gb, Cb
        {"C♭ Major", {}, {0, 10, 3, 8, 1, 6, 11}, 11, true}, // All flats
        
        // Minor keys
        {"A minor", {}, {}, 9, false},
        {"E minor", {6}, {}, 4, false},       // F#
        {"B minor", {6, 1}, {}, 11, false},   // F#, C#
        {"F# minor", {6, 1, 8}, {}, 6, false}, // F#, C#, G#
        {"C# minor", {6, 1, 8, 3}, {}, 1, false}, // F#, C#, G#, D#
        {"G# minor", {6, 1, 8, 3, 10}, {}, 8, false}, // F#, C#, G#, D#, A#
        {"D# minor", {6, 1, 8, 3, 10, 5}, {}, 3, false}, // F#, C#, G#, D#, A#, E#
        {"A# minor", {0, 6, 1, 8, 3, 10, 5}, {}, 10, false}, // All sharps
        
        {"D minor", {}, {10}, 2, false},      // Bb
        {"G minor", {}, {10, 3}, 7, false},   // Bb, Eb
        {"C minor", {}, {10, 3, 8}, 0, false}, // Bb, Eb, Ab
        {"F minor", {}, {10, 3, 8, 1}, 5, false}, // Bb, Eb, Ab, Db
        {"B♭ minor", {}, {10, 3, 8, 1, 6}, 10, false}, // Bb, Eb, Ab, Db, Gb
        {"E♭ minor", {}, {10, 3, 8, 1, 6, 11}, 3, false}, // Bb, Eb, Ab, Db, Gb, Cb
        {"A♭ minor", {}, {0, 10, 3, 8, 1, 6, 11}, 8, false}, // All flats
    };
}

void MusicTheoryEngine::initializeChordPatterns() {
    // Major chords
    chordPatterns["maj"] = {0, 4, 7};
    chordPatterns["maj7"] = {0, 4, 7, 11};
    chordPatterns["maj9"] = {0, 4, 7, 11, 14};
    chordPatterns["6"] = {0, 4, 7, 9};
    chordPatterns["add9"] = {0, 4, 7, 14};
    
    // Minor chords
    chordPatterns["m"] = {0, 3, 7};
    chordPatterns["m7"] = {0, 3, 7, 10};
    chordPatterns["m9"] = {0, 3, 7, 10, 14};
    chordPatterns["m6"] = {0, 3, 7, 9};
    chordPatterns["mMaj7"] = {0, 3, 7, 11};
    
    // Dominant chords
    chordPatterns["7"] = {0, 4, 7, 10};
    chordPatterns["9"] = {0, 4, 7, 10, 14};
    chordPatterns["11"] = {0, 4, 7, 10, 14, 17};
    chordPatterns["13"] = {0, 4, 7, 10, 14, 17, 21};
    
    // Diminished chords
    chordPatterns["dim"] = {0, 3, 6};
    chordPatterns["dim7"] = {0, 3, 6, 9};
    chordPatterns["ø7"] = {0, 3, 6, 10};
    
    // Augmented chords
    chordPatterns["aug"] = {0, 4, 8};
    chordPatterns["aug7"] = {0, 4, 8, 10};
    
    // Suspended chords
    chordPatterns["sus2"] = {0, 2, 7};
    chordPatterns["sus4"] = {0, 5, 7};
    chordPatterns["7sus2"] = {0, 2, 7, 10};
    chordPatterns["7sus4"] = {0, 5, 7, 10};
    
    // Altered chords
    chordPatterns["7♭­­5"] = {0, 4, 6, 10};
    chordPatterns["7#5"] = {0, 4, 8, 10};
    chordPatterns["7♭9"] = {0, 4, 7, 10, 13};
    chordPatterns["7#9"] = {0, 4, 7, 10, 15};
    chordPatterns["7#11"] = {0, 4, 7, 10, 18};
}

const std::vector<MusicTypes::KeySignature>& MusicTheoryEngine::getKeySignatures() const {
    return keySignatures;
}

const MusicTypes::KeySignature& MusicTheoryEngine::getKeySignature(int index) const {
    if (index < 0 || index >= static_cast<int>(keySignatures.size())) {
        return keySignatures[0]; // Return C Major as default
    }
    return keySignatures[index];
}

int MusicTheoryEngine::getKeySignatureCount() const {
    return static_cast<int>(keySignatures.size());
}

QString MusicTheoryEngine::midiNoteToNoteNameInKey(int midiNote, const MusicTypes::KeySignature& key) const {
    const QString sharpNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const QString flatNames[] = {"C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B"};
    
    int noteClass = midiNote % 12;
    int octave = (midiNote / 12) - 1;
    
    QString noteName;
    
    // Check if this note should be sharp in the current key
    for (int sharpNote : key.sharps) {
        if (noteClass == sharpNote) {
            noteName = sharpNames[noteClass];
            break;
        }
    }
    
    // Check if this note should be flat in the current key  
    if (noteName.isEmpty()) {
        for (int flatNote : key.flats) {
            if (noteClass == flatNote) {
                noteName = flatNames[noteClass];
                break;
            }
        }
    }
    
    // If not specified by key signature, use default (prefer sharps)
    if (noteName.isEmpty()) {
        noteName = sharpNames[noteClass];
    }
    
    return noteName + QString::number(octave);
}

QString MusicTheoryEngine::midiNoteToNoteName(int midiNote) const {
    const QString noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    int noteIndex = midiNote % 12;
    int octave = (midiNote / 12) - 1;
    
    return noteNames[noteIndex] + QString::number(octave);
}

int MusicTheoryEngine::getScaleDegree(int noteClass, const MusicTypes::KeySignature& key) const {
    // Calculate the scale degree (1-7) from the tonic
    int degree = (noteClass - key.tonic + 12) % 12;
    
    // Convert chromatic steps to diatonic scale degrees
    const int majorScaleDegrees[] = {1, -1, 2, -1, 3, 4, -1, 5, -1, 6, -1, 7}; // -1 means not in scale
    const int minorScaleDegrees[] = {1, -1, 2, 3, -1, 4, -1, 5, 6, -1, 7, -1}; // Natural minor
    
    if (key.isMajor) {
        return majorScaleDegrees[degree];
    } else {
        return minorScaleDegrees[degree];
    }
}

QString MusicTheoryEngine::getFunctionName(int scaleDegree, const MusicTypes::KeySignature& key) const {
    const QString majorFunctions[] = {"", "Tonic", "Supertonic", "Mediant", "Subdominant", "Dominant", "Submediant", "Leading Tone"};
    const QString minorFunctions[] = {"", "Tonic", "Supertonic", "Mediant", "Subdominant", "Dominant", "Submediant", "Subtonic"};
    
    if (scaleDegree < 1 || scaleDegree > 7) {
        return "Non-diatonic";
    }
    
    if (key.isMajor) {
        return majorFunctions[scaleDegree];
    } else {
        return minorFunctions[scaleDegree];
    }
}

QString MusicTheoryEngine::getRomanNumeralForScaleDegree(int scaleDegree, const MusicTypes::KeySignature& key, const std::string& chordQuality) const {
    if (scaleDegree < 1 || scaleDegree > 7) {
        return ""; // Will be handled in chord analyzer
    }
    
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
        
        // Adjust for chord quality
        if (chordQuality.find("m") != std::string::npos && (scaleDegree == 1 || scaleDegree == 4 || scaleDegree == 5)) {
            romanNumerals[scaleDegree] = romanNumerals[scaleDegree].toLower();
        }
        if (chordQuality.find("maj") != std::string::npos && (scaleDegree == 2 || scaleDegree == 3 || scaleDegree == 6)) {
            romanNumerals[scaleDegree] = romanNumerals[scaleDegree].toUpper();
        }
    } else {
        // Minor key: i, ii°, ♭III, iv, v, ♭VI, ♭VII
        romanNumerals[1] = "i";
        romanNumerals[2] = "ii°";
        romanNumerals[3] = "♭III";
        romanNumerals[4] = "iv";
        romanNumerals[5] = "v";
        romanNumerals[6] = "♭VI";
        romanNumerals[7] = "♭VII";
        
        // Adjust for chord quality
        if (chordQuality.find("maj") != std::string::npos && scaleDegree == 5) {
            romanNumerals[5] = "V";
        }
        if (chordQuality.find("dim") == std::string::npos && scaleDegree == 2) {
            romanNumerals[2] = "ii";
        }
    }
    
    return romanNumerals[scaleDegree];
}

const std::map<std::string, std::vector<int>>& MusicTheoryEngine::getChordPatterns() const {
    return chordPatterns;
}

bool MusicTheoryEngine::isChordDiatonic(int rootNoteClass, const std::string& chordQuality, const MusicTypes::KeySignature& key) const {
    int scaleDegree = getScaleDegree(rootNoteClass, key);
    if (scaleDegree == -1) return false; // Root not in scale
    
    // Check if the chord quality matches what's expected for this scale degree
    if (key.isMajor) {
        // Major scale: I, ii, iii, IV, V, vi, vii°
        switch (scaleDegree) {
            case 1: case 4: case 5: // I, IV, V should be major
                return chordQuality == "maj" || chordQuality == "7" || chordQuality == "maj7" || 
                       chordQuality == "9" || chordQuality == "6" || chordQuality == "add9";
            case 2: case 3: case 6: // ii, iii, vi should be minor
                return chordQuality == "m" || chordQuality == "m7" || chordQuality == "m9" || 
                       chordQuality == "m6" || chordQuality == "mMaj7";
            case 7: // vii should be diminished
                return chordQuality == "dim" || chordQuality == "dim7" || chordQuality == "ø7";
        }
    } else {
        // Minor scale: i, ii°, ♭III, iv, v, ♭VI, ♭VII
        switch (scaleDegree) {
            case 1: case 4: case 5: // i, iv, v should be minor (but V can be major)
                return chordQuality == "m" || chordQuality == "m7" || chordQuality == "m9" ||
                       chordQuality == "m6" || chordQuality == "mMaj7" || 
                       (scaleDegree == 5 && (chordQuality == "maj" || chordQuality == "7"));
            case 3: case 6: case 7: // ♭III, ♭VI, ♭VII should be major
                return chordQuality == "maj" || chordQuality == "7" || chordQuality == "maj7" || 
                       chordQuality == "9" || chordQuality == "6" || chordQuality == "add9";
            case 2: // ii° should be diminished
                return chordQuality == "dim" || chordQuality == "dim7" || chordQuality == "ø7";
        }
    }
    
    return false;
}

std::vector<int> MusicTheoryEngine::findAccidentalNotes(const std::vector<int>& notes, const MusicTypes::KeySignature& key) const {
    std::vector<int> accidentals;
    
    // Define the diatonic notes for this key
    std::set<int> diatonicNotes;
    if (key.isMajor) {
        const int majorScaleSteps[] = {0, 2, 4, 5, 7, 9, 11};
        for (int step : majorScaleSteps) {
            diatonicNotes.insert((key.tonic + step) % 12);
        }
    } else {
        const int minorScaleSteps[] = {0, 2, 3, 5, 7, 8, 10}; // Natural minor
        diatonicNotes.insert((key.tonic + 7) % 12); // Also allow major V (raised 7th)
        for (int step : minorScaleSteps) {
            diatonicNotes.insert((key.tonic + step) % 12);
        }
    }
    
    // Check each note
    for (int note : notes) {
        int noteClass = note % 12;
        if (diatonicNotes.find(noteClass) == diatonicNotes.end()) {
            accidentals.push_back(note);
        }
    }
    
    return accidentals;
}