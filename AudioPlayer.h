#pragma once
#include "daisy_seed.h"
#include "fatfs.h"
#include "util/WavPlayer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#include <math.h>

using namespace daisy;

class AudioPlayer
{
  public:
    AudioPlayer();

    void Init(float sampleRate);
    void SetCrossfadeDuration(float seconds);

    void Prepare();

    void OpenPad(const char* filepath);
    void PreloadNextPad(const char* filepath);
    void CrossfadeToPreloaded();

    void OpenClick(const char* filepath);
    void OpenClickAccent(const char* filepath);

    void Process(float& padOut, float& clickOut);

    void  TriggerClick(bool isAccent);
    void  Play();
    void  Stop();
    void  SetClickMuted(bool muted);
    void  SetPadVolume(float vol) { padVolume_ = vol; }
    void  SetClickVolume(float vol) { clickVolume_ = vol; }
    void  SetClickPlaying(bool playing);
    float getClickVolume() { return clickVolume_; }

    bool IsPlaying() const;
    bool IsCrossfading() const;
    bool IsClickMuted() const;
    bool IsClickPlaying() const;

  private:
    WavPlayer<96000> padA_;
    WavPlayer<96000> padB_;
    WavPlayer<8000>  click_;
    WavPlayer<8000>  clickAccent_;

    // Crossfade — exactly like test project
    float gainA_;
    float gainB_;
    float xfadeAngle_;
    float xfadeStep_;
    bool  crossfading_;
    bool  useB_;

    // Master fade for play/stop
    float masterGain_;
    float masterStep_;
    bool  fadingIn_;
    bool  fadingOut_;

    float crossfadeDuration_;
    float sampleRate_;
    float padVolume_;
    float clickVolume_;

    bool isPlaying_;
    bool clickPending_;
    bool clickPlaying_;
    bool clickAccentPending_;
};