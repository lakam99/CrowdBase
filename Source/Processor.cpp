#include "Processor.h"
#include "Editor.h"
#include "BinaryData.h"

namespace {
struct Bank {
    juce::AudioBuffer<float> samples;
    Bank(){juce::WavAudioFormat fmt;std::unique_ptr<juce::AudioFormatReader> reader(fmt.createReaderFor(new juce::MemoryInputStream(BinaryData::phonemes_wav,BinaryData::phonemes_wavSize,false),true));
        if(!reader||reader->sampleRate!=24000)throw std::runtime_error("Crowd voice atlas could not be loaded");
        samples.setSize(2,(int)reader->lengthInSamples);reader->read(&samples,0,samples.getNumSamples(),0,true,true);}
};
const Bank& bank(){static const Bank b;return b;}
}
juce::AudioProcessorValueTreeState::ParameterLayout CrowdProcessor::layout(){
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto addFloat=[&](int i){
        juce::NormalisableRange<float> range(controlMin(i),controlMax(i),(i==Settle||i==Output||i==TimeToPeak)?.01f:.001f);
        if(i==TimeToPeak)range.setSkewForCentre(.75f);
        l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{controlIds[i],1},controlNames[i],range,defaults[i]));
    };
    for(int i=0;i<TimeToPeak;++i)addFloat(i);
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"erupt",1},"Big Play Trigger",false));
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"free",1},"Free Run",false));
    // Append after the legacy controls so existing host parameter indices hold.
    addFloat(TimeToPeak);return l;
}
CrowdProcessor::CrowdProcessor():AudioProcessor(BusesProperties().withOutput("Crowd",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"CrowdBase",layout()){
    (void)bank();seed.store((uint64_t)juce::Random::getSystemRandom().nextInt64()|1ULL);for(int i=0;i<ControlCount;++i)params[(size_t)i]=state.getRawParameterValue(controlIds[i]);
    eruptParam=state.getRawParameterValue("erupt");freeParam=state.getRawParameterValue("free");
}
void CrowdProcessor::prepareToPlay(double rate,int){auto& b=bank();engine.prepare(rate,b.samples.getReadPointer(0),b.samples.getReadPointer(1),b.samples.getNumSamples(),seed.load());pendingSeed.store(0);triggerWasHigh=eruptParam->load()>.5f;}
void CrowdProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
    juce::ScopedNoDenormals noDenormals;buffer.clear();if(buffer.getNumChannels()<2)return;
    bool run=freeParam->load()>.5f;
    if(standalone.load())run=audition.load();
    else if(auto* head=getPlayHead()){if(auto info=head->getPosition())run=run||info->getIsPlaying();}
    playing.store(run);
    if(auto next=pendingSeed.exchange(0))engine.reseed(next);
    std::array<float,ControlCount> v;for(int i=0;i<ControlCount;++i)v[(size_t)i]=params[(size_t)i]->load(std::memory_order_relaxed);
    engine.setTargets(v,run);
    bool high=eruptParam->load()>.5f;
    if(suppressRestoredTrigger.exchange(false))triggerWasHigh=high;
    const bool manual=manualTrigger.exchange(false);
    if((high&&!triggerWasHigh)||manual)engine.trigger();triggerWasHigh=high;
    int cursor=0;
    for(const auto metadata:midi){if(metadata.numBytes>=3&&(metadata.data[0]&0xf0)==0x90&&metadata.data[1]==36&&metadata.data[2]!=0){int at=juce::jlimit(cursor,buffer.getNumSamples(),metadata.samplePosition);engine.render(buffer.getWritePointer(0)+cursor,buffer.getWritePointer(1)+cursor,at-cursor);cursor=at;engine.trigger();}}
    engine.render(buffer.getWritePointer(0)+cursor,buffer.getWritePointer(1)+cursor,buffer.getNumSamples()-cursor);
    float peak=std::max(buffer.getMagnitude(0,buffer.getNumSamples()),meter.load()*.90f);meter.store(peak);
    energyMeter.store(engine.currentEnergy());reactionMeter.store(engine.reactionAmount());voiceMeter.store(engine.activeVoices());midi.clear();
}
juce::AudioProcessorEditor* CrowdProcessor::createEditor(){return new CrowdEditor(*this);}
void CrowdProcessor::getStateInformation(juce::MemoryBlock& dest){auto tree=state.copyState();tree.setProperty("skin",skin.load(),nullptr);tree.setProperty("seed",juce::String::toHexString((juce::int64)seed.load()),nullptr);tree.setProperty("formatVersion",2,nullptr);if(auto xml=tree.createXml())copyXmlToBinary(*xml,dest);}
void CrowdProcessor::setStateInformation(const void* bytes,int length){
    if(length<=0||length>1024*1024)return;
    if(auto xml=getXmlFromBinary(bytes,length)){
        auto tree=juce::ValueTree::fromXml(*xml);
        if(tree.hasType("CrowdBase")){
            skin.store(juce::jlimit(0,2,(int)tree.getProperty("skin",0)));
            auto next=(uint64_t)tree.getProperty("seed","1").toString().getHexValue64()|1ULL;
            seed.store(next);pendingSeed.store(next);suppressRestoredTrigger.store(true);
            // Old sessions have no rise setting; restore the fast default even
            // when loading into an instance whose slider was already changed.
            if(!tree.getChildWithProperty("id","timeToPeak").isValid()){
                juce::ValueTree added("PARAM");added.setProperty("id","timeToPeak",nullptr);
                added.setProperty("value",defaults[TimeToPeak],nullptr);tree.appendChild(added,nullptr);
            }
            state.replaceState(tree);
        }
    }
}
void CrowdProcessor::applyPreset(int p){
    std::array<float,ControlCount> v=std::to_array(defaults);
    if(p==1){v[Energy]=.44f;v[Population]=.65f;v[Applause]=.55f;v[Whistles]=.2f;}
    if(p==2){v[Energy]=.88f;v[Population]=.86f;v[Chatter]=.4f;v[Cheers]=.9f;v[Applause]=.7f;v[Whistles]=.24f;}
    if(p==3){v[Energy]=.12f;v[Population]=.3f;v[Distance]=.26f;v[Space]=.18f;v[Whistles]=.04f;}
    for(int i=0;i<ControlCount;++i)if(auto* param=state.getParameter(controlIds[i])){param->beginChangeGesture();param->setValueNotifyingHost(param->convertTo0to1(v[(size_t)i]));param->endChangeGesture();}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new CrowdProcessor;}
