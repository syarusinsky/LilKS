/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() :
	lastInputIndex( 0 ),
	audioSettingsBtn( "Audio Settings" ),
	audioSettingsComponent( deviceManager, 2, 2, &audioSettingsBtn ),
	sAudioBuffer(),
	midiHandler(),
	midiInputList(),
	midiInputListLbl()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
	    juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                     [&] (bool granted) { if (granted)  setAudioChannels (2, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

    addAndMakeVisible( audioSettingsBtn );
    audioSettingsBtn.addListener( this );

    addAndMakeVisible( midiInputList );
    midiInputList.setTextWhenNoChoicesAvailable( "No MIDI Inputs Enabled" );
    auto midiInputs = juce::MidiInput::getDevices();
    midiInputList.addItemList( midiInputs, 1 );
    midiInputList.onChange = [this] { setMidiInput( midiInputList.getSelectedItemIndex() ); };
    // find the first enabled device and use that by default
    for ( auto midiInput : midiInputs )
    {
	    if ( deviceManager.isMidiInputEnabled(midiInput) )
	    {
		    setMidiInput( midiInputs.indexOf(midiInput) );
		    break;
	    }
    }
    // if no enabled devices were found just use the first one in the list
    if ( midiInputList.getSelectedId() == 0 )
    {
	    setMidiInput( 0 );
    }
    addAndMakeVisible( midiInputListLbl );
    midiInputListLbl.setText( "Midi Input Device", juce::dontSendNotification );
    midiInputListLbl.attachToComponent( &midiInputList, true );

    // TODO connecting the audio buffer to the voice manager
    // sAudioBuffer.registerCallback( &lilKSVoiceManager );
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    // bufferToFill.clearActiveBufferRegion();
    try
    {
	    float* writePtrR = bufferToFill.buffer->getWritePointer( 1 );
	    float* writePtrL = bufferToFill.buffer->getWritePointer( 0 );

	    for ( int i = 0; i < bufferToFill.numSamples; i++ )
	    {
		    float value = sAudioBuffer.getNextSample();
	    }
    }
    catch ( std::exception& e )
    {
	    std::cout << "Exception caught in getNextAudioBlock: " << e.what() << std::endl;
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    int sliderLeft = 120;

    audioSettingsBtn.setBounds(sliderLeft, 20, getWidth() - sliderLeft - 10, 20);
    midiInputList.setBounds   (sliderLeft, 50, getWidth() - sliderLeft - 10, 20);
}

void MainComponent::buttonClicked (juce::Button* button)
{
}

void MainComponent::setMidiInput (int index)
{
	auto list = juce::MidiInput::getDevices();

	deviceManager.removeMidiInputCallback( list[lastInputIndex], this );

	auto newInput = list[index];

	if ( !deviceManager.isMidiInputEnabled(newInput) )
	{
		deviceManager.setMidiInputEnabled( newInput, true );
	}

	deviceManager.addMidiInputCallback( newInput, this );
	midiInputList.setSelectedId( index + 1, juce::dontSendNotification );

	lastInputIndex = index;
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message)
{
	for ( int byte = 0; byte < message.getRawDataSize(); byte++ )
	{
		midiHandler.processByte( message.getRawData()[byte] );
	}

	midiHandler.dispatchEvents();
}
