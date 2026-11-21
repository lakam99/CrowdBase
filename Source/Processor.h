#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"

class CrowdProcessor final : public juce::AudioProcessor {
public:
    CrowdProcessor();
    const juce::String getName() const override{return "CrowdBase";}
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& b) const override{return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()&&b.getMainInputChannelSet().isDisabled();}
    bool acceptsMidi() const override{return true;}
    bool producesMidi() const override{return false;}
    double getTailLengthSeconds() const override{return 2.;}
    bool hasEditor() const override{return true;}
    juce::AudioProcessorEditor* createEditor() override;
    int getNumPrograms() override{return 1;}
    int getCurrentProgram() override{return 0;}
    void setCurrentProgram(int) override{}
    const juce::String getProgramName(int) override{return "Continuous crowd";}
    void changeProgramName(int,const juce::String&) override{}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorValueTreeState state;
    std::atomic<bool> standalone{false},audition{false},manualTrigger{false};
    std::atomic<float> meter{0},energyMeter{0},reactionMeter{0};
    std::atomic<int> voiceMeter{0};
    std::atomic<bool> playing{false};
    std::atomic<int> skin{0};
    void applyPreset(int);
    CrowdEngine& testEngine(){return engine;}
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    std::array<std::atomic<float>*,ControlCount> params{};
    std::atomic<float>* eruptParam=nullptr,*freeParam=nullptr;
    CrowdEngine engine;
    bool triggerWasHigh=false;
    std::atomic<bool> suppressRestoredTrigger{false};
    std::atomic<uint64_t> seed{1},pendingSeed{0};
};
