// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "LyricsDisplayModel.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>
#include <string>

class ShowLyricsAudioProcessor;

class ShowLyricsAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit ShowLyricsAudioProcessorEditor(ShowLyricsAudioProcessor &processor);
    ~ShowLyricsAudioProcessorEditor() override;

    void paint(juce::Graphics &graphics) override;
    void resized() override;

  private:
    static constexpr int refreshRateHz = 30;

    void timerCallback() override;
    [[nodiscard]] std::optional<std::string> resolveLyrics(showlyrics::ActiveCueReference cue);
    void renderDisplayState();

    ShowLyricsAudioProcessor &processor_;
    showlyrics::LyricsDisplayModel displayModel_;
    juce::Label titleLabel_;
    juce::Label statusLabel_;
    juce::TextEditor lyricsDisplay_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShowLyricsAudioProcessorEditor)
};
