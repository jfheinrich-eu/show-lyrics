// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginEditor.h"

#include "PluginProcessor.h"

ShowLyricsAudioProcessorEditor::ShowLyricsAudioProcessorEditor(ShowLyricsAudioProcessor &processor)
    : AudioProcessorEditor(processor)
{
    titleLabel_.setText("Show Lyrics", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(28.0F, juce::Font::bold));
    titleLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel_);

    statusLabel_.setText("No lyric cues configured.", juce::dontSendNotification);
    statusLabel_.setFont(juce::FontOptions(18.0F));
    statusLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel_);

    setResizable(true, true);
    setResizeLimits(480, 240, 1600, 1200);
    setSize(800, 450);
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
}
