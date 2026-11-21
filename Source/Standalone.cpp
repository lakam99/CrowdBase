#include "Processor.h"
#include "Editor.h"
#include "Tests.h"
#if JUCE_WINDOWS
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#endif

class MainPanel final:public juce::AudioAppComponent {
public:
    CrowdProcessor processor;std::unique_ptr<CrowdEditor> editor;std::unique_ptr<juce::PropertiesFile> prefs;
    MainPanel(){
        processor.standalone.store(true);juce::PropertiesFile::Options options;options.applicationName="CrowdBase";options.filenameSuffix="settings";options.folderName="CrowdBase";options.osxLibrarySubFolder="Application Support";prefs=std::make_unique<juce::PropertiesFile>(options);
        juce::MemoryBlock state;if(state.fromBase64Encoding(prefs->getValue("state")))processor.setStateInformation(state.getData(),(int)state.getSize());
        editor=std::make_unique<CrowdEditor>(processor);editor->onDevices=[this]{auto selector=std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager,0,0,2,2,false,false,true,false);selector->setSize(480,420);juce::DialogWindow::LaunchOptions opt;opt.content.setOwned(selector.release());opt.dialogTitle="CrowdBase audio output";opt.dialogBackgroundColour=juce::Colours::darkgrey;opt.useNativeTitleBar=true;opt.escapeKeyTriggersCloseButton=true;opt.launchAsync();};addAndMakeVisible(*editor);setSize(editor->getWidth(),editor->getHeight());setAudioChannels(0,2);
    }
    ~MainPanel() override {shutdownAudio();juce::MemoryBlock state;processor.getStateInformation(state);prefs->setValue("state",state.toBase64Encoding());prefs->saveIfNeeded();editor.reset();}
    void prepareToPlay(int size,double sr) override{processor.prepareToPlay(sr,size);}
    void releaseResources() override{processor.releaseResources();}
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override{juce::AudioBuffer<float> view(info.buffer->getArrayOfWritePointers(),std::min(2,info.buffer->getNumChannels()),info.startSample,info.numSamples);juce::MidiBuffer midi;processor.processBlock(view,midi);}
    void resized() override{if(editor)editor->setBounds(getLocalBounds());}
};
class Window final:public juce::DocumentWindow,private juce::DarkModeSettingListener {
public:
    Window():DocumentWindow("CrowdBase",juce::Colours::darkgrey,allButtons){setUsingNativeTitleBar(true);setContentOwned(new MainPanel,true);setIcon(RasterAssets::get().icon);centreWithSize(getWidth(),getHeight());setVisible(true);juce::Desktop::getInstance().addDarkModeSettingListener(this);darkModeSettingChanged();}
    ~Window() override{juce::Desktop::getInstance().removeDarkModeSettingListener(this);clearContentComponent();}
    void closeButtonPressed() override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}
    void darkModeSettingChanged() override{bool dark=juce::Desktop::getInstance().isDarkModeActive();setBackgroundColour(dark?juce::Colour(0xff25282b):juce::Colour(0xffeeeeee));
#if JUCE_WINDOWS
        if(auto* peer=getPeer()){BOOL enabled=dark;DwmSetWindowAttribute((HWND)peer->getNativeHandle(),20,&enabled,sizeof(enabled));}
#endif
    }
};
class App final:public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override{return "CrowdBase";}
    const juce::String getApplicationVersion() override{return "0.1.0";}
    bool moreThanOneInstanceAllowed() override{return true;}
    std::unique_ptr<Window> window;
    void initialise(const juce::String& command) override{
        auto args=juce::StringArray::fromTokens(command,true);args.trim();args.removeEmptyStrings();for(auto& a:args)a=a.unquoted();
        auto value=[&](const juce::String& key){int at=args.indexOf(key);return at>=0&&at+1<args.size()?juce::File::getCurrentWorkingDirectory().getChildFile(args[at+1]):juce::File{};};
        try {
            if(args.contains("--test")){auto report=runTests();value("--test").replaceWithText(juce::JSON::toString(report));setApplicationReturnValue((bool)report["passed"]?0:1);quit();return;}
            if(args.contains("--host")){auto report=testHosted(value("--host"),value("--render"),value("--snapshot"));value("--report").replaceWithText(juce::JSON::toString(report));setApplicationReturnValue((bool)report["passed"]?0:1);quit();return;}
            if(args.contains("--render")){CrowdProcessor p;renderDemo(p,value("--render"),false);quit();return;}
            if(args.contains("--snapshot")){CrowdProcessor p;p.standalone.store(true);int skinAt=args.indexOf("--skin");if(skinAt>=0&&skinAt+1<args.size())p.skin.store(juce::jlimit(0,2,args[skinAt+1].getIntValue()));std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());auto image=e->createComponentSnapshot(e->getLocalBounds(),true,1);juce::MemoryOutputStream bytes;juce::PNGImageFormat png;if(!png.writeImageToStream(image,bytes)||!value("--snapshot").replaceWithData(bytes.getData(),bytes.getDataSize()))throw std::runtime_error("Snapshot failed");quit();return;}
            window=std::make_unique<Window>();
        }catch(const std::exception& e){auto error=juce::File::getCurrentWorkingDirectory().getChildFile("crowdbase-error.txt");error.replaceWithText(e.what());setApplicationReturnValue(2);quit();}
    }
    void shutdown() override{window.reset();}
};
START_JUCE_APPLICATION(App)
