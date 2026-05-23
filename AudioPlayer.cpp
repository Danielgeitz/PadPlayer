#include "AudioPlayer.h"
#include <math.h>

AudioPlayer::AudioPlayer()
: gainA_(1.0f),
  gainB_(0.0f),
  xfadeAngle_(0.0f),
  xfadeStep_(0.0f),
  crossfading_(false),
  useB_(false),
  masterGain_(0.0f),
  masterStep_(0.0f),
  fadingIn_(false),
  fadingOut_(false),
  crossfadeDuration_(2.0f),
  sampleRate_(48000.0f),
  padVolume_(0.5f),
  clickVolume_(0.5f),
  isPlaying_(false),
  clickPending_(false),
  clickPlaying_(true),
  clickAccentPending_(false)
{
}

void AudioPlayer::Init(float sampleRate)
{
    sampleRate_ = sampleRate;
    xfadeStep_  = (M_PI / 2.0f) / (crossfadeDuration_ * sampleRate_);
    masterStep_ = 1.5f / sampleRate_;
}

void AudioPlayer::SetCrossfadeDuration(float seconds)
{
    crossfadeDuration_ = seconds;
    xfadeStep_         = (M_PI / 2.0f) / (crossfadeDuration_ * sampleRate_);
}

void AudioPlayer::Prepare()
{
    padA_.Prepare();
    padB_.Prepare();
    click_.Prepare();
    clickAccent_.Prepare();
}

void AudioPlayer::OpenPad(const char* filepath)
{
    padA_.Init(filepath);
    padA_.SetLooping(true);
    padA_.SetPlaying(true);
    gainA_       = 1.0f;
    gainB_       = 0.0f;
    useB_        = false;
    crossfading_ = false;
}

void AudioPlayer::PreloadNextPad(const char* filepath)
{
    if(useB_)
    {
        padA_.Init(filepath);
        padA_.SetLooping(true);
        padA_.SetPlaying(true);
        gainA_ = 0.0f;
    }
    else
    {
        padB_.Init(filepath);
        padB_.SetLooping(true);
        padB_.SetPlaying(true);
        gainB_ = 0.0f;
    }
}

void AudioPlayer::CrossfadeToPreloaded()
{
    if(crossfading_)
        return;

    if(!useB_)
        padB_.SetPlaying(true);
    else
        padA_.SetPlaying(true);

    xfadeAngle_  = 0.0f;
    crossfading_ = true;
}

void AudioPlayer::OpenClick(const char* filepath)
{
    click_.Init(filepath);
    click_.SetLooping(false);
    click_.SetPlaying(false);
}

void AudioPlayer::OpenClickAccent(const char* filepath)
{
    clickAccent_.Init(filepath);
    clickAccent_.SetLooping(false);
    clickAccent_.SetPlaying(false);
}

void AudioPlayer::Play()
{
    isPlaying_ = true;
    fadingIn_  = true;
    fadingOut_ = false;
    padA_.SetPlaying(true);
}

void AudioPlayer::Stop()
{
    fadingOut_ = true;
    fadingIn_  = false;
}

void AudioPlayer::TriggerClick(bool isAccent)
{
    if(isAccent)
        clickAccentPending_ = true;
    else
        clickPending_ = true;
}

bool AudioPlayer::IsPlaying() const
{ return isPlaying_; }

bool AudioPlayer::IsCrossfading() const
{ return crossfading_; }

// Muting the click only ever touches the gain — the click keeps running
// internally so it stays in sync with the beat and doesn't skip when unmuted.
void AudioPlayer::SetClickPlaying(bool playing)
{
    clickPlaying_ = playing;
    clickVolume_  = playing ? 0.5f : 0.0f;
}

bool AudioPlayer::IsClickPlaying() const
{ return clickPlaying_; }

void AudioPlayer::Process(float& padOut, float& clickOut)
{
    padOut   = 0.0f;
    clickOut = 0.0f;

    // ── Master fade ──────────────────────────────────────────────────────────
    if(fadingIn_)
    {
        masterGain_ += masterStep_;
        if(masterGain_ >= 1.0f)
        {
            masterGain_ = 1.0f;
            fadingIn_   = false;
        }
    }

    if(fadingOut_)
    {
        masterGain_ -= masterStep_;
        if(masterGain_ <= 0.0f)
        {
            masterGain_ = 0.0f;
            fadingOut_  = false;
            isPlaying_  = false;
            padA_.SetPlaying(false);
            padB_.SetPlaying(false);
            // NOTE: we do NOT return here — the click section still runs below
        }
    }

    // ── Pad output (only when playing) ───────────────────────────────────────
    if(isPlaying_)
    {
        if(crossfading_)
        {
            xfadeAngle_ += xfadeStep_;
            if(xfadeAngle_ >= M_PI / 2.0f)
            {
                xfadeAngle_  = M_PI / 2.0f;
                crossfading_ = false;
                useB_        = !useB_;
                gainA_       = useB_ ? 0.0f : 1.0f;
                gainB_       = useB_ ? 1.0f : 0.0f;
            }
            else
            {
                if(!useB_)
                {
                    gainA_ = cosf(xfadeAngle_);
                    gainB_ = sinf(xfadeAngle_);
                }
                else
                {
                    gainA_ = sinf(xfadeAngle_);
                    gainB_ = cosf(xfadeAngle_);
                }
            }
        }

        float samplesA[1] = {0.f};
        float samplesB[1] = {0.f};
        padA_.Stream(samplesA, 1);
        padB_.Stream(samplesB, 1);
        padOut = ((samplesA[0] * gainA_) + (samplesB[0] * gainB_)) * masterGain_
                 * padVolume_;
    }

    // ── Click output (always runs, independent of pad state) ─────────────────
    // Pending triggers are always consumed so the internal state stays in sync
    // with the beat, even when the volume is zero (muted).
    if(clickAccentPending_)
    {
        clickAccent_.Restart();
        clickAccent_.SetPlaying(true);
        clickAccentPending_ = false;
    }
    if(clickPending_)
    {
        click_.Restart();
        click_.SetPlaying(true);
        clickPending_ = false;
    }

    float cs[1] = {0.f};
    float ca[1] = {0.f};
    click_.Stream(cs, 1);
    clickAccent_.Stream(ca, 1);
    // clickVolume_ is 0 when muted, 0.5 when active — no branching needed
    clickOut = (cs[0] + ca[0]) * clickVolume_;
}