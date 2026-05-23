#pragma once

class Metronome
{
  private:
    static const int TAP_BUFFER_SIZE = 4;

    int          bpm_;
    int          tapCount_;
    unsigned int lastTapTime_;
    unsigned int lastInterval_;
    unsigned int intervals_[TAP_BUFFER_SIZE];

  public:
    Metronome() : bpm_(120), tapCount_(0), lastTapTime_(0), lastInterval_(0)
    {
        for(int i = 0; i < TAP_BUFFER_SIZE; i++)
            intervals_[i] = 0;
    }

    void Tap(unsigned int currentTimeMs)
    {
        if(lastTapTime_ > 0)
        {
            unsigned int interval = currentTimeMs - lastTapTime_;
            lastInterval_         = interval;

            if(interval >= 250 && interval <= 1500)
            {
                intervals_[tapCount_ % TAP_BUFFER_SIZE] = interval;
                tapCount_++;

                unsigned int sum   = 0;
                int          count = 0;
                for(int i = 0; i < TAP_BUFFER_SIZE; i++)
                {
                    if(intervals_[i] > 0)
                    {
                        sum += intervals_[i];
                        count++;
                    }
                }
                if(count > 0)
                    bpm_ = 60000 / (sum / count);
            }
            else
            {
                tapCount_ = 0;
                for(int i = 0; i < TAP_BUFFER_SIZE; i++)
                    intervals_[i] = 0;
                lastTapTime_ = currentTimeMs;
                return;
            }
        }
        else
        {
            tapCount_ = 1;
        }
        lastTapTime_ = currentTimeMs;
    };

    int          GetBpm() const { return bpm_; }
    void         SetBpm(int bpm) { bpm_ = bpm; }
    unsigned int GetIntervalMs() const { return 60000 / bpm_; }
    unsigned int GetLastInterval() const { return lastInterval_; }
};