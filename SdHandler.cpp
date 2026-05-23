#include "SdHandler.h"
#include <string.h>

SdHandler::SdHandler(DaisySeed& hw)
: hw_(hw), mounted_(false), fileOpen_(false), lastError_(SdError::NONE)
{
}

bool SdHandler::TryMount(SdmmcHandler::Speed    speed,
                         SdmmcHandler::BusWidth width)
{
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd_cfg.speed = speed;
    sd_cfg.width = width;
    sd_.Init(sd_cfg);
    fsi_.Init(FatFSInterface::Config::MEDIA_SD);
    FRESULT res = f_mount(&fsi_.GetSDFileSystem(), "/", 1);
    return res == FR_OK;
}

bool SdHandler::Init()
{
    mounted_   = false;
    lastError_ = SdError::NONE;

    if(TryMount(SdmmcHandler::Speed::MEDIUM_SLOW,
                SdmmcHandler::BusWidth::BITS_1))
    {
        mounted_ = true;
        return true;
    }

    if(TryMount(SdmmcHandler::Speed::SLOW, SdmmcHandler::BusWidth::BITS_1))
    {
        mounted_ = true;
        return true;
    }

    lastError_ = SdError::MOUNT_FAILED;
    return false;
}

SdError SdHandler::GetLastError() const
{ return lastError_; }
bool SdHandler::IsMounted() const
{ return mounted_; }

size_t SdHandler::ReadFile(const char* path, uint8_t* buffer, size_t maxSize)
{
    lastError_ = SdError::NONE;

    if(!mounted_)
    {
        lastError_ = SdError::NOT_MOUNTED;
        return 0;
    }

    FIL file;
    if(f_open(&file, path, FA_READ) != FR_OK)
    {
        lastError_ = SdError::FILE_NOT_FOUND;
        return 0;
    }

    size_t fileSize = f_size(&file);
    if(fileSize > maxSize)
    {
        f_close(&file);
        lastError_ = SdError::BUFFER_TOO_SMALL;
        return 0;
    }

    UINT bytesRead = 0;
    if(f_read(&file, buffer, fileSize, &bytesRead) != FR_OK)
    {
        f_close(&file);
        lastError_ = SdError::READ_FAILED;
        return 0;
    }

    f_close(&file);
    return bytesRead;
}

bool SdHandler::OpenFile(const char* path)
{
    lastError_ = SdError::NONE;

    if(!mounted_)
    {
        lastError_ = SdError::NOT_MOUNTED;
        return false;
    }

    if(fileOpen_)
        CloseFile();

    if(f_open(&file_, path, FA_READ) != FR_OK)
    {
        lastError_ = SdError::FILE_NOT_FOUND;
        return false;
    }

    fileOpen_ = true;
    return true;
}

bool SdHandler::ReadLine(char* buffer, size_t maxLen)
{
    if(!fileOpen_)
    {
        lastError_ = SdError::NO_FILE_OPEN;
        return false;
    }

    if(f_gets(buffer, maxLen, &file_) == NULL)
        return false;

    size_t len = strlen(buffer);
    while(len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
        buffer[--len] = '\0';

    return true;
}

void SdHandler::Flush()
{
    if(fileOpen_)
        CloseFile();
}

void SdHandler::CloseFile()
{
    if(fileOpen_)
    {
        f_close(&file_);
        fileOpen_ = false;
    }
}

bool SdHandler::ListWavFiles(const char* path,
                             char        filepaths[][256],
                             char        names[][128],
                             int&        count,
                             int         maxFiles)
{
    count = 0;

    DIR     dir;
    FILINFO fno;

    if(f_opendir(&dir, path) != FR_OK)
        return false;

    while(count < maxFiles)
    {
        if(f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == 0)
            break;

        if(fno.fattrib & (AM_HID | AM_DIR))
            continue;

        char* dot = strrchr(fno.fname, '.');
        if(!dot)
            continue;

        if(strcmp(dot, ".wav") != 0 && strcmp(dot, ".WAV") != 0)
            continue;

        // NAME WITHOUT EXTENSION — store clean name for display
        strncpy(names[count], fno.fname, 127);
        names[count][127] = '\0';
        char* nameDot     = strrchr(names[count], '.');
        if(nameDot)
            *nameDot = '\0';

        // FULL PATH — always built as /path/ + filename with .wav
        // handles both "/pads" and "/pads/" input gracefully
        size_t pathLen = strlen(path);
        if(pathLen > 0 && path[pathLen - 1] == '/')
            snprintf(filepaths[count], 256, "%s%.120s", path, fno.fname);
        else
            snprintf(filepaths[count], 256, "%s/%.120s", path, fno.fname);

        count++;
    }

    f_closedir(&dir);
    return count > 0;
}