#include "Editor.h"

namespace {
juce::Font font(float size,bool bold=true){return juce::Font(juce::FontOptions("Arial",size,bold?juce::Font::bold:juce::Font::plain));}
void label(juce::Graphics& g,const juce::String& text,juce::Rectangle<int> bounds,juce::Colour ink,float size=11){g.setColour(ink);g.setFont(font(size));g.drawText(text,bounds,juce::Justification::centred);}
}
void CrowdLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider&){
    auto area=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(13);float radius=std::min(area.getWidth(),area.getHeight())*.5f,cx=area.getCentreX(),cy=area.getCentreY();
    for(int i=0;i<=10;++i){float a=start+(end-start)*i/10;g.setColour(ink().withAlpha(.7f));g.drawLine(cx+std::sin(a)*(radius+4),cy-std::cos(a)*(radius+4),cx+std::sin(a)*(radius+9),cy-std::cos(a)*(radius+9),i%5==0?1.6f:.7f);}
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.drawImage(RasterAssets::get().knob,{cx-radius,cy-radius,radius*2,radius*2},juce::RectanglePlacement::stretchToFit);
    float a=start+position*(end-start);g.setColour(juce::Colour(0xfff6efd4));g.drawLine(cx+std::sin(a)*radius*.36f,cy-std::cos(a)*radius*.36f,cx+std::sin(a)*radius*.79f,cy-std::cos(a)*radius*.79f,3.f);
}
void CrowdLook::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down){
    auto r=b.getLocalBounds().toFloat().reduced(1);g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    if(b.getComponentID()=="play"||b.getComponentID()=="stop"){g.drawImage(RasterAssets::get().transport[b.getComponentID()=="play"?0:2],r,juce::RectanglePlacement::centred);return;}
    g.drawImage(RasterAssets::get().buttons[down?1:0],r,juce::RectanglePlacement::stretchToFit);
    if(b.getComponentID()=="erupt"){g.setColour(juce::Colour(0xffb4482c).withAlpha(down?.75f:.58f));g.fillRoundedRectangle(r.reduced(8,6),3);}
    if(over){g.setColour(juce::Colours::white.withAlpha(.05f));g.fillRoundedRectangle(r.reduced(5),3);}
}
void CrowdLook::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool){if(b.getComponentID()=="play"||b.getComponentID()=="stop")return;label(g,b.getButtonText(),b.getLocalBounds(),juce::Colour(0xffffefd3),11);}
CrowdEditor::CrowdEditor(CrowdProcessor& processor):AudioProcessorEditor(processor),p(processor){
    look.skin=p.skin.load();setLookAndFeel(&look);(void)RasterAssets::get();
    for(int i=0;i<ControlCount;++i){presetValues[(size_t)i]=p.state.getRawParameterValue(controlIds[i])->load();if(std::abs(presetValues[(size_t)i]-defaults[i])>.002f)presetIndex=-1;}
    for(int i=0;i<ControlCount;++i){auto& k=knobs[(size_t)i];k.setSliderStyle(juce::Slider::RotaryVerticalDrag);k.setRotaryParameters(juce::MathConstants<float>::pi*1.22f,juce::MathConstants<float>::pi*2.78f,true);k.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);k.setName(controlNames[i]);k.setDoubleClickReturnValue(true,defaults[i]);k.setTooltip(juce::String(controlNames[i])+": drag to adjust. Double-click to reset. Automatable from your host.");addAndMakeVisible(k);attachments[(size_t)i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,controlIds[i],k);}
    auto add=[this](juce::TextButton& b){addAndMakeVisible(b);};add(erupt);add(play);add(stop);add(preset);add(skinButton);add(freeButton);add(device);
    erupt.setComponentID("erupt");play.setComponentID("play");stop.setComponentID("stop");
    erupt.onClick=[this]{pulse();};erupt.setTooltip("Explosive reaction, then return to the current crowd. Automate Big Play Trigger low/high or send MIDI note 36.");
    play.onClick=[this]{p.audition.store(true);};stop.onClick=[this]{p.audition.store(false);};
    play.setTooltip("Start continuous synthesis in standalone");stop.setTooltip("Fade out and stop standalone synthesis");
    freeButton.onClick=[this]{auto* q=p.state.getParameter("free");q->beginChangeGesture();q->setValueNotifyingHost(q->getValue()>.5f?0.f:1.f);q->endChangeGesture();};
    freeButton.setTooltip("FOLLOW HOST starts and stops with the DAW. FREE RUN generates while the host transport is stopped.");
    preset.onClick=[this]{juce::PopupMenu m;const char* names[]{"Mellow stands","Anticipation","Full celebration","Small gathering"};for(int i=0;i<4;++i)m.addItem(i+1,names[i]);m.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(&preset),[safe=juce::Component::SafePointer<CrowdEditor>(this)](int n){if(safe&&n>0){safe->presetIndex=n-1;safe->p.applyPreset(n-1);for(int i=0;i<ControlCount;++i)safe->presetValues[(size_t)i]=safe->p.state.getRawParameterValue(controlIds[i])->load();}});};
    skinButton.onClick=[this]{p.skin.store((p.skin.load()+1)%3);look.skin=p.skin.load();repaint();};
    device.onClick=[this]{if(onDevices)onDevices();};
    play.setVisible(p.standalone.load());stop.setVisible(p.standalone.load());device.setVisible(p.standalone.load());freeButton.setVisible(!p.standalone.load());
    setSize(1060,740);setResizable(false,false);startTimerHz(24);
}
CrowdEditor::~CrowdEditor(){stopTimer();setLookAndFeel(nullptr);}
void CrowdEditor::pulse(){if(auto* q=p.state.getParameter("erupt")){q->beginChangeGesture();q->setValueNotifyingHost(1);q->endChangeGesture();triggerOff=juce::Time::getMillisecondCounter()+160;}}
void CrowdEditor::timerCallback(){if(triggerOff&&(int32_t)(juce::Time::getMillisecondCounter()-triggerOff)>=0){triggerOff=0;if(auto* q=p.state.getParameter("erupt")){q->beginChangeGesture();q->setValueNotifyingHost(0);q->endChangeGesture();}}level=std::max(p.meter.load(),level*.86f);look.skin=p.skin.load();freeButton.setButtonText(p.state.getRawParameterValue("free")->load()>.5f?"FREE RUN":"FOLLOW HOST");repaint();}
void CrowdEditor::resized(){
    for(int i=0;i<6;++i){cells[(size_t)i]={48+i*161,214,151,194};knobs[(size_t)i].setBounds(cells[(size_t)i].withTrimmedTop(24).withTrimmedBottom(37));}
    for(int i=6;i<ControlCount;++i){cells[(size_t)i]={45+(i-6)*139,442,134,166};knobs[(size_t)i].setBounds(cells[(size_t)i].withTrimmedTop(20).withTrimmedBottom(34));}
    erupt.setBounds(783,641,216,51);preset.setBounds(54,642,82,28);skinButton.setBounds(54,677,82,27);
    play.setBounds(598,646,59,43);stop.setBounds(662,646,59,43);freeButton.setBounds(580,651,145,34);device.setBounds(503,651,81,34);
}
void CrowdEditor::paint(juce::Graphics& g){
    auto ink=look.ink();g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.drawImage(RasterAssets::get().panels[(size_t)look.skin],getLocalBounds().toFloat(),juce::RectanglePlacement::stretchToFit);
    label(g,"CROWDBASE",{45,41,388,46},ink,35);label(g,"PROCEDURAL CROWD ENSEMBLE",{49,91,380,22},ink.withAlpha(.7f),12);
    label(g,"LAKAM AUDIO     /     MODEL CB-01",{49,124,380,16},ink.withAlpha(.65f),10);
    // Photographed VU frame with a live needle over its printed scale.
    juce::Rectangle<float> meterBounds{478,36,213,119};g.drawImage(RasterAssets::get().meter,meterBounds,juce::RectanglePlacement::stretchToFit);
    float db=juce::Decibels::gainToDecibels(level,-60.f);float pos=juce::jlimit(0.f,1.f,(db+38)/38);
    float angle=(-.76f+pos*1.45f);float cx=584,cy=130;
    for(int i=0;i<=20;++i){float a=-.76f+i/20.f*1.45f;float outer=56,inner=i%5==0?48.f:52.f;g.setColour(juce::Colour(i>=19?0xff9c3826:0xff39352e));g.drawLine(cx+std::sin(a)*inner,cy-std::cos(a)*inner,cx+std::sin(a)*outer,cy-std::cos(a)*outer,.8f);if(i%5==0)label(g,juce::String(-40+i*2),{(int)(cx+std::sin(a)*66)-12,(int)(cy-std::cos(a)*66)-5,24,12},juce::Colour(0xff39352e),9);}
    label(g,"dBFS",{565,103,40,15},juce::Colour(0xff605343),10);g.setColour(juce::Colour(0xff632a1e));g.drawLine(cx,cy,cx+std::sin(angle)*51,cy-std::cos(angle)*51,1.4f);
    label(g,"OUTPUT",{479,156,211,16},ink.withAlpha(.65f),10);
    juce::Rectangle<float> display{724,52,282,91};drawVfdHousing(g,display);
    juce::String state=p.playing.load()?(p.reactionMeter.load()>.12f?"REACTION":"LIVE CROWD"):"TRANSPORT STOPPED";
    status.draw(g,state,{742,65,250,23},1.65f,false);
    static constexpr const char* sceneNames[]{"MELLOW STANDS","ANTICIPATION","FULL CELEBRATION","SMALL GATHERING"};
    auto shownEnergy=p.playing.load()?p.energyMeter.load():p.state.getRawParameterValue("energy")->load();
    presetLine.draw(g,"ENERGY "+juce::String((int)std::round(shownEnergy*100))+"% / ENSEMBLE",{742,103,244,22},1.45f,false);
    g.setColour(ink.withAlpha(.22f));g.drawHorizontalLine(190,52,1006);g.drawHorizontalLine(438,52,1006);g.drawHorizontalLine(625,52,1006);
    label(g,"THE CROWD",{54,168,130,20},ink,10);label(g,"SPACE & CHARACTER",{54,413,150,20},ink,10);
    for(int i=0;i<ControlCount;++i){auto cell=cells[(size_t)i];label(g,juce::String(controlNames[i]).toUpperCase(),cell.withHeight(23),ink,11);
        auto vfd=cell.withTrimmedTop(cell.getHeight()-31).reduced(15,0).toFloat();drawVfdHousing(g,vfd);
        auto value=(float)knobs[(size_t)i].getValue();juce::String text=i==Output?juce::String(value,1)+" DB":i==Settle?juce::String(value,1)+" S":juce::String((int)std::round(value*100))+" %";
        values[(size_t)i].draw(g,text,vfd.reduced(10,4),1.35f,false);
    }
    for(int i=0;i<ControlCount;++i)if(std::abs(presetValues[(size_t)i]-(float)knobs[(size_t)i].getValue())>.002f)presetIndex=-1;
    drawVfdHousing(g,{146,643,330,27});skinLine.draw(g,presetIndex<0?"CUSTOM CROWD":sceneNames[presetIndex],{157,647,310,20},1.30f,false);
    drawVfdHousing(g,{146,678,330,26});static constexpr const char* finishes[]{"IVORY / WALNUT","GRAPHITE","VINTAGE WOOD"};
    finishLine.draw(g,finishes[look.skin],{157,680,310,22},1.25f,false);
    label(g,"FAST ATTACK / NATURAL SETTLE",{783,697,216,16},ink.withAlpha(.7f),9);
}
