#pragma once
#include "SdHandler.h"

static constexpr const char* SETLIST_PATH = "/setlist/setlist.csv";
static constexpr const char* PADS_PATH    = "/pads/";
static constexpr const char* CLICK_PATH   = "/click/";

struct SetlistEntry
{
    char key[4];       // "A", "Bb", "C#"
    char name[64];     // "Soft Pad"
    char filename[72]; // "A_Soft Pad.wav"
    char filepath[80]; // "/pads/A_Soft Pad.wav"
    int  bpm;
    int  numerator;
    int  denominator;
    bool valid; // false if line failed to parse
};

class Setlist
{
  public:
    Setlist(SdHandler& sd);

    bool Load();
    void Next();

    SetlistEntry* GetCurrent();
    SetlistEntry* GetNext();
    int           GetCount() const;
    bool          IsLoaded() const;

  private:
    SdHandler&   sd_;
    SetlistEntry entries_[12];
    int          count_;
    int          currentIndex_;
    bool         loaded_;

    bool ParseLine(const char* line, SetlistEntry& entry);
    void ParseKeyAndName(const char* token, SetlistEntry& entry);
    void
    ParseTimeSignature(const char* token, int& numerator, int& denominator);
};