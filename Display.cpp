#include "Display.h"
#include <stdio.h>

Display::Display(DaisySeed& hw) : hw_(hw) {}

void Display::Init()
{
    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.pin_config.dc    = hw_.GetPin(9);
    disp_cfg.driver_config.transport_config.pin_config.reset = hw_.GetPin(30);
    display_.Init(disp_cfg);
}

void Display::Clear()
{ display_.Fill(false); }

void Display::Update()
{ display_.Update(); }

void Display::ShowBpm(int bpm)
{
    sprintf(strbuff_, "BPM: %d", bpm);
    display_.SetCursor(0, 0);
    display_.WriteString(strbuff_, Font_7x10, true);
}

void Display::ShowPadName(const char* name)
{
    display_.SetCursor(0, 25);
    display_.WriteString(name, Font_11x18, true);
}

void Display::ShowKey(const char* key)
{
    sprintf(strbuff_, "Key: %s", key);
    display_.SetCursor(80, 0);
    display_.WriteString(strbuff_, Font_7x10, true);
}

void Display::ShowNextPadName(const char* key, const char* name)
{
    sprintf(strbuff_, "> %s %s", key, name);
    display_.SetCursor(0, 56);
    display_.WriteString(strbuff_, Font_6x8, true);
}

void Display::ShowTimeSignature(int numerator, int denominator)
{
    sprintf(strbuff_, "%d/%d", numerator, denominator);
    display_.SetCursor(100, 14);
    display_.WriteString(strbuff_, Font_7x10, true);
}

void Display::ShowStatus(bool padPlaying, bool clickPlaying)
{
    sprintf(strbuff_,
            "%s %s",
            padPlaying ? "PAD:>" : "PAD:||",
            clickPlaying ? "CLK:>" : "CLK:||");

    int len = strlen(strbuff_);

    // Font_4x6 → 4 pixels wide per char
    int x = 128 - (len * 4);
    int y = 64 - 8;

    display_.SetCursor(x, y);
    display_.WriteString(strbuff_, Font_4x6, true);
}

void Display::ShowPadBrowser(const char* key,
                             const char* name,
                             int         index,
                             int         total)
{
    display_.Fill(false);

    display_.SetCursor(0, 0);
    display_.WriteString("SELECT PAD", Font_6x8, true);

    // Key
    display_.SetCursor(0, 14);
    display_.WriteString(key, Font_11x18, true);

    // Name — truncate to fit display (max ~18 chars at Font_7x10)
    char nameBuf[20];
    strncpy(nameBuf, name, 19);
    nameBuf[19] = '\0';
    display_.SetCursor(0, 35);
    display_.WriteString(nameBuf, Font_7x10, true);

    // Index
    char idxBuf[24];
    snprintf(idxBuf, sizeof(idxBuf), "%d/%d", index + 1, total);
    display_.SetCursor(90, 56);
    display_.WriteString(idxBuf, Font_6x8, true);

    display_.SetCursor(0, 56);
    display_.WriteString("HOLD=OK", Font_6x8, true);

    display_.Update();
}

void Display::Render(int         bpm,
                     const char* key,
                     const char* padName,
                     const char* nextKey,
                     const char* nextPadName,
                     int         numerator,
                     int         denominator,
                     bool        padPlaying,
                     bool        clickPlaying)
{
    Clear();
    ShowBpm(bpm);
    ShowTimeSignature(numerator, denominator);
    ShowKey(key);
    ShowPadName(padName);
    ShowNextPadName(nextKey, nextPadName);
    ShowStatus(padPlaying, clickPlaying);
    Update();
}