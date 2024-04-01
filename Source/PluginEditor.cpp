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
    
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    gainLabel.setText("Gain", juce::dontSendNotification);
    addAndMakeVisible(gainLabel);
    
    
    feedbackSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    feedbackLabel.setText("Feedback", juce::dontSendNotification);
    addAndMakeVisible(feedbackLabel);
    
    mixSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mixLabel.setText("Mix", juce::dontSendNotification);
    addAndMakeVisible(mixLabel);
    
    for (auto* slider : { &gainSlider, &feedbackSlider, &mixSlider }){
        slider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 50, 30);
        addAndMakeVisible(slider);
    }

    setSize (696, 613);
    
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
    int knobSize = 192;
    //int proportion = 2;
    
    //juce::Rectangle<int> topRow = bounds.removeFromTop(getHeight() / proportion + 30);
    //juce::Rectangle<int> firstElement = topRow.removeFromRight(getWidth() / proportion - 40);
    //juce::Rectangle<int> secondElement = topRow.removeFromLeft(getWidth() /proportion);
    
    //juce::Rectangle<int> gainBounds = bounds.removeFromLeft(getWidth() / 4);
    
    gainSlider.setBounds(66, 69, knobSize, knobSize);
    gainLabel.setBounds(gainSlider.getBounds().removeFromBottom(30));
    
    
    mixSlider.setBounds(getWidth()-61-knobSize, 69, knobSize, knobSize);
    mixLabel.setBounds(mixSlider.getBounds().removeFromBottom(30));
    
    
    feedbackSlider.setBounds(66, getHeight() - 91 - knobSize, knobSize, knobSize);
    feedbackLabel.setBounds(feedbackSlider.getBounds().removeFromBottom(30));
    
    //juce::Rectangle<int> knobsBounds = bounds.removeFromTop (getHeight() / 2);
    //juce::Rectangle<int> feedbackBounds = knobsBounds.removeFromLeft (knobsBounds.getWidth() / 2);
    //feedbackSlider.setBounds(secondElement.reduced(margin));
    //juce::Rectangle<int> bottomRow = bounds.removeFromBottom(getHeight() / proportion + 100);
    //juce::Rectangle<int> firstBottomElement = bottomRow.removeFromLeft(getWidth() / proportion);
    
    //mixSlider.setBounds(firstBottomElement.reduced(margin));
    
    
}
