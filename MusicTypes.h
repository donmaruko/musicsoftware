#pragma once

#include <QString>
#include <vector>
#include <string>

namespace MusicTypes {

struct KeySignature {
    std::string name;
    std::vector<int> sharps;  // MIDI note numbers that should be sharp
    std::vector<int> flats;   // MIDI note numbers that should be flat
    int tonic;                // Root note of the key (0-11)
    bool isMajor;
};

struct ChordAnalysis {
    QString chordName;           // e.g., "Bdim/D"
    QString romanNumeral;        // e.g., "vii°⁶"
    QString functionName;        // e.g., "Leading Tone"
    bool isNonDiatonic;         // true if contains accidentals
    bool isSecondaryDominant;   // true if V/x pattern
    QString secondaryTarget;     // e.g., "V" in "V/V"
    QString inversionFigure;     // e.g., "⁶", "⁶₄", "⁷", "⁶₅"
    std::vector<int> accidentalNotes; // MIDI note numbers of accidentals
    int bassNote;               // MIDI note number of bass (lowest note)
    int rootNote;               // MIDI note number of harmonic root
};

struct MidiMessage {
    double timeStamp;
    std::vector<unsigned char> data;
};

enum class MidiEventType {
    NoteOn,
    NoteOff,
    Unknown
};

struct MidiEvent {
    MidiEventType type;
    int noteNumber;
    int velocity;
    int channel;
};

} // namespace MusicTypes