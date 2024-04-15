/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class TutorialADCAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    TutorialADCAudioProcessor();
    ~TutorialADCAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;
    void resampleBuffer (int initialSampleSize, int targetSampleSize);
private:
    //==============================================================================
    int delayWritePosition = 0;
    juce::AudioBuffer<float> delayBuffer;
    float globalSampleRate = 44100;
    int oldTimeInSamples = 44100;
    juce::LinearSmoothedValue<float> timeSmoothed { 0.3f };
    int delayMaxSamples;
    int delayRead = 0;
    int delayWrite = 0;
    std::vector<int> writeHeadBuffer;
    std::vector<int> readHeadBuffer;
    int lastWriteHead = 0;
    int lastReadHead = 0;
    int currentTimeInSamples = 44100;
    std::vector<int> delaySizeBuffer;
    int maxDelay = 2;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TutorialADCAudioProcessor)
};
