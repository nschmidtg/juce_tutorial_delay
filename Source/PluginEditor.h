/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class TutorialADCAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    TutorialADCAudioProcessorEditor (TutorialADCAudioProcessor&);
    ~TutorialADCAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    TutorialADCAudioProcessor& audioProcessor;
    
    juce::Image background;
    juce::Slider gainSlider, feedbackSlider, mixSlider, timeSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment, feedbackAttachment, mixAttachment, timeAttachment;
    juce::Label gainLabel, mixLabel, feedbackLabel, timeLabel;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TutorialADCAudioProcessorEditor)
};
