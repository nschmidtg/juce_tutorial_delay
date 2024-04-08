/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


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
    int maxDelay = 2; //seconds
    delayMaxSamples = static_cast<int>(sampleRate * maxDelay);
    delayBuffer.setSize(2, delayMaxSamples);
    delayBuffer.clear();
    globalSampleRate = (float) sampleRate;
    writeHeadBuffer.resize(samplesPerBlock);
    readHeadBuffer.resize(samplesPerBlock);
    timeSmoothed.reset(sampleRate, 0.1f);
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

void TutorialADCAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float gain = state.getParameter ("gain")->getValue();
    float feedback = state.getParameter ("feedback")->getValue();
    float mix = state.getParameter("mix")->getValue();
    float time = state.getParameter("time")->getValue();
    timeSmoothed.setTargetValue(time);


    int currentTimeInSamples = std::round(timeSmoothed.getNextValue() * globalSampleRate * 2.0f);
        
    for(int i = 0; i<buffer.getNumSamples(); ++i){
        delaySizeBuffer[i] = std::round(timeSmoothed.getNextValue() * globalSampleRate * 2.0f);
        
        lastWriteHead = (lastWriteHead + 1) % currentTimeInSamples;
        writeHeadBuffer[i] = lastWriteHead;
        
        //readHeadBuffer[i] = lastReadHead % currentTimeInSamples;
        //lastReadHead += 1;
    }
    
    
    
    //float ratio = ((float) oldTimeInSamples - 1) / ((float) timeSmoothedArray[0] - 1);
            
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        
        for (int i=0; i < buffer.getNumSamples(); ++i) {
            int wIndex = writeHeadBuffer[i];
            int rIndex = (wIndex + 1) % currentTimeInSamples;
    
            float delaySample = delayBuffer.getSample(channel, rIndex);
            float drySample = channelData[i];
            delayBuffer.setSample(channel, wIndex, drySample + (delaySample * feedback));
            
            channelData[i] = (drySample * (1.0f - mix) + (delaySample * mix));
            channelData[i] *= gain;
        }
    }

    oldTimeInSamples = currentTimeInSamples;

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
