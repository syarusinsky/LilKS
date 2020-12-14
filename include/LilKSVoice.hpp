#ifndef LILKSVOICE_HPP
#define LILKSVOICE_HPP

/*******************************************************************************
 * A single voice for the LilKS Karplus Strong synthesizer. It uses the
 * Karplus-Strong algorithm to simulate stringed instrument plucks.
*******************************************************************************/

#include "IKeyEventListener.hpp"
#include "IBufferCallback.hpp"
#include "AudioConstants.hpp"
#include "IStorageMedia.hpp"

class LilKSVoice : public IKeyEventListener, public IBufferCallback
{
	public:
		LilKSVoice (IStorageMedia* storageMedia, unsigned int voiceNum);
		~LilKSVoice();

		void onKeyEvent (const KeyEvent& keyEvent) override;

		void call (float* writeBuffer) override;

	private:
		static const int 	m_NoiseBufferSize = static_cast<int>( SAMPLE_RATE / MUSIC_A0 );
		IStorageMedia* 		m_StorageMedia;

		unsigned int 		m_KSBufferOffset;
		unsigned int 		m_KSBufferIncr;
		unsigned int 		m_KSBufferMax;
};

#endif // LILKSVOICE_HPP
