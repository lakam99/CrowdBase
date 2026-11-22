// Independent oscillator diagnostic. Build with a C++20 compiler; pass an output
// directory to write equal-level, six-event before/after listening comparisons.
#include "../Source/Whistle.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>

namespace {
void wav(const std::filesystem::path& path,const std::vector<float>& samples){
    std::ofstream out(path,std::ios::binary);uint32_t data=uint32_t(samples.size()*4);
    auto u16=[&](uint16_t n){out.write(reinterpret_cast<const char*>(&n),2);};
    auto u32=[&](uint32_t n){out.write(reinterpret_cast<const char*>(&n),4);};
    out.write("RIFF",4);u32(36+data);out.write("WAVEfmt ",8);u32(16);u16(3);u16(1);
    u32(48000);u32(48000*4);u16(4);u16(32);out.write("data",4);u32(data);
    out.write(reinterpret_cast<const char*>(samples.data()),data);
    if(!out)throw std::runtime_error("Could not write oscillator comparison");
}
}
int main(int argc,char** argv){
    if(argc!=2)return 2;
    std::filesystem::path directory(argv[1]);std::filesystem::create_directories(directory);
    constexpr double sr=48000,pi=3.14159265358979323846;
    std::vector<float> before(18*48000,0),after(before.size(),0);
    uint32_t seeds[]{28263,91726,1226,993182,31925,179323};
    for(int event=0;event<6;++event){
        HumanWhistle whistle;whistle.start(sr,seeds[event]);
        std::cout<<"event="<<event<<" pitch_hz="<<whistle.pitch<<" duration="<<whistle.duration<<"\n";
        const int offset=int((.3+3*event)*sr);
        for(int i=0;i<int(whistle.duration*sr);++i)after[offset+i]=.24f*whistle.process(i/sr,sr);
        HumanWhistle oldRandom;oldRandom.rng=seeds[event];
        double duration=.24+oldRandom.random()*.62;
        double pitch=2200+oldRandom.random()*1250,slope=(oldRandom.random()-.5)*750,phase=0;
        for(int i=0;i<int(duration*sr);++i){
            double age=i/sr,frequency=pitch+slope*std::sin(pi*age/duration);
            phase+=2*pi*frequency/sr;if(phase>2*pi)phase-=2*pi;
            double envelope=std::clamp(age/.055,0.,1.)*std::clamp((duration-age)/.09,0.,1.);
            before[offset+i]=float(.24*(std::sin(phase)+.015*(oldRandom.random()*2-1))*envelope*.7);
        }
    }
    wav(directory/"CrowdBase-whistles-before.wav",before);wav(directory/"CrowdBase-whistles-after.wav",after);
    // Exercise every whistle family/seed at common host rates. A reset oscillator
    // must reproduce exactly, all voices must finish, and no non-finite state may
    // leak into the host even during the optional second breath.
    bool passed=true;double peak=0;
    for(double rate:{44100.,48000.,96000.})for(uint32_t seed=1;seed<=40;++seed){
        HumanWhistle a,b;a.start(rate,seed*7193);b.start(rate,seed*7193);
        passed&=a.duration>=.92&&a.duration<=2.14&&a.pitch>=1850&&a.pitch<=3300;
        for(int i=0;i<int(a.duration*rate)+2;++i){
            float value=a.process(i/rate,rate),other=b.process(i/rate,rate);
            passed&=std::isfinite(value)&&value==other;peak=std::max(peak,double(std::abs(value)));
        }
        passed&=a.process(a.duration+1/rate,rate)==0;
    }
    std::cout<<"passed="<<(passed&&peak<1)<<" peak="<<peak<<" sample_rates=44100,48000,96000 seeds_per_rate=40\n";
    return passed&&peak<1?0:1;
}
