#ifndef LILKSVOICE_HPP
#define LILKSVOICE_HPP

/*******************************************************************************
 * A single voice for the LilKS Karplus Strong synthesizer. It uses the
 * Karplus-Strong algorithm to simulate stringed instrument plucks.
*******************************************************************************/

#include "IKeyEventListener.hpp"
#include "IBufferCallback.hpp"
#include "AudioConstants.hpp"

class LilKSVoice : public IKeyEventListener, public IBufferCallback
{
	public:
		LilKSVoice();
		~LilKSVoice();

		void onKeyEvent (const KeyEvent& keyEvent) override;

		void call (float* writeBuffer) override;

	private:
		static const int 	m_NoiseBufferSize = static_cast<int>( SAMPLE_RATE / MUSIC_A0 );
		static float 		m_NoiseBuffer[m_NoiseBufferSize];

		float 			m_KSBuffer[m_NoiseBufferSize];

		unsigned int 		m_KSBufferIncr;
		unsigned int 		m_KSBufferMax;
};

#endif // LILKSVOICE_HPP
