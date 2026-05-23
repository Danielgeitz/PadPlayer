#include "daisy_seed.h"
#include "Display.h"
#include "Setlist.h"
#include "AudioPlayer.h"
#include "Metronome.h"
#include "PadBrowser.h"
#include "EncoderMode.h"

using namespace daisy;

#define MAX_PADS 24

DaisySeed hw;

Switch tapButton;
Switch nextButton;
Switch playStopButton;
Switch clickMuteButton;

Encoder encoder;

EncoderMode encoderMode = EncoderMode::BPM;


char padFiles[MAX_PADS][256];
char padNames[MAX_PADS][128];
int  padCount    = 0;
int  selectedPad = 0;

unsigned int encoderPressTime  = 0;
bool         encoderWasPressed = false;
bool         inPadBrowser      = false;
unsigned int lastDisplayUpdate = 0;
unsigned int lastBeatTime      = 0;
int          beatCount         = 0;

const char* currentKey     = "";
const char* currentPadName = "";
const char* nextKey        = "";
const char* nextPadName    = "";

// LEDs
GPIO ledBpm;
GPIO ledPad;

Metronome   metronome;
Display     display(hw);
SdHandler   sd(hw);
Setlist     setlist(sd);
AudioPlayer audioPlayer;
PadBrowser  padBrowser;

bool WasCrossfading_;

// ADC channel indices
static constexpr int ADC_PAD_VOL   = 0; // A0 — pin 15
static constexpr int ADC_CLICK_VOL = 1; // A1 — pin 16

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float padOut, clickOut;
        audioPlayer.Process(padOut, clickOut);
        out[0][i] = padOut;
        out[1][i] = clickOut;
    }
}

void handleEncoder()
{
    encoder.Debounce();
    int32_t increment = encoder.Increment();

    static unsigned int holdStartTime      = 0;
    static bool         holdActive         = false;
    static bool         longPressTriggered = false;

    // START PRESS
    if(encoder.RisingEdge())
    {
        holdStartTime      = System::GetNow();
        holdActive         = true;
        longPressTriggered = false;
    }

    // WHILE HOLDING
    if(encoder.Pressed() && holdActive && !longPressTriggered)
    {
        unsigned int holdTime = System::GetNow() - holdStartTime;

        // LONG PRESS (2 seconds)
        if(holdTime >= 2000 && inPadBrowser)
        {
            longPressTriggered = true;

            PadFile* selected = padBrowser.GetSelected();

            if(selected && !audioPlayer.IsCrossfading())
            {
                audioPlayer.PreloadNextPad(selected->filepath);
                audioPlayer.CrossfadeToPreloaded();
                currentKey     = selected->key;
                currentPadName = selected->padName;
            }
            encoderMode  = EncoderMode::BPM;
            inPadBrowser = false;
            ledBpm.Write(true);
            ledPad.Write(false);
        }
    }

    // END PRESS (release only resets state, no logic decisions)
    if(encoder.FallingEdge())
    {
        unsigned int holdTime = System::GetNow() - holdStartTime;
        holdActive            = false;

        // SHORT PRESS (<2s)
        if(holdTime < 500)
        {
            if(encoderMode == EncoderMode::BPM)
            {
                encoderMode       = EncoderMode::PAD_SELECT;
                inPadBrowser      = true;
                lastDisplayUpdate = System::GetNow();

                ledBpm.Write(false);
                ledPad.Write(true);

                PadFile* sel = padBrowser.GetSelected();
                if(sel)
                    display.ShowPadBrowser(sel->key,
                                           sel->padName,
                                           padBrowser.GetSelectedIndex(),
                                           padBrowser.GetCount());
                else
                    display.ShowPadName("NO PADS");

                display.Update();
            }
            else if(encoderMode == EncoderMode::PAD_SELECT)
            {
                encoderMode  = EncoderMode::BPM;
                inPadBrowser = false;

                ledBpm.Write(true);
                ledPad.Write(false);
            }
        }
    }

    // ENCODER ROTATION
    if(increment != 0)
    {
        if(encoderMode == EncoderMode::BPM)
        {
            int newBpm = metronome.GetBpm() + increment;
            if(newBpm < 40)
                newBpm = 40;
            if(newBpm > 300)
                newBpm = 300;
            metronome.SetBpm(newBpm);
        }
        else
        {
            if(increment > 0)
                padBrowser.Next();
            else
                padBrowser.Prev();

            PadFile* sel = padBrowser.GetSelected();
            if(sel)
                display.ShowPadBrowser(sel->key,
                                       sel->padName,
                                       padBrowser.GetSelectedIndex(),
                                       padBrowser.GetCount());
        }
    }

    // KEEP REFRESHING BROWSER DISPLAY
    if(inPadBrowser)
    {
        PadFile* sel = padBrowser.GetSelected();
        if(sel)
            display.ShowPadBrowser(sel->key,
                                   sel->padName,
                                   padBrowser.GetSelectedIndex(),
                                   padBrowser.GetCount());
    }
}

const char* GetPadDisplayName()
{
    if(padCount == 0)
        return "NO PADS";

    const char* full = padFiles[selectedPad];

    // skip "/pads/"
    const char* name = full + 6;

    static char clean[64];
    strncpy(clean, name, 63);
    clean[63] = '\0';

    char* dot = strrchr(clean, '.');
    if(dot)
        *dot = '\0';

    return clean;
}

int main(void)
{
    hw.Init();
    display.Init();

    if(!sd.Init())
    {
        while(1)
        {
            hw.SetLed(true);
            System::Delay(500);
            hw.SetLed(false);
            System::Delay(500);
        }
    }

    if(!setlist.Load())
    {
        while(1)
        {
            hw.SetLed(true);
            System::Delay(200);
            hw.SetLed(false);
            System::Delay(200);
        }
    }


    PadFile tempFiles[MAX_PADS];

    padCount = 0;

    // 1. scan SD card
    bool ok = sd.ListWavFiles("/pads", padFiles, padNames, padCount, MAX_PADS);

    if(!ok || padCount == 0)
    {
        while(1)
        {
            hw.SetLed(true);
            System::Delay(200);
            hw.SetLed(false);
            System::Delay(200);
        }
    }

    for(int i = 0; i < padCount; i++)
    {
        PadFile& f = tempFiles[i];
        memset(&f, 0, sizeof(PadFile));

        // name from padNames — already bounded to 128
        strncpy(f.name, padNames[i], PAD_FILENAME_MAX - 1);
        f.name[PAD_FILENAME_MAX - 1] = '\0';

        // Build filepath using fixed-size pieces to avoid truncation warning
        // /pads/ = 6, name max 119, .wav = 4, null = 1 → total max 130
        char tmp[120];
        strncpy(tmp, f.name, 119);
        tmp[119] = '\0';
        snprintf(f.filepath, sizeof(f.filepath), "/pads/%s.wav", tmp);
    }

    // 3. give to browser
    padBrowser.SetFiles(tempFiles, padCount);


    // Encoder — A, B, SW with internal pullups
    encoder.Init(seed::D20, seed::D19, seed::D18, 1000);

    // LEDs
    ledBpm.Init(seed::D17, GPIO::Mode::OUTPUT);
    ledPad.Init(seed::D16, GPIO::Mode::OUTPUT);

    // Start in BPM mode
    ledBpm.Write(true);
    ledPad.Write(false);

    //ADC
    AdcChannelConfig adcCfg[2];
    adcCfg[ADC_PAD_VOL].InitSingle(hw.GetPin(15));   // A0
    adcCfg[ADC_CLICK_VOL].InitSingle(hw.GetPin(16)); // A1
    hw.adc.Init(adcCfg, 2);
    hw.adc.Start();

    audioPlayer.Init(hw.AudioSampleRate());
    audioPlayer.SetCrossfadeDuration(2.0f);

    SetlistEntry* first  = setlist.GetCurrent();
    SetlistEntry* second = setlist.GetNext();

    if(first != nullptr)
    {
        metronome.SetBpm(first->bpm);
        audioPlayer.OpenPad(first->filepath);
        currentKey     = first->key;
        currentPadName = first->name;
    }

    if(second != nullptr)
    {
        audioPlayer.PreloadNextPad(second->filepath);
        nextKey     = second->key;
        nextPadName = second->name;
    }

    audioPlayer.OpenClick("/click/click.wav");
    audioPlayer.OpenClickAccent("/click/click_accent.wav");

    display.Init(); // reinit after SD operations

    tapButton.Init(seed::D26,
                   1000,
                   Switch::TYPE_MOMENTARY,
                   Switch::POLARITY_INVERTED,
                   GPIO::Pull::PULLUP);
    nextButton.Init(seed::D25,
                    1000,
                    Switch::TYPE_MOMENTARY,
                    Switch::POLARITY_INVERTED,
                    GPIO::Pull::PULLUP);
    playStopButton.Init(seed::D24,
                        1000,
                        Switch::TYPE_MOMENTARY,
                        Switch::POLARITY_INVERTED,
                        GPIO::Pull::PULLUP);
    clickMuteButton.Init(seed::D22,
                         1000,
                         Switch::TYPE_MOMENTARY,
                         Switch::POLARITY_INVERTED,
                         GPIO::Pull::PULLUP);

    hw.SetAudioBlockSize(1024);
    hw.StartAudio(AudioCallback);
    audioPlayer.Play();

    while(1)
    {
        audioPlayer.Prepare();

        tapButton.Debounce();
        nextButton.Debounce();
        playStopButton.Debounce();
        clickMuteButton.Debounce();

        // read Pots
        float rawPad   = hw.adc.GetFloat(ADC_PAD_VOL);
        float rawClick = hw.adc.GetFloat(ADC_CLICK_VOL);
        audioPlayer.SetPadVolume(rawPad * rawPad);
        // Only write click volume from the pot when unmuted — mute holds it at 0
        if(audioPlayer.IsClickPlaying())
            audioPlayer.SetClickVolume(rawClick * rawClick);

        if(tapButton.RisingEdge())
            metronome.Tap(System::GetNow());

        if(nextButton.RisingEdge() && !audioPlayer.IsCrossfading())
        {
            setlist.Next();
            SetlistEntry* current = setlist.GetCurrent();

            if(current != nullptr)
            {
                metronome.SetBpm(current->bpm);
                audioPlayer.CrossfadeToPreloaded();
                currentKey     = current->key;
                currentPadName = current->name;
            }

            beatCount    = 0;
            lastBeatTime = System::GetNow();
        }

        bool isNowCrossfading = audioPlayer.IsCrossfading();

        if(WasCrossfading_ && !isNowCrossfading)
        {
            SetlistEntry* next = setlist.GetNext();
            if(next)
                audioPlayer.PreloadNextPad(next->filepath);
        }

        WasCrossfading_ = isNowCrossfading;

        // Play/Stop
        if(playStopButton.RisingEdge())
        {
            if(audioPlayer.IsPlaying())
                audioPlayer.Stop();
            else
                audioPlayer.Play();
        }

        // Click mute
        if(clickMuteButton.RisingEdge())
            audioPlayer.SetClickPlaying(!audioPlayer.IsClickPlaying());

        // Click clock
        unsigned int now      = System::GetNow();
        unsigned int interval = metronome.GetIntervalMs();

        if(interval > 0 && (now - lastBeatTime >= interval))
        {
            lastBeatTime += interval;

            SetlistEntry* current   = setlist.GetCurrent();
            int           numerator = current ? current->numerator : 4;

            beatCount++;
            if(beatCount > numerator)
                beatCount = 1;

            audioPlayer.TriggerClick(beatCount == 1);
        }

        if(now - lastDisplayUpdate > 500)
        {
            if(!inPadBrowser)
            {
                SetlistEntry* current = setlist.GetCurrent();
                SetlistEntry* next    = setlist.GetNext();

                if(current && next)
                {
                    display.Render(metronome.GetBpm(),
                                   currentKey,     // was current->key
                                   currentPadName, // was current->name
                                   next->key,
                                   next->name,
                                   current->numerator,
                                   current->denominator,
                                   audioPlayer.IsPlaying(),
                                   audioPlayer.IsClickPlaying());
                }
            }
            lastDisplayUpdate = System::GetNow();
        }

        handleEncoder();
    }
}