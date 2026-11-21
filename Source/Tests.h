#pragma once
#include "Processor.h"
void renderDemo(juce::AudioProcessor&,const juce::File&,bool hostTransport);
juce::var runTests();
juce::var testHosted(const juce::File&,const juce::File&,const juce::File&);
