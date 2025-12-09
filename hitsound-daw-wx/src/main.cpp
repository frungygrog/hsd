#include <wx/wx.h>
#include "ui/MainFrame.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

class MyApp : public wxApp
{
public:
    bool OnInit() override
    {
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
