#include "Setlist.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Setlist::Setlist(SdHandler& sd)
: sd_(sd), count_(0), currentIndex_(0), loaded_(false)
{
    for(int i = 0; i < 12; i++)
        entries_[i] = {};
}

bool Setlist::Load()
{
    loaded_ = false;
    count_  = 0;

    if(!sd_.OpenFile(SETLIST_PATH))
        return false;

    char line[128];
    int  lineNum = 0;

    while(sd_.ReadLine(line, sizeof(line)) && count_ < 12)
    {
        lineNum++;

        // Skip empty lines and comments
        if(line[0] == '\0' || line[0] == '#')
            continue;

        SetlistEntry entry = {};
        if(ParseLine(line, entry))
        {
            entries_[count_++] = entry;
        }
        // if ParseLine fails we just skip this line
    }

    sd_.CloseFile();
    loaded_ = count_ > 0;
    return loaded_;
}

bool Setlist::ParseLine(const char* line, SetlistEntry& entry)
{
    // Expected format: "A_Soft Pad, 120, 4/4"
    // Make a mutable copy to tokenize
    char buf[128];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    // Split on commas
    char* token1 = strtok(buf, ",");  // "A_Soft Pad"
    char* token2 = strtok(NULL, ","); // " 120"
    char* token3 = strtok(NULL, ","); // " 4/4"

    if(!token1 || !token2 || !token3)
        return false;

    // Trim leading spaces
    while(*token1 == ' ')
        token1++;
    while(*token2 == ' ')
        token2++;
    while(*token3 == ' ')
        token3++;

    // Parse key and name from token1
    ParseKeyAndName(token1, entry);

    // Build filename and filepath
    snprintf(entry.filename, sizeof(entry.filename), "%s.wav", token1);
    snprintf(
        entry.filepath, sizeof(entry.filepath), "%s%s.wav", PADS_PATH, token1);

    // Parse BPM
    entry.bpm = atoi(token2);
    if(entry.bpm <= 0 || entry.bpm > 400)
        return false;

    // Parse time signature
    ParseTimeSignature(token3, entry.numerator, entry.denominator);
    if(entry.numerator <= 0 || entry.denominator <= 0)
        return false;

    entry.valid = true;
    return true;
}

void Setlist::ParseKeyAndName(const char* token, SetlistEntry& entry)
{
    // Split "A_Soft Pad" on first '_'
    const char* underscore = strchr(token, '_');

    if(underscore == NULL)
    {
        // No underscore found — use whole token as name, no key
        strncpy(entry.name, token, sizeof(entry.name) - 1);
        entry.key[0] = '\0';
        return;
    }

    // Key is everything before '_'
    int keyLen = underscore - token;
    if(keyLen > 3)
        keyLen = 3; // max "C##" or "Bb" etc
    strncpy(entry.key, token, keyLen);
    entry.key[keyLen] = '\0';

    // Name is everything after '_'
    strncpy(entry.name, underscore + 1, sizeof(entry.name) - 1);
    entry.name[sizeof(entry.name) - 1] = '\0';
}

void Setlist::ParseTimeSignature(const char* token,
                                 int&        numerator,
                                 int&        denominator)
{
    // Split "4/4" on '/'
    char buf[16];
    strncpy(buf, token, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* slash = strchr(buf, '/');
    if(slash == NULL)
    {
        numerator   = 0;
        denominator = 0;
        return;
    }

    *slash      = '\0';
    numerator   = atoi(buf);
    denominator = atoi(slash + 1);
}

void Setlist::Next()
{
    if(count_ == 0)
        return;
    currentIndex_ = (currentIndex_ + 1) % count_;
}

SetlistEntry* Setlist::GetCurrent()
{
    if(count_ == 0)
        return nullptr;
    return &entries_[currentIndex_];
}

SetlistEntry* Setlist::GetNext()
{
    if(count_ == 0)
        return nullptr;
    int nextIndex = (currentIndex_ + 1) % count_;
    return &entries_[nextIndex];
}

int Setlist::GetCount() const
{
    return count_;
}
bool Setlist::IsLoaded() const
{
    return loaded_;
}