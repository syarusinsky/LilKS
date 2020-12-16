/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "AudioSettingsComponent.h"
#include "AudioBuffer.hpp"
#include "MidiHandler.hpp"
#include "FakeStorageDevice.hpp"
#include "LilKSVoiceManager.hpp"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public juce::AudioAppComponent, public juce::Button::Listener, public juce::MidiInputCallback
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;

    void setMidiInput (int index);
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage &message) override;

private:
    //==============================================================================
    // Your private member variables go here...

    int 			lastInputIndex;

    juce::TextButton 		audioSettingsBtn;
    AudioSettingsComponent 	audioSettingsComponent;

    ::AudioBuffer 		sAudioBuffer;
    MidiHandler 		midiHandler;

    FakeStorageDevice 		fakeStorageDevice;

    LilKSVoiceManager 		lilKSVoiceManager;

    juce::ComboBox 		midiInputList;
    juce::Label 		midiInputListLbl;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
