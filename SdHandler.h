#pragma once
#include "daisy_seed.h"
#include "fatfs.h"

using namespace daisy;

enum class SdError
{
    NONE,
    MOUNT_FAILED,
    FILE_NOT_FOUND,
    READ_FAILED,
    BUFFER_TOO_SMALL,
    NOT_MOUNTED,
    NO_FILE_OPEN
};

class SdHandler
{
  public:
    SdHandler(DaisySeed& hw);

    bool    Init();
    SdError GetLastError() const;

    size_t ReadFile(const char* path, uint8_t* buffer, size_t maxSize);

    bool OpenFile(const char* path);
    bool ReadLine(char* buffer, size_t maxLen);
    void CloseFile();
    void Flush();
    bool ListWavFiles(const char* path,
                      char        filepaths[][256],
                      char        names[][128],
                      int&        count,
                      int         maxFiles);

    bool IsMounted() const;

  private:
    DaisySeed&     hw_;
    SdmmcHandler   sd_;
    FatFSInterface fsi_;
    FIL            file_;
    bool           mounted_;
    bool           fileOpen_;
    SdError        lastError_;

    bool TryMount(SdmmcHandler::Speed speed, SdmmcHandler::BusWidth width);
};