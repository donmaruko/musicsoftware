#include <QApplication>
#include "MidiKeyboardMonitor.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    std::cout << "Starting MIDI Keyboard Monitor..." << std::endl;
    
    MidiKeyboardMonitor window;
    window.show();
    
    return app.exec();
}