#pragma once
#include "SdHandler.h"

#define MAX_PAD_FILES 24
#define PAD_FILENAME_MAX 128

struct PadFile
{
    char name[PAD_FILENAME_MAX];
    char filepath[256];
    char key[4];
    char padName[PAD_FILENAME_MAX];
};

class PadBrowser
{
  public:
    PadBrowser() : count_(0), selectedIndex_(0) {}

    void SetFiles(PadFile* files, int count);

    void Next();
    void Prev();

    PadFile* GetSelected();
    int      GetSelectedIndex() const { return selectedIndex_; }
    int      GetCount() const { return count_; }

  private:
    PadFile files_[MAX_PAD_FILES];
    int     count_;
    int     selectedIndex_;

    void ParseKeyAndName(PadFile& file);
};