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
    std::make_unique<juce::AudioParameterFloat>   ( "time", "Time", 0.040f, 1.0f, 0.300f),
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
    int maxDelay = 2000;
    auto delayMaxSamples = (int) std::round (sampleRate * maxDelay / 1000.0);
    delayBuffer.setSize(2, delayMaxSamples);
    delayBuffer.clear();
    delayBufferPosition = 0;
    globalSampleRate = (float) sampleRate;
    
    timeSmoothed.reset(sampleRate, 0.0001);
    
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

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    float gain = state.getParameter ("gain")->getValue();
    float feedback = state.getParameter ("feedback")->getValue();
    float mix = state.getParameter("mix")->getValue();
    float time = state.getParameter("time")->getValue();
    timeSmoothed.setTargetValue(time);
    float intermedio = timeSmoothed.getNextValue() * globalSampleRate * 2.0f;
    int currentTimeInSamples = (int) std::round (intermedio);
    if(currentTimeInSamples <= 1)
        currentTimeInSamples = 1;
    
    bool toggle = state.getParameter("toggle")->getValue();
    
    if (toggle) {
        
        
        if (currentTimeInSamples != oldTimeInSamples) {
            float ratio = (float)(oldTimeInSamples) / (currentTimeInSamples);
            for (int channel = 0; channel < 2; ++channel) {
                for (int i = 0; i < currentTimeInSamples - 1 ; ++i) {
                    float index = i * ratio;
                    float lower = index; // Lower index
                    float upper = lower + 1; // Upper index, ensuring it's within bounds
                    float t = index - lower; // Fractional part
                    // Linear interpolation
                    float firstValue = delayBuffer.getSample(channel, (int) std::round (lower));
                    float secondValue = delayBuffer.getSample(channel, (int) std::round (upper));
                    float interpolatedValue = firstValue + (t) * (secondValue - firstValue);
                    delayBuffer.setSample(channel, i, interpolatedValue);
                }
            }
        }
        
                
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            int delayPos = delayBufferPosition;
            
            
            for (int i=0; i< buffer.getNumSamples(); ++i) {
                float drySample = channelData[i];
                float delaySample = delayBuffer.getSample(channel, delayPos) * feedback;
                delayBuffer.setSample(channel, delayPos, drySample + delaySample);
                
                delayPos++;
                if(delayPos == currentTimeInSamples)
                    delayPos = 0;
                
                
                channelData[i] = (drySample * (1.0f - mix) + (delaySample * mix));
                channelData[i] *= gain;
            }
        }
        
        delayBufferPosition += buffer.getNumSamples();
        if (delayBufferPosition >= currentTimeInSamples)
            delayBufferPosition -= currentTimeInSamples;
        
        oldTimeInSamples = currentTimeInSamples;
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
