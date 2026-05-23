#include "PadBrowser.h"
#include <cstring>

void PadBrowser::SetFiles(PadFile* files, int count)
{
    count_ = (count > MAX_PAD_FILES) ? MAX_PAD_FILES : count;
    for(int i = 0; i < count_; i++)
    {
        files_[i] = files[i];
        ParseKeyAndName(files_[i]); // parse from file.name
    }
    selectedIndex_ = 0;
}

void PadBrowser::Next()
{
    if(count_ == 0)
        return;
    selectedIndex_ = (selectedIndex_ + 1) % count_;
}

void PadBrowser::Prev()
{
    if(count_ == 0)
        return;
    selectedIndex_ = (selectedIndex_ - 1 + count_) % count_;
}

PadFile* PadBrowser::GetSelected()
{
    if(count_ == 0)
        return nullptr;
    return &files_[selectedIndex_];
}

void PadBrowser::ParseKeyAndName(PadFile& file)
{
    // file.name = "A_Clouds" — parse key and padName from this
    const char* underscore = strchr(file.name, '_');
    if(!underscore)
    {
        file.key[0] = '\0';
        strncpy(file.padName, file.name, PAD_FILENAME_MAX - 1);
        file.padName[PAD_FILENAME_MAX - 1] = '\0';
        return;
    }
    int keyLen = underscore - file.name;
    if(keyLen > 3)
        keyLen = 3;
    strncpy(file.key, file.name, keyLen);
    file.key[keyLen] = '\0';
    strncpy(file.padName, underscore + 1, PAD_FILENAME_MAX - 1);
    file.padName[PAD_FILENAME_MAX - 1] = '\0';
}