#include "Engine.h"
#include "BankIndex.h"
#include <algorithm>
#include <cmath>

namespace {constexpr float pi=3.14159265358979323846f;float clip(float x,float a,float b){return std::clamp(x,a,b);} }
float CrowdEngine::random() noexcept {rng^=rng>>12;rng^=rng<<25;rng^=rng>>27;return float((rng*2685821657736338717ULL)>>40)*(1.f/16777216.f);}
void CrowdEngine::prepare(double rate,const float* samples,const float* samplesRight,int count,uint64_t seed) {
    sr=rate;bank=samples;bankR=samplesRight;bankSize=count;rng=seed?seed:1;dropped=0;time=0;
    target=smooth=std::to_array(defaults);voices={};clocks={};reactionAge=1000;
    runTarget=gate=reaction=shownEnergy=slow=slowTarget=lowL=lowR=dcL=dcR=0;
    shownVoices=controlTick=0;slowClock=0;
    constexpr double lengths[]{.0371,.0437,.0533,.0611,.0719,.0797,.0893,.1013};
    for(int i=0;i<8;++i){auto& d=delays[(size_t)i];d.data.assign((size_t)(sr*lengths[i])+1,0);d.pos=0;d.low=0;}
    // Pre-roll voices at independent ages; transport still begins with a fade.
    for(int i=0;i<16;++i){spawn(i%2,.18f);auto& v=voices[(size_t)i];v.age=random()*v.duration*.7;v.cursor+=v.age*sr*v.step;}
}
void CrowdEngine::setTargets(const std::array<float,ControlCount>& values,bool running) {
    for(int i=0;i<ControlCount;++i)target[(size_t)i]=std::isfinite(values[(size_t)i])?values[(size_t)i]:defaults[i];
    for(int i=0;i<ControlCount;++i)target[(size_t)i]=clip(target[(size_t)i],i==Output?-36.f:i==Settle?1.f:0.f,i==Output?6.f:i==Settle?25.f:1.f);
    runTarget=running?1.f:0.f;
}
void CrowdEngine::trigger() noexcept {reactionAge=0;}
void CrowdEngine::spawn(int kind,float e) noexcept {
    Voice* slot=nullptr;for(auto& v:voices)if(v.kind<0){slot=&v;break;}
    if(!slot){++dropped;return;}auto& v=*slot;v=Voice{};v.kind=kind;
    float pan=(random()*2-1)*.94f;v.panL=std::sqrt((1-pan)*.5f);v.panR=std::sqrt((1+pan)*.5f);
    float pop=smooth[Population];float level=1/std::sqrt(.45f+pop*1.4f);
    v.amp=(.65f+random()*.65f)*level;
    if(kind==0||kind==1){
        v.region=kind==0?(int)(random()*chatterRegionCount):chatterRegionCount+(int)(random()*(regions.size()-chatterRegionCount));auto r=regions[(size_t)v.region];
        float speed=.975f+random()*.05f;
        v.step=24000/sr*speed;v.cursor=random()*8000;
        v.duration=(r.length-v.cursor)/(24000.0*speed);
        v.amp*=kind==0?.055f:.18f;
    } else if(kind==2){v.duration=.055+random()*.035;v.amp*=.021f+.055f*e;v.freq=1300+random()*2500;}
    else {v.duration=.24+random()*.62;v.freq=2200+random()*1250;v.slope=(random()-.5f)*750;v.amp*=.010f+.018f*e;}
}
void CrowdEngine::render(float* left,float* right,int count) noexcept {
    const float sm=1-std::exp(-1.f/(float(sr)*.075f));
    const float gateStep=1.f/(float(sr)*.045f);
    const float dcCoef=1-std::exp(-2*pi*150/float(sr));
    for(int sample=0;sample<count;++sample){
        for(int k=0;k<ControlCount;++k)smooth[(size_t)k]+=(target[(size_t)k]-smooth[(size_t)k])*sm;
        gate+=clip(runTarget-gate,-gateStep,gateStep);
        // A stopped host freezes the population after the click-free fade.
        if(runTarget==0&&gate<.00001f){gate=0;left[sample]=right[sample]=0;continue;}
        reactionAge+=1/sr;time+=1/sr;
        reaction=float((1-std::exp(-reactionAge/.045))*std::exp(-std::max(0.,reactionAge-.45)/(smooth[Settle]/3.3)));
        if((slowClock-=1/sr)<=0){slowTarget=(random()*2-1)*smooth[Variation]*.11f;slowClock=1.8+random()*3.7;}
        slow+=(slowTarget-slow)*float(1/(sr*1.5));
        float e=clip(smooth[Energy]+slow+reaction*(1-smooth[Energy]),0,1);shownEnergy=e;
        // Poisson timing avoids a repeated loop or a mechanical applause clock.
        const float population=.22f+smooth[Population]*1.25f;
        const float rates[]{2.25f*population,
            2.45f*population,
            (.06f+52*e*e*std::sqrt(e))*smooth[Applause]*population,
            (.025f+1.25f*e*e)*smooth[Whistles]};
        for(int k=0;k<4;++k){clocks[(size_t)k]-=rates[k]/sr;if(clocks[(size_t)k]<=0&&rates[k]>.0001f){spawn(k,e);clocks[(size_t)k]+=-std::log(std::max(.000001f,random()));}}
        float l=0,r=0;int active=0;
        const float distance=smooth[Distance];
        const float cutoff=10500-6000*distance;
        const float lpCoef=1-std::exp(-2*pi*cutoff/float(sr));
        for(auto& v:voices){
            if(v.kind<0)continue;++active;float x=0,y=0;float u=float(v.age/v.duration);
            if(v.kind<2){
                const auto region=regions[(size_t)v.region];int index=(int)v.cursor;
                if(index+1>=region.length||region.start+index+1>=bankSize){v.kind=-1;continue;}
                float fraction=float(v.cursor-index);x=bank[region.start+index]*(1-fraction)+bank[region.start+index+1]*fraction;
                y=bankR[region.start+index]*(1-fraction)+bankR[region.start+index+1]*fraction;
                v.cursor+=v.step;
                const float attack=std::sin(clip(float(v.age/.45),0,1)*pi*.5f),release=std::sin(clip(float((v.duration-v.age)/.65),0,1)*pi*.5f);
                float envelope=attack*attack*release*release;
                float mix=v.kind==0?smooth[Chatter]*(.7f-.37f*e):smooth[Cheers]*e*e;
                x*=envelope*mix;y*=envelope*mix;
            } else if(v.kind==2){
                // Broad palm impact, not differentiated white noise (a hi-hat).
                float n=random()*2-1;
                float hp=1-std::exp(-2*pi*650/float(sr));
                float lp=1-std::exp(-2*pi*(2550-700*distance)/float(sr));
                v.noisePrev+=hp*(n-v.noisePrev);
                v.lp+=lp*(n-v.noisePrev-v.lp);v.lpR+=lp*(v.lp-v.lpR);
                x=v.lpR*1.7f*std::exp(-float(v.age)/.013f)*clip(float(v.age/.0035),0,1);
            } else {
                float frequency=v.freq+v.slope*std::sin(pi*u);
                v.phase+=2*pi*frequency/float(sr);if(v.phase>2*pi)v.phase-=2*pi;
                float env=clip(float(v.age/.055),0,1)*clip(float((v.duration-v.age)/.09),0,1);
                x=(std::sin(v.phase)+.015f*(random()*2-1))*env*.7f;
            }
            if(v.kind>=2)y=x;
            if(v.kind!=2){v.lp+=lpCoef*(x-v.lp);v.lpR+=lpCoef*(y-v.lpR);x=v.lp;y=v.lpR;}
            x*=v.amp;y*=v.amp;l+=x*v.panL;r+=y*v.panR;
            v.age+=1/sr;if(v.age>=v.duration)v.kind=-1;
        }
        shownVoices=active;
        // Eight-line normalized Householder feedback network, stable at all settings.
        float taps[8],sum=0;
        for(int k=0;k<8;++k){auto& d=delays[(size_t)k];float tap=d.data[d.pos];d.low+=.36f*(tap-d.low);taps[k]=d.low;sum+=d.low;}
        float wetL=0,wetR=0;
        for(int k=0;k<8;++k){auto& d=delays[(size_t)k];float input=(k%2?l:r)*.20f;float feedback=.47f+.30f*smooth[Space];d.data[d.pos]=input+feedback*(taps[k]-.25f*sum);if(++d.pos==d.data.size())d.pos=0;if(k%2)wetL+=taps[k]*(k%4==1?1:-1);else wetR+=taps[k]*(k%4==0?1:-1);}
        const float wet=(.10f+.9f*smooth[Space])*(.35f+.65f*distance);
        l=l*(1-distance*.37f)+wetL*wet;r=r*(1-distance*.37f)+wetR*wet;
        // Tone tilt, low rumble rejection and mono-compatible width.
        lowL+=.14f*(l-lowL);lowR+=.14f*(r-lowR);float bright=(smooth[Tone]-.5f)*1.2f;
        l+=(l-lowL)*bright;r+=(r-lowR)*bright;
        dcL+=dcCoef*(l-dcL);dcR+=dcCoef*(r-dcR);l-=dcL;r-=dcR;
        float mid=(l+r)*.5f,side=(l-r)*.5f*smooth[Width];
        float gain=std::pow(10.f,smooth[Output]/20)*gate*6.f;
        // Smooth ceiling protection; no blockwise normalization or pumping.
        left[sample]=std::tanh((mid+side)*gain);right[sample]=std::tanh((mid-side)*gain);
    }
}
