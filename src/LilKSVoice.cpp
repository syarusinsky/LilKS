#include "LilKSVoice.hpp"

#include "MidiConstants.hpp"

#include <cstdlib>
#include <string.h>

LilKSVoice::LilKSVoice (IStorageMedia* storageMedia, unsigned int voiceNum ) :
	m_StorageMedia( storageMedia ),
	m_KSBufferOffset( m_NoiseBufferSize * sizeof(float) * voiceNum ),
	m_KSBufferIncr( 0 ),
	m_KSBufferMax( m_NoiseBufferSize )
{
	// fill noise buffer 	NOTE: this means the noise buffer will be written to for each voice that's initialized
	// 			which is dumb, but whatever
	SharedData<uint8_t> noiseBuffer = SharedData<uint8_t>::MakeSharedData( m_NoiseBufferSize * sizeof(float) );
	float* noiseBufferPtr = reinterpret_cast<float*>( noiseBuffer.getPtr() );
	for ( int sample = 0; sample < m_NoiseBufferSize; sample++ )
	{
		noiseBufferPtr[sample] = static_cast<float>( rand() ) / static_cast<float>( RAND_MAX );
	}

	// write the noise buffer to the storage device
	m_StorageMedia->writeToMedia( noiseBuffer, 0 );
}

LilKSVoice::~LilKSVoice()
{
}

void LilKSVoice::onKeyEvent (const KeyEvent& keyEvent)
{
	if ( keyEvent.pressed() == KeyPressedEnum::PRESSED )
	{
		float frequency = MUSIC_A0;
		switch ( keyEvent.note() )
		{
			case MIDI_NOTE_A0:
				frequency = MUSIC_A0;
				break;
			case MIDI_NOTE_BB0:
				frequency = MUSIC_BB0;
				break;
			case MIDI_NOTE_B0:
				frequency = MUSIC_B0;
				break;
			case MIDI_NOTE_C1:
				frequency = MUSIC_C1;
				break;
			case MIDI_NOTE_DB1:
				frequency = MUSIC_DB1;
				break;
			case MIDI_NOTE_D1:
				frequency = MUSIC_D1;
				break;
			case MIDI_NOTE_EB1:
				frequency = MUSIC_EB1;
				break;
			case MIDI_NOTE_E1:
				frequency = MUSIC_E1;
				break;
			case MIDI_NOTE_F1:
				frequency = MUSIC_F1;
				break;
			case MIDI_NOTE_GB1:
				frequency = MUSIC_GB1;
				break;
			case MIDI_NOTE_G1:
				frequency = MUSIC_GB1;
				break;
			case MIDI_NOTE_AB1:
				frequency = MUSIC_AB1;
				break;
			case MIDI_NOTE_A1:
				frequency = MUSIC_A1;
				break;
			case MIDI_NOTE_BB1:
				frequency = MUSIC_BB1;
				break;
			case MIDI_NOTE_B1:
				frequency = MUSIC_B1;
				break;
			case MIDI_NOTE_C2:
				frequency = MUSIC_C2;
				break;
			case MIDI_NOTE_DB2:
				frequency = MUSIC_DB2;
				break;
			case MIDI_NOTE_D2:
				frequency = MUSIC_D2;
				break;
			case MIDI_NOTE_EB2:
				frequency = MUSIC_EB2;
				break;
			case MIDI_NOTE_E2:
				frequency = MUSIC_E2;
				break;
			case MIDI_NOTE_F2:
				frequency = MUSIC_F2;
				break;
			case MIDI_NOTE_GB2:
				frequency = MUSIC_GB2;
				break;
			case MIDI_NOTE_G2:
				frequency = MUSIC_G2;
				break;
			case MIDI_NOTE_AB2:
				frequency = MUSIC_AB2;
				break;
			case MIDI_NOTE_A2:
				frequency = MUSIC_A2;
				break;
			case MIDI_NOTE_BB2:
				frequency = MUSIC_BB2;
				break;
			case MIDI_NOTE_B2:
				frequency = MUSIC_B2;
				break;
			case MIDI_NOTE_C3:
				frequency = MUSIC_C3;
				break;
			case MIDI_NOTE_DB3:
				frequency = MUSIC_DB3;
				break;
			case MIDI_NOTE_D3:
				frequency = MUSIC_D3;
				break;
			case MIDI_NOTE_EB3:
				frequency = MUSIC_EB3;
				break;
			case MIDI_NOTE_E3:
				frequency = MUSIC_E3;
				break;
			case MIDI_NOTE_F3:
				frequency = MUSIC_F3;
				break;
			case MIDI_NOTE_GB3:
				frequency = MUSIC_GB3;
				break;
			case MIDI_NOTE_G3:
				frequency = MUSIC_G3;
				break;
			case MIDI_NOTE_AB3:
				frequency = MUSIC_AB3;
				break;
			case MIDI_NOTE_A3:
				frequency = MUSIC_A3;
				break;
			case MIDI_NOTE_BB3:
				frequency = MUSIC_BB3;
				break;
			case MIDI_NOTE_B3:
				frequency = MUSIC_B3;
				break;
			case MIDI_NOTE_C4:
				frequency = MUSIC_C4;
				break;
			case MIDI_NOTE_DB4:
				frequency = MUSIC_DB4;
				break;
			case MIDI_NOTE_D4:
				frequency = MUSIC_D4;
				break;
			case MIDI_NOTE_EB4:
				frequency = MUSIC_EB4;
				break;
			case MIDI_NOTE_E4:
				frequency = MUSIC_E4;
				break;
			case MIDI_NOTE_F4:
				frequency = MUSIC_F4;
				break;
			case MIDI_NOTE_GB4:
				frequency = MUSIC_GB4;
				break;
			case MIDI_NOTE_G4:
				frequency = MUSIC_G4;
				break;
			case MIDI_NOTE_AB4:
				frequency = MUSIC_AB4;
				break;
			case MIDI_NOTE_A4:
				frequency = MUSIC_A4;
				break;
			case MIDI_NOTE_BB4:
				frequency = MUSIC_BB4;
				break;
			case MIDI_NOTE_B4:
				frequency = MUSIC_B4;
				break;
			case MIDI_NOTE_C5:
				frequency = MUSIC_C5;
				break;
			case MIDI_NOTE_DB5:
				frequency = MUSIC_DB5;
				break;
			case MIDI_NOTE_D5:
				frequency = MUSIC_D5;
				break;
			case MIDI_NOTE_EB5:
				frequency = MUSIC_EB5;
				break;
			case MIDI_NOTE_E5:
				frequency = MUSIC_E5;
				break;
			case MIDI_NOTE_F5:
				frequency = MUSIC_F5;
				break;
			case MIDI_NOTE_GB5:
				frequency = MUSIC_GB5;
				break;
			case MIDI_NOTE_G5:
				frequency = MUSIC_G5;
				break;
			case MIDI_NOTE_AB5:
				frequency = MUSIC_AB5;
				break;
			case MIDI_NOTE_A5:
				frequency = MUSIC_A5;
				break;
			case MIDI_NOTE_BB5:
				frequency = MUSIC_BB5;
				break;
			case MIDI_NOTE_B5:
				frequency = MUSIC_B5;
				break;
			case MIDI_NOTE_C6:
				frequency = MUSIC_C6;
				break;
			case MIDI_NOTE_DB6:
				frequency = MUSIC_DB6;
				break;
			case MIDI_NOTE_D6:
				frequency = MUSIC_D6;
				break;
			case MIDI_NOTE_EB6:
				frequency = MUSIC_EB6;
				break;
			case MIDI_NOTE_E6:
				frequency = MUSIC_E6;
				break;
			case MIDI_NOTE_F6:
				frequency = MUSIC_F6;
				break;
			case MIDI_NOTE_GB6:
				frequency = MUSIC_GB6;
				break;
			case MIDI_NOTE_G6:
				frequency = MUSIC_G6;
				break;
			case MIDI_NOTE_AB6:
				frequency = MUSIC_AB6;
				break;
			case MIDI_NOTE_A6:
				frequency = MUSIC_A6;
				break;
			case MIDI_NOTE_BB6:
				frequency = MUSIC_BB6;
				break;
			case MIDI_NOTE_B6:
				frequency = MUSIC_B6;
				break;
			case MIDI_NOTE_C7:
				frequency = MUSIC_C7;
				break;
			case MIDI_NOTE_DB7:
				frequency = MUSIC_DB7;
				break;
			case MIDI_NOTE_D7:
				frequency = MUSIC_D7;
				break;
			case MIDI_NOTE_EB7:
				frequency = MUSIC_EB7;
				break;
			case MIDI_NOTE_E7:
				frequency = MUSIC_E7;
				break;
			case MIDI_NOTE_F7:
				frequency = MUSIC_F7;
				break;
			case MIDI_NOTE_GB7:
				frequency = MUSIC_GB7;
				break;
			case MIDI_NOTE_G7:
				frequency = MUSIC_G7;
				break;
			case MIDI_NOTE_AB7:
				frequency = MUSIC_AB7;
				break;
			case MIDI_NOTE_A7:
				frequency = MUSIC_A7;
				break;
			case MIDI_NOTE_BB7:
				frequency = MUSIC_BB7;
				break;
			case MIDI_NOTE_B7:
				frequency = MUSIC_B7;
				break;
			case MIDI_NOTE_C8:
				frequency = MUSIC_C8;
				break;
			case MIDI_NOTE_DB8:
				frequency = MUSIC_DB8;
				break;
			case MIDI_NOTE_D8:
				frequency = MUSIC_D8;
				break;
			case MIDI_NOTE_EB8:
				frequency = MUSIC_EB8;
				break;
			case MIDI_NOTE_E8:
				frequency = MUSIC_E8;
				break;
			case MIDI_NOTE_F8:
				frequency = MUSIC_F8;
				break;
			case MIDI_NOTE_GB8:
				frequency = MUSIC_GB8;
				break;
			case MIDI_NOTE_G8:
				frequency = MUSIC_G8;
				break;
			case MIDI_NOTE_AB8:
				frequency = MUSIC_AB8;
				break;
			case MIDI_NOTE_A8:
				frequency = MUSIC_A8;
				break;
			case MIDI_NOTE_BB8:
				frequency = MUSIC_BB8;
				break;
			case MIDI_NOTE_B8:
				frequency = MUSIC_B8;
				break;
			case MIDI_NOTE_C9:
				frequency = MUSIC_C9;
				break;
			case MIDI_NOTE_DB9:
				frequency = MUSIC_DB9;
				break;
			case MIDI_NOTE_D9:
				frequency = MUSIC_D9;
				break;
			case MIDI_NOTE_EB9:
				frequency = MUSIC_EB9;
				break;
			case MIDI_NOTE_E9:
				frequency = MUSIC_E9;
				break;
			case MIDI_NOTE_F9:
				frequency = MUSIC_F9;
				break;
			case MIDI_NOTE_GB9:
				frequency = MUSIC_GB9;
				break;
			case MIDI_NOTE_G9:
				frequency = MUSIC_G9;
				break;
			default:
				frequency = MUSIC_A0;
		}

		// reset the buffer size to the correct size for the frequency
		m_KSBufferMax = static_cast<int>( SAMPLE_RATE / frequency );

		// retrieve the noise buffer from the storage media
		SharedData<uint8_t> noiseBuffer = m_StorageMedia->readFromMedia( m_NoiseBufferSize * sizeof(float), 0 );
		float* noiseBufferPtr = reinterpret_cast<float*>( noiseBuffer.getPtr() );

		// retrieve the KS buffer from the storage media
		SharedData<uint8_t> ksBuffer = m_StorageMedia->readFromMedia( m_NoiseBufferSize * sizeof(float), m_KSBufferOffset );
		float* ksBufferPtr = reinterpret_cast<float*>( ksBuffer.getPtr() );

		// refill the buffer with noise
		for ( unsigned int sample = 0; sample < m_KSBufferMax; sample++ )
		{
			ksBufferPtr[sample] = noiseBufferPtr[sample];
		}

		// rewrite the KS buffer to the storage media
		m_StorageMedia->writeToMedia( ksBuffer, m_KSBufferOffset );
	}
}

void LilKSVoice::call (float* writeBuffer)
{
	// first clear write buffer
	memset( writeBuffer, 0, sizeof(float) * ABUFFER_SIZE );

	// retrieve the KS buffer from the storage media
	SharedData<uint8_t> ksBuffer = m_StorageMedia->readFromMedia( m_NoiseBufferSize * sizeof(float), m_KSBufferOffset );
	float* ksBufferPtr = reinterpret_cast<float*>( ksBuffer.getPtr() );

	// fill the buffer with samples from this voice
	for ( unsigned int sample = 0; sample < ABUFFER_SIZE; sample++ )
	{
		writeBuffer[sample] = ksBufferPtr[m_KSBufferIncr];
		m_KSBufferIncr = ( m_KSBufferIncr + 1 ) % m_KSBufferMax;

		ksBufferPtr[m_KSBufferIncr] = ( writeBuffer[sample] + ksBufferPtr[m_KSBufferIncr] ) * 0.5f;
	}

	// rewrite the KS buffer to the storage media
	m_StorageMedia->writeToMedia( ksBuffer, m_KSBufferOffset );
}
