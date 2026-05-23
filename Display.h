#pragma once
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;
using MyOledDisplay = OledDisplay<SSD130x4WireSpi128x64Driver>;

class Display
{
  public:
    Display(DaisySeed& hw);

    void Init();
    void ShowBpm(int bpm);
    void ShowPadName(const char* name);
    void ShowTimeSignature(int numerator, int denominator);
    void ShowKey(const char* key);
    void ShowNextPadName(const char* key, const char* name);
    void Clear();
    void Update();
    void ShowStatus(bool padPlaying, bool clickPlaying);
    void
    ShowPadBrowser(const char* key, const char* name, int index, int total);

    void Render(int         bpm,
                const char* key,
                const char* padName,
                const char* nextKey,
                const char* nextPadName,
                int         numerator,
                int         denominator,
                bool        padPlaying,
                bool        clickPlaying);

  private:
    DaisySeed&    hw_;
    MyOledDisplay display_;
    char          strbuff_[128];
};