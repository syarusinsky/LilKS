#include "LilKSVoiceManager.hpp"

#include <cstdlib>
#include <string.h>

LilKSVoiceManager::LilKSVoiceManager (IStorageMedia* storageMedia) :
	m_Voices{ LilKSVoice(storageMedia, 1), LilKSVoice(storageMedia, 2), LilKSVoice(storageMedia, 3),
			LilKSVoice(storageMedia, 4), LilKSVoice(storageMedia, 5), LilKSVoice(storageMedia, 6) },
	m_Monophonic( false ),
	m_ActiveKeyEventIndex( 0 )
{
}

LilKSVoiceManager::~LilKSVoiceManager()
{
}

void LilKSVoiceManager::onKeyEvent (const KeyEvent& keyEvent)
{
	if ( ! m_Monophonic ) // polyphonic implementation
	{
		if ( keyEvent.pressed() == KeyPressedEnum::PRESSED )
		{
			bool containsKeyEvent = false;
			for (unsigned int voice = 0; voice < LILKS_NUM_VOICES; voice++)
			{
				if ( m_ActiveKeyEvents[voice].isNoteAndType( keyEvent ) )
				{
					containsKeyEvent = true;
					m_ActiveKeyEvents[voice] = keyEvent;
					m_Voices[voice].onKeyEvent(keyEvent);

					return;
				}
			}

			if ( ! containsKeyEvent )
			{
				// ensure we aren't overwriting a pressed key
				unsigned int initialActiveKeyEventIndex = m_ActiveKeyEventIndex;
				while ( m_ActiveKeyEvents[m_ActiveKeyEventIndex].pressed() == KeyPressedEnum::PRESSED )
				{
					m_ActiveKeyEventIndex = (m_ActiveKeyEventIndex + 1) % LILKS_NUM_VOICES;

					if ( m_ActiveKeyEventIndex == initialActiveKeyEventIndex )
					{
						break;
					}
				}
				m_ActiveKeyEvents[m_ActiveKeyEventIndex] = keyEvent;
				m_Voices[m_ActiveKeyEventIndex].onKeyEvent(keyEvent);

				m_ActiveKeyEventIndex = (m_ActiveKeyEventIndex + 1) % LILKS_NUM_VOICES;

				return;
			}
		}
		else if ( keyEvent.pressed() == KeyPressedEnum::RELEASED )
		{
			for (unsigned int voice = 0; voice < LILKS_NUM_VOICES; voice++)
			{
				if ( m_ActiveKeyEvents[voice].isNoteAndType( keyEvent, KeyPressedEnum::PRESSED ) )
				{
					m_ActiveKeyEvents[voice] = keyEvent;
					m_Voices[voice].onKeyEvent( keyEvent );

					return;
				}
			}
		}
	}
	else // monophonic implementation 	TODO needs to be debugged!
	{
		if ( keyEvent.pressed() == KeyPressedEnum::PRESSED )
		{
			// if a key is currently playing
			KeyPressedEnum activeKeyPressed = m_ActiveKeyEvents[0].pressed();
			if ( activeKeyPressed == KeyPressedEnum::PRESSED || activeKeyPressed == KeyPressedEnum::HELD )
			{
				// build a 'held' key event, since we don't want to retrigger the envelope generator
				KeyEvent newKeyEvent( KeyPressedEnum::HELD, keyEvent.note(), keyEvent.velocity() );

				if ( m_ActiveKeyEvents[0].note() < newKeyEvent.note() )
				{
					KeyEvent oldKeyEvent( KeyPressedEnum::HELD, m_ActiveKeyEvents[0].note(), m_ActiveKeyEvents[0].velocity() );

					// look for a place to store the old key event, since we only want to play the highest note
					for (unsigned int voice = 1; voice < LILKS_NUM_VOICES; voice++)
					{
						if ( m_ActiveKeyEvents[voice].pressed() == KeyPressedEnum::RELEASED )
						{
							m_ActiveKeyEvents[voice] = oldKeyEvent;
							break;
						}
					}

					m_ActiveKeyEvents[0] = newKeyEvent;
					m_Voices[0].onKeyEvent( newKeyEvent );

					return;
				}
				else if ( m_ActiveKeyEvents[0].note() > newKeyEvent.note() )
				{
					// look for a place to store this key event, since we only want to play the highest note
					for (unsigned int voice = 1; voice < LILKS_NUM_VOICES; voice++)
					{
						if ( m_ActiveKeyEvents[voice].pressed() == KeyPressedEnum::RELEASED )
						{
							m_ActiveKeyEvents[voice] = newKeyEvent;
							break;
						}
					}

					return;
				}
			}
			else // there is no note currently active
			{
				m_ActiveKeyEvents[0] = keyEvent;
				m_Voices[0].onKeyEvent( keyEvent );

				return;
			}
		}
		else if ( keyEvent.pressed() == KeyPressedEnum::RELEASED )
		{
			// look for this note in the active key events array
			for (unsigned int voice = 0; voice < LILKS_NUM_VOICES; voice++)
			{
				// if there is a note that matches and isn't released
				KeyPressedEnum voiceKeyPressed = m_ActiveKeyEvents[voice].pressed();
				unsigned int voiceKeyNote = m_ActiveKeyEvents[voice].note();
				if ( voiceKeyPressed != KeyPressedEnum::RELEASED && voiceKeyNote == keyEvent.note() )
				{
					// if the main active voice is released, replace with lower note
					if (voice == 0)
					{
						int highestNote = -1; // negative 1 means no highest note found
						for (unsigned int voice2 = 1; voice2 < LILKS_NUM_VOICES; voice2++)
						{
							KeyPressedEnum voice2KeyPressed = m_ActiveKeyEvents[voice2].pressed();
							int voice2KeyNote = m_ActiveKeyEvents[voice2].note();
							if ( voice2KeyPressed == KeyPressedEnum::HELD && voice2KeyNote > highestNote )
							{
								highestNote = voice2;
							}
						}

						if ( highestNote > 0 ) // if an active lower key is found
						{
							// store the lower key
							KeyEvent newActiveKeyEvent = m_ActiveKeyEvents[highestNote];

							// replace the lower key with a released key event
							unsigned int keyNote = m_ActiveKeyEvents[highestNote].note();
							unsigned int keyVelocity = m_ActiveKeyEvents[highestNote].velocity();
							KeyEvent inactiveKeyEvent( KeyPressedEnum::RELEASED, keyNote, keyVelocity );
							m_ActiveKeyEvents[highestNote] = inactiveKeyEvent;

							// replace the currently active note with the lower key
							m_ActiveKeyEvents[0] = newActiveKeyEvent;
							m_Voices[0].onKeyEvent( newActiveKeyEvent );

							return;
						}
						else // if there are no active lower keys
						{
							m_ActiveKeyEvents[0] = keyEvent;
							m_Voices[0].onKeyEvent( keyEvent );

							return;
						}
					}
					else // if one of the lower notes is released, replace with released key event
					{
						m_ActiveKeyEvents[voice] = keyEvent;

						return;
					}
				}
			}
		}
	}
}

void LilKSVoiceManager::call (float* writeBuffer)
{
	// first clear write buffer
	memset( writeBuffer, 0, sizeof(float) * ABUFFER_SIZE );

	// write the contents of each voice to the audio buffer
	for ( unsigned int voice = 0; voice < LILKS_NUM_VOICES; voice++ )
	{
		m_Voices[voice].call( writeBuffer );
	}
}

void LilKSVoiceManager::setMonophonic (bool monophonic)
{
	m_Monophonic = monophonic;
}
