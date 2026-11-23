// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginEditor.h"

#include "PluginProcessor.h"

ShowLyricsAudioProcessorEditor::ShowLyricsAudioProcessorEditor(ShowLyricsAudioProcessor &processor)
    : AudioProcessorEditor(processor), processor_(processor),
      displayModel_(showlyrics::LyricsDisplayModel::Clock::now())
{
    const auto foreground = juce::Colour::fromRGB(226, 229, 235);

    titleLabel_.setText("Show Lyrics", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(28.0F, juce::Font::bold));
    titleLabel_.setJustificationType(juce::Justification::centred);
    titleLabel_.setColour(juce::Label::textColourId, foreground);
    addAndMakeVisible(titleLabel_);

    statusLabel_.setText("Waiting for fresh transport data.", juce::dontSendNotification);
    statusLabel_.setFont(juce::FontOptions(18.0F));
    statusLabel_.setJustificationType(juce::Justification::centred);
    statusLabel_.setColour(juce::Label::textColourId, foreground);
    addAndMakeVisible(statusLabel_);

    lyricsDisplay_.setReadOnly(true);
    lyricsDisplay_.setMultiLine(true, true);
    lyricsDisplay_.setScrollbarsShown(true);
    lyricsDisplay_.setCaretVisible(false);
    lyricsDisplay_.setFont(juce::FontOptions(24.0F));
    lyricsDisplay_.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    lyricsDisplay_.setColour(juce::TextEditor::textColourId, foreground);
    lyricsDisplay_.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(lyricsDisplay_);
    lyricsDisplay_.setVisible(false);

    setResizable(true, true);
    setResizeLimits(480, 240, 1600, 1200);
    setSize(800, 450);
    startTimerHz(refreshRateHz);
}

ShowLyricsAudioProcessorEditor::~ShowLyricsAudioProcessorEditor()
{
    stopTimer();
}

void ShowLyricsAudioProcessorEditor::paint(juce::Graphics &graphics)
{
    graphics.fillAll(juce::Colour::fromRGB(24, 26, 31));
    graphics.setColour(juce::Colour::fromRGB(226, 229, 235));
}

void ShowLyricsAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(24);
    titleLabel_.setBounds(bounds.removeFromTop(60));
    statusLabel_.setBounds(bounds);
    lyricsDisplay_.setBounds(bounds);
}

void ShowLyricsAudioProcessorEditor::timerCallback()
{
    const auto snapshot = processor_.consumeLatestTransportSnapshot();
    const auto changed =
        displayModel_.update(snapshot, showlyrics::LyricsDisplayModel::Clock::now(),
                             [this](showlyrics::ActiveCueReference cue) { return resolveLyrics(cue); });
    if (changed)
        renderDisplayState();
}

std::optional<std::string> ShowLyricsAudioProcessorEditor::resolveLyrics(showlyrics::ActiveCueReference cue)
{
    std::optional<std::string> lyrics;
    const auto revisionAvailable = processor_.visitCueState(cue.revision, [&](const showlyrics::CueState &state) {
        const auto &cues = state.getCues();
        if (cue.cueIndex < cues.size())
            lyrics = cues[cue.cueIndex].getLyrics();
    });

    if (!revisionAvailable)
        return std::nullopt;
    return lyrics;
}

void ShowLyricsAudioProcessorEditor::renderDisplayState()
{
    const auto mode = displayModel_.getMode();
    if (mode == showlyrics::LyricsDisplayMode::activeCue)
    {
        const auto &lyrics = displayModel_.getLyrics();
        lyricsDisplay_.setText(juce::String::fromUTF8(lyrics.data(), static_cast<int>(lyrics.size())), false);
        statusLabel_.setVisible(false);
        lyricsDisplay_.setVisible(true);
        return;
    }

    lyricsDisplay_.setText({}, false);
    lyricsDisplay_.setVisible(false);
    statusLabel_.setVisible(true);

    switch (mode)
    {
    case showlyrics::LyricsDisplayMode::noCue:
        statusLabel_.setText("No lyric cue at the current position.", juce::dontSendNotification);
        break;
    case showlyrics::LyricsDisplayMode::stopped:
        statusLabel_.setText("Transport stopped.", juce::dontSendNotification);
        break;
    case showlyrics::LyricsDisplayMode::invalidHostPosition:
        statusLabel_.setText("Host position unavailable.", juce::dontSendNotification);
        break;
    case showlyrics::LyricsDisplayMode::mailboxOverflow:
        statusLabel_.setText("Waiting for a safe transport update.", juce::dontSendNotification);
        break;
    case showlyrics::LyricsDisplayMode::audioUnavailable:
        statusLabel_.setText("Audio processing unavailable.", juce::dontSendNotification);
        break;
    case showlyrics::LyricsDisplayMode::cueUnavailable:
        statusLabel_.setText("Lyric cue unavailable.", juce::dontSendNotification);
        break;
    case showlyrics::LyricsDisplayMode::awaitingSnapshot:
    case showlyrics::LyricsDisplayMode::staleSnapshot:
        statusLabel_.setText("Waiting for fresh transport data.", juce::dontSendNotification);
        break;
    case showlyrics::LyricsDisplayMode::activeCue:
        break;
    }
}
