#ifndef LILKSVOICEMANAGER_HPP
#define LILKSVOICEMANAGER_HPP

/****************************************************************************
 * The LilKSVoiceManager is responsible for processing MIDI messages and
 * setting the correct values for each LilKSVoice. This class will take
 * input from the MidiHandler and other peripheral handlers.
****************************************************************************/

#include "LilKSVoice.hpp"

#ifdef TARGET_BUILD
#define LILKS_NUM_VOICES 1
#else
#define LILKS_NUM_VOICES 6
#endif

class LilKSVoiceManager : public IBufferCallback, public IKeyEventListener
{
	public:
		LilKSVoiceManager (IStorageMedia* storageMedia);
		~LilKSVoiceManager() override;

		void onKeyEvent (const KeyEvent& keyEvent) override;

		void call (float* writeBuffer) override;

		void setMonophonic (bool monophonic);

	private:
		LilKSVoice 	m_Voices[LILKS_NUM_VOICES];
		bool 		m_Monophonic;

		KeyEvent 	m_ActiveKeyEvents[LILKS_NUM_VOICES];
		unsigned int 	m_ActiveKeyEventIndex;
};

#endif // LILKSVOICEMANAGER_HPP
