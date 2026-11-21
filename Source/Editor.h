#pragma once
#include "Processor.h"
#include "VfdDisplay.h"

class CrowdLook final:public juce::LookAndFeel_V4 {
public:
    int skin=0;
    juce::Colour ink() const{return skin==1?juce::Colour(0xffe6decb):juce::Colour(0xff302b24);}
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
};
class CrowdEditor final:public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit CrowdEditor(CrowdProcessor&);
    ~CrowdEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override{if(auto* peer=getPeer())peer->setCurrentRenderingEngine(0);}
    void visibilityChanged() override{if(auto* peer=getPeer())peer->setCurrentRenderingEngine(0);}
    std::function<void()> onDevices;
private:
    void timerCallback() override;
    CrowdProcessor& p;CrowdLook look;
    std::array<juce::Slider,ControlCount> knobs;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,ControlCount> attachments;
    std::array<VfdLine,ControlCount> values;
    std::array<juce::Rectangle<int>,ControlCount> cells;
    VfdLine status,presetLine,skinLine,finishLine;
    std::array<float,ControlCount> presetValues{};
    juce::TextButton erupt{"BIG PLAY"},play{""},stop{""},preset{"SCENE"},skinButton{"FINISH"},freeButton{"FOLLOW HOST"},device{"AUDIO"};
    float level=0;int presetIndex=0;uint32_t triggerOff=0;
    void pulse();
};
