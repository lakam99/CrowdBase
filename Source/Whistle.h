#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

// A single breath of spectator whistling. The pitch stays on one note: large
// melodic arcs and rapid repeated syllables made the old oscillator sound avian.
// POD-sized voice state; start/process neither allocate nor share mutable state.
struct HumanWhistle {
    double duration=1,phase=0,vibratoPhase=0,driftClock=0;
    float pitch=2800,vibratoRate=5,drift=0,driftTarget=0;
    float attack=.04f,release=.1f,harmonic=.04f,breathLow=0,breathBase=0;
    float driftCoefficient=0,breathLowCoefficient=0,breathBaseCoefficient=0;
    double gapAt=-1;uint32_t rng=1;

    float random() noexcept {
        rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
        return float(rng>>8)*(1.f/16777216.f);
    }
    void start(double sampleRate,uint32_t seed) noexcept {
        *this=HumanWhistle{};rng=seed?seed:1;
        const bool finger=random()<.72f;
        pitch=finger?2450.f+random()*850.f:1850.f+random()*650.f;
        duration=.92+random()*1.22;
        attack=.025f+random()*.025f;release=.075f+random()*.045f;
        harmonic=finger?.035f+random()*.025f:.01f+random()*.012f;
        vibratoRate=4.4f+random()*1.3f;vibratoPhase=random()*twoPi;
        // An occasional second breath, rather than a train of little tweets.
        if(duration>1.8&&random()<.22f)gapAt=duration*(.49+random()*.06);
        driftTarget=random()*2-1;
        driftCoefficient=1-std::exp(-twoPi*7.f/float(sampleRate));
        breathLowCoefficient=1-std::exp(-twoPi*6500.f/float(sampleRate));
        breathBaseCoefficient=1-std::exp(-twoPi*1600.f/float(sampleRate));
    }
    float process(double age,double sampleRate) noexcept {
        if(age<0||age>=duration)return 0;
        if((driftClock-=1/sampleRate)<=0){
            driftTarget=random()*2-1;driftClock=.045+random()*.075;
        }
        drift+=(driftTarget-drift)*driftCoefficient;
        // A small pressure dip at the onset resolves within a few milliseconds;
        // ongoing pitch variation is a fraction of one semitone, not a glide.
        const float frequency=pitch*(1+.0018f*float(std::sin(vibratoPhase))+.0022f*drift)
                              -18.f*std::exp(-float(age)/.018f);
        phase+=twoPi*frequency/sampleRate;if(phase>=twoPi)phase-=twoPi;
        vibratoPhase+=twoPi*vibratoRate/sampleRate;if(vibratoPhase>=twoPi)vibratoPhase-=twoPi;
        const float a=ramp(float(age)/attack),r=ramp(float((duration-age)/release));
        float envelope=a*r;
        if(gapAt>0){
            const float breathGap=float(std::abs(age-gapAt));
            envelope*=ramp((breathGap-.045f)/.045f);
        }
        const float noise=random()*2-1;
        breathBase+=breathBaseCoefficient*(noise-breathBase);
        breathLow+=breathLowCoefficient*(noise-breathBase-breathLow);
        const float tone=float(std::sin(phase))+harmonic*float(std::sin(2*phase))
                         +.008f*float(std::sin(3*phase));
        return (tone*(.96f+.04f*drift)+breathLow*.024f)*envelope*.7f;
    }
private:
    static constexpr double twoPi=6.2831853071795864769;
    static float ramp(float x) noexcept {
        x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);
    }
};
