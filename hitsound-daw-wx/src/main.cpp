#include <wx/wx.h>
#include "ui/MainFrame.h"

// JUCE typically requires a ScopedJuceInitialiser_GUI if we use any GUI related juce classes.
// Even for audio, it might be safer to ensure JUCE is initialised.
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

class MyApp : public wxApp
{
public:
    bool OnInit() override
    {
        // Initialise JUCE
        juce::initialiseJuce_GUI();

        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
    
    int OnExit() override
    {
        juce::shutdownJuce_GUI();
        return wxApp::OnExit();
    }
};

wxIMPLEMENT_APP(MyApp);
