/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <math.h>


//==============================================================================
TutorialADCAudioProcessor::TutorialADCAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
, state (*this, nullptr, "STATE", {
    std::make_unique<juce::AudioParameterFloat> ( "gain", "Gain", 0.0f, 1.0f, 1.0f),
    std::make_unique<juce::AudioParameterFloat> ( "feedback", "Feedback", 0.0f, 1.0f, 0.35f),
    std::make_unique<juce::AudioParameterFloat> ( "mix", "Dry / Mix", 0.0f, 1.0f, 0.5f),
    std::make_unique<juce::AudioParameterFloat>   ( "time", "Time", 0.004f, 2.0f, 0.300f),
    std::make_unique<juce::AudioParameterBool> ( "toggle", "On / Off", true),
})
{
}

TutorialADCAudioProcessor::~TutorialADCAudioProcessor()
{
}

//==============================================================================
const juce::String TutorialADCAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TutorialADCAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TutorialADCAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TutorialADCAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TutorialADCAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TutorialADCAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TutorialADCAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TutorialADCAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TutorialADCAudioProcessor::getProgramName (int index)
{
    return {};
}

void TutorialADCAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TutorialADCAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    delayMaxSamples = (int) std::round(sampleRate * maxDelay);
    delayBuffer.setSize(2, delayMaxSamples);
    delayBuffer.clear();
    globalSampleRate = (float) sampleRate;
    writeHeadBuffer.resize(2);
    readHeadBuffer.resize(samplesPerBlock);
    timeSmoothed.reset(sampleRate, 0.001f);
    delaySizeBuffer.resize(samplesPerBlock);
    currentTimeInSamples = 0.3f * delayMaxSamples;
    
}

void TutorialADCAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TutorialADCAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

float cubicInterpolation(float y0, float y1, float y2, float y3, float mu) {
    float a0, a1, a2, a3;
    float mu2;

    mu2 = mu * mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;

    return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
}

void TutorialADCAudioProcessor::resampleBuffer (int initialSampleSize, int targetSampleSize)
{
    
    // Check for valid sample sizes and ensure initialSampleSize > targetSampleSize
    if (initialSampleSize <= 0 || targetSampleSize <= 0 || initialSampleSize <= targetSampleSize) {
        // Handle error: Invalid sample sizes
        return;
    }

    // Ensure lastWriteHead is within bounds of delay buffer
    lastWriteHead = (lastWriteHead + delayMaxSamples) % delayMaxSamples;
    for (int channel = 0; channel < 2; ++channel) {
        // Calculate ratio for resampling
        float ratio = static_cast<float>(initialSampleSize - 1) / static_cast<float>(targetSampleSize - 1);

        // Calculate starting and ending read indices
        int indexStart = (lastWriteHead - initialSampleSize + delayMaxSamples) % delayMaxSamples;

        // Initialize writeIndex to the starting index
        int writeIndex = (lastWriteHead - (initialSampleSize - targetSampleSize) + delayMaxSamples) % delayMaxSamples;

        // Perform resampling using linear interpolation
        for (int i = 0; i < targetSampleSize; ++i) {
            float readIndex = indexStart + i * ratio;

            // Calculate the neighboring indices for interpolation
            int xlow1 = static_cast<int>(std::floor(readIndex)) - 1;
            int xlow2 = static_cast<int>(std::floor(readIndex));
            int xhigh1 = static_cast<int>(std::ceil(readIndex));
            int xhigh2 = static_cast<int>(std::ceil(readIndex)) + 1;

            // Wrap indices around the circular buffer
            xlow1 = (xlow1 + delayMaxSamples) % delayMaxSamples;
            xlow2 = (xlow2 + delayMaxSamples) % delayMaxSamples;
            xhigh1 = (xhigh1 + delayMaxSamples) % delayMaxSamples;
            xhigh2 = (xhigh2 + delayMaxSamples) % delayMaxSamples;

            // Perform cubic interpolation
            float ylow1 = delayBuffer.getSample(channel, xlow1);
            float ylow2 = delayBuffer.getSample(channel, xlow2);
            float yhigh1 = delayBuffer.getSample(channel, xhigh1);
            float yhigh2 = delayBuffer.getSample(channel, xhigh2);

            float mu = (readIndex - xlow1) / (xhigh2 - xlow1);
            float yInterpolated = cubicInterpolation(ylow1, ylow2, yhigh1, yhigh2, mu);

            // Write interpolated sample to delay buffer
            delayBuffer.setSample(channel, writeIndex, yInterpolated);

            // Move writeIndex to the next position
            writeIndex = (writeIndex + 1) % delayMaxSamples;
        }
    }
    
}

void TutorialADCAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    float gain = state.getParameter("gain")->getValue();
    float feedback = state.getParameter("feedback")->getValue();
    float mix = state.getParameter("mix")->getValue();
    float time = state.getParameter("time")->getValue(); // Use getNextValue directly for smoother updates
    timeSmoothed.setTargetValue(time);
    
    int currentTimeInSamples = static_cast<int>(timeSmoothed.getNextValue() * delayMaxSamples); // Calculate current time in samples directly

    // Resample the delay buffer if the time parameter has changed
    if (oldTimeInSamples != currentTimeInSamples)
        resampleBuffer(oldTimeInSamples, currentTimeInSamples);

    oldTimeInSamples = currentTimeInSamples; // Update oldTimeInSamples

    // Iterate over each channel
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        int writeIndex = writeHeadBuffer[channel];

        // Iterate over each sample in the buffer
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Get the read index based on the current time
            int readIndex = (writeIndex - currentTimeInSamples + delayMaxSamples) % delayMaxSamples;
            float delaySample = delayBuffer.getSample(channel, readIndex); // Get the delay sample

            // Update delay buffer with input sample and feedback
            delayBuffer.setSample(channel, writeIndex, channelData[i] + (delaySample * feedback));

            // Apply wet/dry mix
            channelData[i] = (channelData[i] * (1.0f - mix)) + (delaySample * mix);

            // Apply gain
            channelData[i] *= gain;

            // Update write index
            writeIndex = (writeIndex + 1) % delayMaxSamples;
        }

        // Update write head buffer
        writeHeadBuffer[channel] = writeIndex;
    }
}

//==============================================================================
bool TutorialADCAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TutorialADCAudioProcessor::createEditor()
{
    return new TutorialADCAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void TutorialADCAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    if (auto xmlState = state.copyState().createXml())
        copyXmlToBinary(*xmlState, destData);
}


void TutorialADCAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        state.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TutorialADCAudioProcessor();
}
