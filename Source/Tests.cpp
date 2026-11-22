#include "Tests.h"
#include <cmath>

namespace {
struct Clock:juce::AudioPlayHead {
    int64_t sample=0;bool running=true;
    juce::Optional<PositionInfo> getPosition() const override {PositionInfo p;p.setIsPlaying(running);p.setTimeInSamples(sample);p.setTimeInSeconds(sample/48000.);p.setBpm(120);p.setPpqPosition(sample/24000.);return p;}
};
void set(juce::AudioProcessor& p,const juce::String& name,float value,bool normalized=false){for(auto* param:p.getParameters())if(param->getName(128)==name){auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(param);param->setValueNotifyingHost(normalized?value:ranged?ranged->convertTo0to1(value):value);return;}throw std::runtime_error(("Missing parameter: "+name).toStdString());}
float peakNormalised(float seconds){juce::NormalisableRange<float> range(.05f,8.f,.01f);range.setSkewForCentre(.75f);return range.convertTo0to1(seconds);}
void snapshot(juce::AudioProcessor& p,const juce::File& file){std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditorIfNeeded());if(!ed)throw std::runtime_error("Missing editor");juce::DocumentWindow window("CrowdBase hosted editor",juce::Colours::black,0);window.setUsingNativeTitleBar(true);window.setContentNonOwned(ed.get(),true);window.setTopLeftPosition(-4000,-4000);window.setVisible(true);juce::MessageManager::getInstance()->runDispatchLoopUntil(200);auto image=ed->createComponentSnapshot(ed->getLocalBounds(),true,1);file.getParentDirectory().createDirectory();juce::MemoryOutputStream bytes;juce::PNGImageFormat png;if(!png.writeImageToStream(image,bytes)||!file.replaceWithData(bytes.getData(),bytes.getDataSize()))throw std::runtime_error("Screenshot failed");window.setVisible(false);window.clearContentComponent();ed.reset();juce::MessageManager::getInstance()->runDispatchLoopUntil(200);}

// Locate the actual envelope maximum, rather than the loudest random crowd
// sample. Observe every sample near the peak while varying render block sizes.
bool peakTiming(double sr,int blockSize,float seconds,double& worstError){
    CrowdProcessor p;p.prepareToPlay(sr,blockSize);auto& engine=p.testEngine();
    auto controls=std::to_array(defaults);controls[TimeToPeak]=seconds;
    engine.setTargets(controls,true);engine.trigger();
    juce::AudioBuffer<float> block(2,blockSize);int frame=0;
    auto advance=[&](int until){while(frame<until){int n=std::min(blockSize,until-frame);engine.render(block.getWritePointer(0),block.getWritePointer(1),n);frame+=n;}};
    advance((int)(seconds*sr*.5));float half=engine.reactionAmount();
    advance((int)(seconds*sr)-std::max(2,(int)(sr*.004)));
    float maximum=engine.reactionAmount();int peakFrame=frame;
    int end=(int)(seconds*sr)+std::max(2,(int)(sr*.004));
    while(frame<end){engine.render(block.getWritePointer(0),block.getWritePointer(1),1);++frame;
        // A float may round to 1 just before the exact peak; keep the last tie.
        if(engine.reactionAmount()>=maximum){maximum=engine.reactionAmount();peakFrame=frame;}}
    double error=std::abs(peakFrame-seconds*sr);worstError=std::max(worstError,error);
    return error<=2&&maximum>.99995f&&std::abs(half-.5f)<.001f&&engine.reactionAmount()<maximum;
}

bool latchedRiseAndRetrigger(){
    CrowdProcessor p;p.prepareToPlay(48000,128);auto& engine=p.testEngine();
    auto controls=std::to_array(defaults);controls[TimeToPeak]=1.f;
    engine.setTargets(controls,true);engine.trigger();juce::AudioBuffer<float> b(2,128);
    auto advance=[&](int n){while(n>0){int count=std::min(n,128);engine.render(b.getWritePointer(0),b.getWritePointer(1),count);n-=count;}};
    advance(12000);float quarter=engine.reactionAmount();
    controls[TimeToPeak]=.05f;engine.setTargets(controls,true);advance(12000);
    bool latched=std::abs(quarter-.15625f)<.001f&&std::abs(engine.reactionAmount()-.5f)<.001f;
    float before=engine.reactionAmount();engine.trigger();advance(1);
    bool continuous=engine.reactionAmount()>=before&&engine.reactionAmount()-before<.0001f;
    advance(2399);bool nextDuration=engine.reactionAmount()>.99995f;
    return latched&&continuous&&nextDuration;
}

bool legacyStateDefaults(){
    CrowdProcessor source;set(source,"Energy",.317f);auto tree=source.state.copyState();
    for(int i=tree.getNumChildren()-1;i>=0;--i)if(tree.getChild(i).getProperty("id").toString()=="timeToPeak")tree.removeChild(i,nullptr);
    juce::MemoryBlock old;juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),old);
    CrowdProcessor restored;set(restored,"Time to Peak",7.f);restored.setStateInformation(old.getData(),(int)old.getSize());
    return std::abs(restored.state.getRawParameterValue("timeToPeak")->load()-defaults[TimeToPeak])<.0001f
        &&std::abs(restored.state.getRawParameterValue("energy")->load()-.317f)<.001f;
}

bool legacyParameterOrder(){
    CrowdProcessor p;auto parameters=p.getParameters();if(parameters.size()!=16)return false;
    const char* expected[]{"energy","population","chatter","cheers","applause","whistles","distance","space","width","tone","variation","settle","output","erupt","free","timeToPeak"};
    for(int i=0;i<16;++i){auto* parameter=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameters[i]);if(!parameter||parameter->paramID!=expected[i])return false;}
    return true;
}

bool midiSampleTiming(double sr){
    CrowdProcessor p;p.standalone.store(true);p.audition.store(true);p.prepareToPlay(sr,128);
    juce::AudioBuffer<float> block(2,128);juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1,36,(juce::uint8)100),64);p.processBlock(block,midi);
    float u=float(64/(sr*defaults[TimeToPeak]));float expected=u*u*(3-2*u);
    return std::abs(p.reactionMeter.load()-expected)<1e-6f&&p.reactionMeter.load()>0&&midi.isEmpty();
}

bool peakSliderAttachment(){
    CrowdProcessor p;std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    if(!editor)return false;juce::Slider* peakSlider=nullptr;
    for(auto* child:editor->getChildren())if(child->getName()=="Time to Peak")peakSlider=dynamic_cast<juce::Slider*>(child);
    if(!peakSlider||peakSlider->getSliderStyle()!=juce::Slider::LinearHorizontal||peakSlider->getBounds().isEmpty()||!editor->getLocalBounds().contains(peakSlider->getBounds()))return false;
    for(auto* child:editor->getChildren())if(child!=peakSlider&&child->isVisible()&&child->getBounds().intersects(peakSlider->getBounds()))return false;
    peakSlider->setValue(2.46,juce::sendNotificationSync);
    bool toParameter=std::abs(p.state.getRawParameterValue("timeToPeak")->load()-2.46f)<.001f;
    set(p,"Time to Peak",.62f);juce::MessageManager::getInstance()->runDispatchLoopUntil(25);
    bool toSlider=std::abs(peakSlider->getValue()-.62)<.001;
    return toParameter&&toSlider;
}
}
void renderDemo(juce::AudioProcessor& p,const juce::File& output,bool transport){
    output.getParentDirectory().createDirectory();juce::WavAudioFormat wav;auto fileStream=output.createOutputStream();if(!fileStream||!fileStream->setPosition(0)||fileStream->truncate().failed())throw std::runtime_error("Could not replace WAV output");std::unique_ptr<juce::OutputStream> stream(std::move(fileStream));auto writer=wav.createWriterFor(stream,juce::AudioFormatWriterOptions{}.withSampleRate(48000).withNumChannels(2).withBitsPerSample(24));if(!writer)throw std::runtime_error("WAV writer failed");
    Clock clock;if(transport)p.setPlayHead(&clock);else if(auto* own=dynamic_cast<CrowdProcessor*>(&p)){own->standalone.store(true);own->audition.store(true);}
    p.setRateAndBufferSizeDetails(48000,256);p.prepareToPlay(48000,256);juce::AudioBuffer<float> block(2,256);juce::MidiBuffer midi;
    for(int at=0;at<48000*60;at+=256){int n=std::min(256,48000*60-at);block.setSize(2,n,false,false,true);clock.sample=at;
        // Ordinary host automation: rise into anticipation, erupt at 30 seconds,
        // then leave the underlying energy exactly where it began.
        float t=at/48000.f;float e=t<24?.18f:t<29?.18f+(t-24)*.045f:t<30?.405f:t<35?.18f:.18f;
        set(p,"Energy",e);set(p,"Big Play Trigger",at>=48000*30&&at<48000*30+512?1:0,true);
        p.processBlock(block,midi);if(!writer->writeFromAudioSampleBuffer(block,0,n))throw std::runtime_error("WAV write failed");
    }
    p.releaseResources();p.setPlayHead(nullptr);
}
juce::var runTests(){
    auto* result=new juce::DynamicObject;bool passed=true;double worstMs=0,totalMs=0,totalAudioMs=0;bool finite=true,stopped=true,variation=true,recall=true,midiWorks=true;float peak=0;
    for(double sr:{44100.,48000.,96000.}){
        CrowdProcessor p;p.standalone.store(true);p.audition.store(true);p.prepareToPlay(sr,128);juce::AudioBuffer<float> block(2,128);juce::MidiBuffer midi;
        double firstEnergy=0,secondEnergy=0,activeEnergy=0,stoppedEnergy=0;
        for(int k=0;k<1400;++k){if(k==300)p.manualTrigger.store(true);if(k==650)p.applyPreset(2);if(k==850){set(p,"Width",0);set(p,"Tone",1);set(p,"Space",1);}if(k==1100)p.audition.store(false);
            if(k==520)midi.addEvent(juce::MidiMessage::noteOn(1,36,(juce::uint8)100),64);
            auto start=juce::Time::getMillisecondCounterHiRes();p.processBlock(block,midi);double ms=juce::Time::getMillisecondCounterHiRes()-start;worstMs=std::max(worstMs,ms);totalMs+=ms;totalAudioMs+=128/sr*1000;
            double energy=0;for(int c=0;c<2;++c)for(int i=0;i<128;++i){float v=block.getSample(c,i);finite&=std::isfinite(v);peak=std::max(peak,std::abs(v));energy+=v*v;}
            if(k>30&&k<130)firstEnergy+=energy;if(k>140&&k<240)secondEnergy+=energy;if(k>350&&k<450)activeEnergy+=energy;if(k>1350)stoppedEnergy+=energy;
        }
        stopped&=stoppedEnergy<1e-10;variation&=firstEnergy>0&&secondEnergy>0&&std::abs(firstEnergy-secondEnergy)>1e-6;passed&=activeEnergy>firstEnergy;
        set(p,"Energy",.717f);set(p,"Settle",13.4f);set(p,"Time to Peak",2.37f);p.skin.store(2);juce::MemoryBlock saved;p.getStateInformation(saved);
        CrowdProcessor restored;restored.setStateInformation(saved.getData(),(int)saved.getSize());recall&=std::abs(restored.state.getRawParameterValue("energy")->load()-.717f)<.001f&&std::abs(restored.state.getRawParameterValue("settle")->load()-13.4f)<.01f&&std::abs(restored.state.getRawParameterValue("timeToPeak")->load()-2.37f)<.001f&&restored.skin.load()==2;
        p.releaseResources();
        midiWorks&=midiSampleTiming(sr);
    }
    // Reaction must settle to the baseline without retriggering a held automation value.
    CrowdProcessor reaction;reaction.standalone.store(true);reaction.audition.store(true);reaction.prepareToPlay(48000,512);juce::AudioBuffer<float> b(2,512);juce::MidiBuffer m;set(reaction,"Big Play Trigger",1,true);
    float onset=0,tail=0;for(int k=0;k<2400;++k){reaction.processBlock(b,m);if(k==20)onset=reaction.reactionMeter.load();if(k==2399)tail=reaction.reactionMeter.load();}
    bool settles=onset>.7f&&tail<.001f;
    // Loading saved high trigger state must not cause a phantom new reaction.
    juce::MemoryBlock high;reaction.getStateInformation(high);CrowdProcessor loaded;loaded.setStateInformation(high.getData(),(int)high.getSize());loaded.standalone.store(true);loaded.audition.store(true);loaded.prepareToPlay(48000,512);loaded.processBlock(b,m);bool noPhantom=loaded.reactionMeter.load()<.001f;
    bool timing=true;double peakErrorSamples=0;int scenario=0;
    for(double sr:{44100.,48000.,96000.})for(float seconds:{.05f,.15f,1.f,8.f})timing&=peakTiming(sr,std::array<int,3>{64,127,512}[(size_t)(scenario++%3)],seconds,peakErrorSamples);
    bool latching=latchedRiseAndRetrigger(),legacyDefaults=legacyStateDefaults(),legacyOrder=legacyParameterOrder(),sliderAttachment=peakSliderAttachment();
    passed&=finite&&peak<1&&stopped&&variation&&recall&&midiWorks&&settles&&noPhantom&&timing&&latching&&legacyDefaults&&legacyOrder&&sliderAttachment;
    result->setProperty("time_to_peak_slider_attachment_and_layout",sliderAttachment);
    result->setProperty("time_to_peak_timing",timing);result->setProperty("time_to_peak_max_error_samples",peakErrorSamples);result->setProperty("time_to_peak_cases",scenario);result->setProperty("active_rise_latched_and_retrigger_continuous",latching);result->setProperty("legacy_state_defaults_time_to_peak",legacyDefaults);result->setProperty("legacy_parameter_indices_preserved",legacyOrder);
    result->setProperty("passed",passed);result->setProperty("sample_rates","44100,48000,96000");result->setProperty("finite_audio",finite);result->setProperty("peak",peak);result->setProperty("transport_fades_to_silence",stopped);result->setProperty("nonrepeating_windows",variation);result->setProperty("parameter_and_skin_recall",recall);result->setProperty("midi_note_36_reaction",midiWorks);result->setProperty("held_trigger_fires_once_and_settles",settles);result->setProperty("restore_does_not_trigger_reaction",noPhantom);result->setProperty("processing_percent_one_core",totalMs/totalAudioMs*100);result->setProperty("worst_measured_callback_ms",worstMs);return juce::var(result);
}
juce::var testHosted(const juce::File& file,const juce::File& audio,const juce::File& image){
    auto stage=[](const char* name){juce::File::getCurrentWorkingDirectory().getChildFile("tests/host-stages.txt").appendText(juce::String(name)+"\n");};stage("discover");
    juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> descriptions;format.findAllTypesForFile(descriptions,file.getFullPathName());if(descriptions.isEmpty())throw std::runtime_error("No VST3 discovered");
    juce::String error;auto plugin=format.createInstanceFromDescription(*descriptions[0],48000,256,error);if(!plugin)throw std::runtime_error(error.toStdString());
    stage("render");renderDemo(*plugin,audio,true);stage("save");juce::MemoryBlock state;set(*plugin,"Energy",.613f);set(*plugin,"Time to Peak",peakNormalised(3.21f),true);juce::AudioBuffer<float> flush(2,256);juce::MidiBuffer empty;plugin->prepareToPlay(48000,256);plugin->processBlock(flush,empty);plugin->getStateInformation(state);stage("remove first");plugin.reset();stage("recreate");
    auto restored=format.createInstanceFromDescription(*descriptions[0],48000,256,error);if(!restored)throw std::runtime_error(error.toStdString());restored->setStateInformation(state.getData(),(int)state.getSize());
    stage("restored");restored->prepareToPlay(48000,256);restored->processBlock(flush,empty);bool recall=false,peakRecall=false;for(auto* q:restored->getParameters()){if(q->getName(128)=="Energy")recall=std::abs(q->getValue()-.613f)<.002f;if(q->getName(128)=="Time to Peak")peakRecall=std::abs(q->getValue()-peakNormalised(3.21f))<.001f;}recall&=peakRecall;
    if(image!=juce::File{}){stage("snapshot");snapshot(*restored,image);stage("snapshot done");}
    Clock clock;clock.running=false;restored->setPlayHead(&clock);stage("prepare restored");restored->prepareToPlay(48000,128);stage("process stopped");juce::AudioBuffer<float> block(2,128);juce::MidiBuffer midi;restored->processBlock(block,midi);stage("processed stopped");bool stopped=block.getMagnitude(0,128)==0;
    clock.running=true;double loud=0;for(int i=0;i<300;++i){clock.sample=i*128;restored->processBlock(block,midi);loud+=block.getRMSLevel(0,0,128);}stage("processed playing");
    clock.running=false;for(int i=0;i<300;++i)restored->processBlock(block,midi);stage("stopped again");stopped&=block.getMagnitude(0,128)==0;restored->setPlayHead(nullptr);restored->releaseResources();stage("remove restored");restored.reset();stage("removed restored");
    block.setSize(2,64);double maxRemove=0;for(int i=0;i<8;++i){auto p=format.createInstanceFromDescription(*descriptions[0],48000,64,error);if(!p)throw std::runtime_error(error.toStdString());p->setPlayHead(&clock);p->prepareToPlay(48000,64);p->processBlock(block,midi);if(i<3&&image!=juce::File{})snapshot(*p,image);auto start=juce::Time::getMillisecondCounterHiRes();p->releaseResources();p.reset();maxRemove=std::max(maxRemove,juce::Time::getMillisecondCounterHiRes()-start);}
    auto* report=new juce::DynamicObject;report->setProperty("passed",recall&&stopped&&loud>0&&maxRemove<500);report->setProperty("actual_vst3_loaded",true);report->setProperty("render_timeline_advanced",true);report->setProperty("parameter_recall_after_recreate",recall);report->setProperty("host_transport_gates_audio",stopped&&loud>0);report->setProperty("editor_snapshot",image.getFullPathName());report->setProperty("remove_recreate_cycles",8);report->setProperty("max_removal_ms",maxRemove);return juce::var(report);
}
