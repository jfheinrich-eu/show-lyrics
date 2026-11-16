// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginProcessor.h"

#include "PluginEditor.h"

ShowLyricsAudioProcessor::ShowLyricsAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void ShowLyricsAudioProcessor::prepareToPlay(double, int)
{
}

void ShowLyricsAudioProcessor::releaseResources()
{
}

bool ShowLyricsAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    const auto supportedInput = input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo();
    return supportedInput && input == output;
}

void ShowLyricsAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor *ShowLyricsAudioProcessor::createEditor()
{
    return new ShowLyricsAudioProcessorEditor(*this);
}

bool ShowLyricsAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String ShowLyricsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ShowLyricsAudioProcessor::acceptsMidi() const
{
    return false;
}

bool ShowLyricsAudioProcessor::producesMidi() const
{
    return false;
}

bool ShowLyricsAudioProcessor::isMidiEffect() const
{
    return false;
}

double ShowLyricsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ShowLyricsAudioProcessor::getNumPrograms()
{
    return 1;
}

int ShowLyricsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ShowLyricsAudioProcessor::setCurrentProgram(int)
{
}

const juce::String ShowLyricsAudioProcessor::getProgramName(int)
{
    return "Default";
}

void ShowLyricsAudioProcessor::changeProgramName(int, const juce::String &)
{
}

void ShowLyricsAudioProcessor::getStateInformation(juce::MemoryBlock &destinationData)
{
    const juce::ValueTree state("ShowLyricsState");
    juce::MemoryOutputStream stream(destinationData, false);
    state.writeToStream(stream);
}

void ShowLyricsAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    const auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (!state.isValid() || !state.hasType("ShowLyricsState"))
        return;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new ShowLyricsAudioProcessor();
}
