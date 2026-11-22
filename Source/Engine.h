#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include "Whistle.h"

enum Control { Energy, Population, Chatter, Cheers, Applause, Whistles, Distance, Space,
               Width, Tone, Variation, Settle, Output, TimeToPeak, ControlCount };
inline constexpr const char* controlIds[]{"energy","population","chatter","cheers","applause","whistles","distance","space","width","tone","variation","settle","output","timeToPeak"};
inline constexpr const char* controlNames[]{"Energy","Population","Crowd Bed","Cheers","Applause","Whistles","Distance","Space","Width","Tone","Variation","Settle","Output","Time to Peak"};
inline constexpr float defaults[]{.18f,.55f,.8f,.65f,.36f,.12f,.62f,.34f,.85f,.55f,.65f,9.f,-3.f,.15f};
inline constexpr float controlMin(int i){return i==Output?-36.f:i==Settle?1.f:i==TimeToPeak?.05f:0.f;}
inline constexpr float controlMax(int i){return i==Output?6.f:i==Settle?25.f:i==TimeToPeak?8.f:1.f;}

// Owns no threads. Bank lifetime exceeds the engine; no allocation in render().
class CrowdEngine {
public:
    void prepare(double sampleRate, const float* samples, const float* samplesRight, int count, uint64_t seed=0x43c6f2a9);
    void reseed(uint64_t seed) noexcept {rng=seed?seed:1;}
    void setTargets(const std::array<float,ControlCount>& values, bool running);
    void trigger() noexcept;
    void render(float* left,float* right,int count) noexcept;
    float currentEnergy() const noexcept { return shownEnergy; }
    int activeVoices() const noexcept {return shownVoices;}
    float reactionAmount() const noexcept {return reaction;}
    uint64_t droppedEvents() const noexcept {return dropped;}
private:
    struct Voice {
        int kind=-1,region=0; double cursor=0,step=1,age=0,duration=1;
        float amp=0,panL=0,panR=0,lp=0,lpR=0,hp=0,freq=0,noisePrev=0;
        HumanWhistle whistle;
    };
    struct Delay {std::vector<float> data;size_t pos=0;float low=0;};
    std::array<Voice,160> voices{};
    std::array<Delay,8> delays;
    std::array<float,ControlCount> target{},smooth{};
    std::array<double,4> clocks{};
    uint64_t rng=1,dropped=0; double sr=48000,time=0,reactionAge=1000;
    double reactionPeak=.15;float reactionStart=0;
    float runTarget=0,gate=0,reaction=0,shownEnergy=0,slow=0,slowTarget=0;
    float lowL=0,lowR=0,dcL=0,dcR=0;int shownVoices=0,controlTick=0;double slowClock=0;
    const float* bank=nullptr;const float* bankR=nullptr;int bankSize=0;
    float random() noexcept;
    void spawn(int kind,float energy) noexcept;
};
