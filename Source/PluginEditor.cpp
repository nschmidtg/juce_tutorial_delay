/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TutorialADCAudioProcessorEditor::TutorialADCAudioProcessorEditor (TutorialADCAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
gainAttachment(p.state, "gain", gainSlider),
feedbackAttachment(p.state, "feedback", feedbackSlider),
mixAttachment(p.state, "mix", mixSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    feedbackSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mixSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    
    for (auto* slider : { &gainSlider, &feedbackSlider, &mixSlider }){
        slider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 200, 30);
        addAndMakeVisible(slider);
    }

    setSize (512, 512);
    
}

TutorialADCAudioProcessorEditor::~TutorialADCAudioProcessorEditor()
{
}

//==============================================================================
void TutorialADCAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    background = ImageCache::getFromMemory (BinaryData::background_png, BinaryData::background_pngSize);
    g.drawImageWithin(background, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::stretchToFit);
    
    //g.setColour (juce::Colours::cyan);
    //g.fillRect (20, 20, 100, 200);
}

void TutorialADCAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    juce::Rectangle<int> bounds = getLocalBounds();
    int margin = 20;
    
    juce::Rectangle<int> gainBounds = bounds.removeFromRight(getWidth() / 3);
    gainSlider.setBounds(gainBounds.reduced(margin));
    
    juce::Rectangle<int> knobsBounds = bounds.removeFromTop (getHeight() / 2);
    juce::Rectangle<int> feedbackBounds = knobsBounds.removeFromLeft (knobsBounds.getWidth() / 2);
    feedbackSlider.setBounds(feedbackBounds.reduced(margin));
    mixSlider.setBounds(knobsBounds.reduced(margin));
    
    
}
