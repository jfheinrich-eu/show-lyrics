// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class ShowLyricsAudioProcessor;

class ShowLyricsAudioProcessorEditor final : public juce::AudioProcessorEditor
{
  public:
    explicit ShowLyricsAudioProcessorEditor(ShowLyricsAudioProcessor &processor);

    void paint(juce::Graphics &graphics) override;
    void resized() override;

  private:
    juce::Label titleLabel_;
    juce::Label statusLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShowLyricsAudioProcessorEditor)
};
