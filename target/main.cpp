#include "../lib/STM32f302x8-HAL/llpd/include/LLPD.hpp"

#include <math.h>

#include "EEPROM_CAT24C64.hpp"
#include "SRAM_23K256.hpp"

#include "AudioBuffer.hpp"
#include "MidiHandler.hpp"
#include "LilKSVoiceManager.hpp"

// to disassemble -- arm-none-eabi-objdump -S --disassemble main_debug.elf > disassembled.s

#define SYS_CLOCK_FREQUENCY 32000000

// global variables
volatile bool ledState = false; // led state: true for on, false for off
volatile bool keepBlinking = true; // a test variable that determines whether or not to flash the led
volatile bool adcSetupComplete = false; // should be set to true after adc has been initialized
volatile int ledIncr = 0; // should flash led every time this value is equal to ledMax
volatile int ledMax = 20000;

// peripheral defines
#define LED_PORT 		GPIO_PORT::A
#define LED_PIN  		GPIO_PIN::PIN_15
#define EFFECT1_ADC_PORT 	GPIO_PORT::A
#define EFFECT1_ADC_PIN 	GPIO_PIN::PIN_0
#define EFFECT1_ADC_CHANNEL 	ADC_CHANNEL::CHAN_1
#define EFFECT2_ADC_PORT 	GPIO_PORT::A
#define EFFECT2_ADC_PIN 	GPIO_PIN::PIN_1
#define EFFECT2_ADC_CHANNEL 	ADC_CHANNEL::CHAN_2
#define EFFECT3_ADC_PORT 	GPIO_PORT::A
#define EFFECT3_ADC_PIN 	GPIO_PIN::PIN_2
#define EFFECT3_ADC_CHANNEL 	ADC_CHANNEL::CHAN_3
#define AUDIO_IN_PORT 		GPIO_PORT::A
#define AUDIO_IN_PIN  		GPIO_PIN::PIN_3
#define AUDIO_IN_CHANNEL 	ADC_CHANNEL::CHAN_4
#define EFFECT1_BUTTON_PORT 	GPIO_PORT::B
#define EFFECT1_BUTTON_PIN 	GPIO_PIN::PIN_0
#define EFFECT2_BUTTON_PORT 	GPIO_PORT::B
#define EFFECT2_BUTTON_PIN 	GPIO_PIN::PIN_1
#define SRAM1_CS_PORT 		GPIO_PORT::B
#define SRAM1_CS_PIN 		GPIO_PIN::PIN_12
#define SRAM2_CS_PORT 		GPIO_PORT::B
#define SRAM2_CS_PIN 		GPIO_PIN::PIN_2
#define SRAM3_CS_PORT 		GPIO_PORT::B
#define SRAM3_CS_PIN 		GPIO_PIN::PIN_3
#define SRAM4_CS_PORT 		GPIO_PORT::B
#define SRAM4_CS_PIN 		GPIO_PIN::PIN_4
#define EEPROM1_ADDRESS 	false, false, false
#define EEPROM2_ADDRESS 	true, false, false
#define SDCARD_CS_PORT 		GPIO_PORT::A
#define SDCARD_CS_PIN 		GPIO_PIN::PIN_11
#define OLED_RESET_PORT 	GPIO_PORT::B
#define OLED_RESET_PIN 		GPIO_PIN::PIN_7
#define OLED_DC_PORT 		GPIO_PORT::B
#define OLED_DC_PIN 		GPIO_PIN::PIN_8
#define OLED_CS_PORT 		GPIO_PORT::B
#define OLED_CS_PIN 		GPIO_PIN::PIN_9

// LilKS global variables
AudioBuffer* 		audioBufferPtr;
MidiHandler* 		midiHandlerPtr;
LilKSVoiceManager* 	voiceManagerPtr;
bool 			lilKSSetupComplete = false;

// a simple class to manage the 4 srams available on the Gen_FX_SYN Rev 1 board
class Sram_Manager : public IStorageMedia
{
	public:
		Sram_Manager (const SPI_NUM& spiNum, const GPIO_PORT& sram1CsPort, const GPIO_PIN& sram1CsPin,
				const GPIO_PORT& sram2CsPort, const GPIO_PIN& sram2CsPin,
				const GPIO_PORT& sram3CsPort, const GPIO_PIN& sram3CsPin,
				const GPIO_PORT& sram4CsPort, const GPIO_PIN& sram4CsPin, bool hasMBR = false) :
			m_Sram1( spiNum, sram1CsPort, sram1CsPin ),
			m_Sram2( spiNum, sram2CsPort, sram2CsPin ),
			m_Sram3( spiNum, sram3CsPort, sram3CsPin ),
			m_Sram4( spiNum, sram4CsPort, sram4CsPin ),
			m_Size( Sram_23K256::SRAM_SIZE * 4 )
		{
			// setup gpio for cs pins
			LLPD::gpio_output_setup( sram1CsPort, sram1CsPin, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL,
							GPIO_OUTPUT_SPEED::HIGH, false );
			LLPD::gpio_output_set( sram1CsPort, sram1CsPin, true );

			LLPD::gpio_output_setup( sram2CsPort, sram2CsPin, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL,
							GPIO_OUTPUT_SPEED::HIGH, false );
			LLPD::gpio_output_set( sram2CsPort, sram2CsPin, true );

			LLPD::gpio_output_setup( sram3CsPort, sram3CsPin, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL,
							GPIO_OUTPUT_SPEED::HIGH, false );
			LLPD::gpio_output_set( sram3CsPort, sram3CsPin, true );

			LLPD::gpio_output_setup( sram4CsPort, sram4CsPin, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL,
							GPIO_OUTPUT_SPEED::HIGH, false );
			LLPD::gpio_output_set( sram4CsPort, sram4CsPin, true );
		}

		void writeByte (uint32_t address, uint8_t data)
		{
			if ( address < Sram_23K256::SRAM_SIZE )
			{
				// write to sram 1
				m_Sram1.writeByte( address, data );
			}
			else if ( address < Sram_23K256::SRAM_SIZE * 2 )
			{
				// write to sram 2
				address = ( address - Sram_23K256::SRAM_SIZE );
				m_Sram2.writeByte( address, data );
			}
			else if ( address < Sram_23K256::SRAM_SIZE * 3 )
			{
				// write to sram 3
				address = ( address - (Sram_23K256::SRAM_SIZE * 2) );
				m_Sram3.writeByte( address, data );
			}
			else if ( address < m_Size )
			{
				// write to sram 4
				address = ( address - (Sram_23K256::SRAM_SIZE * 3) );
				m_Sram4.writeByte( address, data );
			}
		}

		uint8_t readByte (uint32_t address)
		{
			if ( address < Sram_23K256::SRAM_SIZE )
			{
				// read from sram 1
				return m_Sram1.readByte( address );
			}
			else if ( address < Sram_23K256::SRAM_SIZE * 2 )
			{
				// read from sram 2
				address = ( address - Sram_23K256::SRAM_SIZE );
				return m_Sram2.readByte( address );
			}
			else if ( address < Sram_23K256::SRAM_SIZE * 3 )
			{
				// read from sram 3
				address = ( address - (Sram_23K256::SRAM_SIZE * 2) );
				return m_Sram3.readByte( address );
			}
			else if ( address < m_Size )
			{
				// read from sram 4
				address = ( address - (Sram_23K256::SRAM_SIZE * 3) );
				return m_Sram4.readByte( address );
			}

			return 0;
		}

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int address) override
		{
			uint8_t* dataPtr = data.getPtr();

			for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
			{
				this->writeByte( address + byte, dataPtr[byte] );
			}
		}

		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override
		{
			SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );
			uint8_t* dataPtr = data.getPtr();

			for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
			{
				dataPtr[byte] = this->readByte( address + byte );
			}

			return data;
		}

		bool needsInitialization() override { return false; }
		void initialize() override {}
		void afterInitialize() override {}

		bool hasMBR() override { return false; }

	private:
		Sram_23K256 	m_Sram1;
		Sram_23K256 	m_Sram2;
		Sram_23K256 	m_Sram3;
		Sram_23K256 	m_Sram4;
		uint32_t 	m_Size;
};

// a simple class to manage the 2 eeproms available on the Gen_FX_SYN Rev 1 board
class Eeprom_Manager : public IStorageMedia
{
	public:
		Eeprom_Manager (const I2C_NUM& i2cNum, bool Eeprom1A0IsHigh, bool Eeprom1A1IsHigh, bool Eeprom1A2IsHigh,
				bool Eeprom2A0IsHigh, bool Eeprom2A1IsHigh, bool Eeprom2A2IsHigh) :
			m_Eeprom1( i2cNum, Eeprom1A0IsHigh, Eeprom1A1IsHigh, Eeprom1A2IsHigh ),
			m_Eeprom2( i2cNum, Eeprom2A0IsHigh, Eeprom2A1IsHigh, Eeprom2A2IsHigh )
		{
		}

		void writeByte (uint16_t address, uint8_t data)
		{
			if ( address < Eeprom_CAT24C64::EEPROM_SIZE )
			{
				m_Eeprom1.writeByte( address, data );
			}
			else if ( address < m_Size )
			{
				address = ( address - Eeprom_CAT24C64::EEPROM_SIZE );
				m_Eeprom2.writeByte( address, data );
			}
		}

		uint8_t readByte (uint16_t address)
		{
			if ( address < Eeprom_CAT24C64::EEPROM_SIZE )
			{
				return m_Eeprom1.readByte( address );
			}
			else if ( address < m_Size )
			{
				address = ( address - Eeprom_CAT24C64::EEPROM_SIZE );
				return m_Eeprom2.readByte( address );
			}

			return 0;
		}

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int address) override
		{
			uint8_t* dataPtr = data.getPtr();

			for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
			{
				this->writeByte( address + byte, dataPtr[byte] );
			}
		}

		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override
		{
			SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );
			uint8_t* dataPtr = data.getPtr();

			for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
			{
				dataPtr[byte] = this->readByte( address + byte );
			}

			return data;
		}

		bool needsInitialization() override { return false; }
		void initialize() override {}
		void afterInitialize() override {}

		bool hasMBR() override { return false; }

	private:
		Eeprom_CAT24C64 	m_Eeprom1;
		Eeprom_CAT24C64 	m_Eeprom2;
		uint32_t 		m_Size;
};

// these pins are unconnected on Gen_FX_SYN Rev 1 development board, so we disable them as per the ST recommendations
void disableUnusedPins()
{
	LLPD::gpio_output_setup( GPIO_PORT::C, GPIO_PIN::PIN_13, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
					GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_setup( GPIO_PORT::C, GPIO_PIN::PIN_14, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
					GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_setup( GPIO_PORT::C, GPIO_PIN::PIN_15, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
					GPIO_OUTPUT_SPEED::LOW );

	LLPD::gpio_output_setup( GPIO_PORT::A, GPIO_PIN::PIN_8, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
					GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_setup( GPIO_PORT::A, GPIO_PIN::PIN_12, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
					GPIO_OUTPUT_SPEED::LOW );
	// TODO currently we're using this as an LED pin, remove it when done debugging
	// LLPD::gpio_output_setup( GPIO_PORT::A, GPIO_PIN::PIN_15, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
	// 				GPIO_OUTPUT_SPEED::LOW );

	LLPD::gpio_output_setup( GPIO_PORT::B, GPIO_PIN::PIN_2, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
					GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_setup( GPIO_PORT::B, GPIO_PIN::PIN_3, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
					GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_setup( GPIO_PORT::B, GPIO_PIN::PIN_4, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
					GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_setup( GPIO_PORT::B, GPIO_PIN::PIN_5, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
					GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_setup( GPIO_PORT::B, GPIO_PIN::PIN_6, GPIO_PUPD::PULL_DOWN, GPIO_OUTPUT_TYPE::PUSH_PULL,
					GPIO_OUTPUT_SPEED::LOW );
}

int main(void)
{
	// clock setup
	LLPD::rcc_clock_setup( RCC_CLOCK_SOURCE::EXTERNAL, false );

	// enable all gpio clocks
	LLPD::gpio_enable_clock( GPIO_PORT::A );
	LLPD::gpio_enable_clock( GPIO_PORT::B );
	LLPD::gpio_enable_clock( GPIO_PORT::C );

	// USART setup
	LLPD::usart_init( USART_NUM::USART_3, USART_WORD_LENGTH::BITS_8, USART_PARITY::EVEN, USART_CONF::TX_AND_RX,
				USART_STOP_BITS::BITS_1, SYS_CLOCK_FREQUENCY, 9600 );
	LLPD::usart_log( USART_NUM::USART_3, "Gen_FX_SYN starting up -----------------------------" );

	// disable the unused pins
	disableUnusedPins();

	// i2c setup (8MHz source 100KHz clock 0x00201D2B, 32MHz source 100KHz clock 0x00B07CB4, 72MHz source 100KHz 0x10C08DCF)
	LLPD::i2c_master_setup( I2C_NUM::I2C_2, 0x00B07CB4 );
	LLPD::usart_log( USART_NUM::USART_3, "I2C initialized..." );

	// spi init
	LLPD::spi_master_init( SPI_NUM::SPI_2, SPI_BAUD_RATE::SYSCLK_DIV_BY_256, SPI_CLK_POL::LOW_IDLE, SPI_CLK_PHASE::FIRST,
				SPI_DUPLEX::FULL, SPI_FRAME_FORMAT::MSB_FIRST, SPI_DATA_SIZE::BITS_8 );
	LLPD::usart_log( USART_NUM::USART_3, "spi initialized..." );

	// LED pin
	LLPD::gpio_output_setup( LED_PORT, LED_PIN, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL, GPIO_OUTPUT_SPEED::HIGH );
	LLPD::gpio_output_set( LED_PORT, LED_PIN, ledState );

	// audio timer setup (for 40 kHz sampling rate at 32 MHz system clock)
	LLPD::tim6_counter_setup( 1, 800, 40000 );
	LLPD::tim6_counter_enable_interrupts();
	LLPD::usart_log( USART_NUM::USART_3, "tim6 initialized..." );

	// DAC setup
	LLPD::dac_init( true );
	LLPD::usart_log( USART_NUM::USART_3, "dac initialized..." );

	// Op Amp setup
	LLPD::gpio_analog_setup( GPIO_PORT::A, GPIO_PIN::PIN_5 );
	LLPD::gpio_analog_setup( GPIO_PORT::A, GPIO_PIN::PIN_6 );
	LLPD::gpio_analog_setup( GPIO_PORT::A, GPIO_PIN::PIN_7 );
	LLPD::opamp_init();
	LLPD::usart_log( USART_NUM::USART_3, "op amp initialized..." );

	// audio timer start
	LLPD::tim6_counter_start();
	LLPD::usart_log( USART_NUM::USART_3, "tim6 started..." );

	// ADC setup (note, this must be done after the tim6_counter_start() call since it uses the delay function)
	LLPD::rcc_pll_enable( RCC_CLOCK_SOURCE::INTERNAL, RCC_PLL_MULTIPLY::NONE );
	LLPD::gpio_analog_setup( EFFECT1_ADC_PORT, EFFECT1_ADC_PIN );
	LLPD::gpio_analog_setup( EFFECT2_ADC_PORT, EFFECT2_ADC_PIN );
	LLPD::gpio_analog_setup( EFFECT3_ADC_PORT, EFFECT3_ADC_PIN );
	LLPD::gpio_analog_setup( AUDIO_IN_PORT, AUDIO_IN_PIN );
	LLPD::adc_init( ADC_CYCLES_PER_SAMPLE::CPS_2p5 );
	LLPD::adc_set_channel_order( 4, EFFECT1_ADC_CHANNEL, EFFECT2_ADC_CHANNEL, EFFECT3_ADC_CHANNEL, AUDIO_IN_CHANNEL );
	adcSetupComplete = true;
	LLPD::usart_log( USART_NUM::USART_3, "adc initialized..." );

	// pushbutton setup
	LLPD::gpio_digital_input_setup( EFFECT1_BUTTON_PORT, EFFECT1_BUTTON_PIN, GPIO_PUPD::PULL_UP );
	LLPD::gpio_digital_input_setup( EFFECT2_BUTTON_PORT, EFFECT2_BUTTON_PIN, GPIO_PUPD::PULL_UP );

	// EEPROM setup and test
	Eeprom_Manager eeproms( I2C_NUM::I2C_2, EEPROM1_ADDRESS, EEPROM2_ADDRESS );
	// TODO comment the verification lines out if you're using the eeprom for persistent memory
	SharedData<uint8_t> eepromValsToWrite = SharedData<uint8_t>::MakeSharedData( 3 );
	eepromValsToWrite[0] = 64; eepromValsToWrite[1] = 23; eepromValsToWrite[2] = 17;
	eeproms.writeToMedia( eepromValsToWrite, 45 );
	eeproms.writeToMedia( eepromValsToWrite, 45 + Eeprom_CAT24C64::EEPROM_SIZE );
	SharedData<uint8_t> eeprom1Verification = eeproms.readFromMedia( 3, 45 );
	SharedData<uint8_t> eeprom2Verification = eeproms.readFromMedia( 3, 45 + Eeprom_CAT24C64::EEPROM_SIZE );
	if ( eeprom1Verification[0] == 64 && eeprom1Verification[1] == 23 && eeprom1Verification[2] == 17 &&
			eeprom2Verification[0] == 64 && eeprom2Verification[1] == 23 && eeprom2Verification[2] == 17 )
	{
		LLPD::usart_log( USART_NUM::USART_3, "eeproms verified..." );
	}
	else
	{
		LLPD::usart_log( USART_NUM::USART_3, "WARNING!!! eeproms failed verification..." );
	}

	// SRAM setup and test
	Sram_Manager srams( SPI_NUM::SPI_2, SRAM1_CS_PORT, SRAM1_CS_PIN, SRAM2_CS_PORT, SRAM2_CS_PIN, SRAM3_CS_PORT, SRAM3_CS_PIN,
				SRAM4_CS_PORT, SRAM4_CS_PIN );
	SharedData<uint8_t> sramValsToWrite = SharedData<uint8_t>::MakeSharedData( 3 );
	sramValsToWrite[0] = 25; sramValsToWrite[1] = 16; sramValsToWrite[2] = 8;
	srams.writeToMedia( sramValsToWrite, 45 );
	srams.writeToMedia( sramValsToWrite, 45 + Sram_23K256::SRAM_SIZE );
	srams.writeToMedia( sramValsToWrite, 45 + Sram_23K256::SRAM_SIZE * 2 );
	srams.writeToMedia( sramValsToWrite, 45 + Sram_23K256::SRAM_SIZE * 3 );
	SharedData<uint8_t> sram1Verification = srams.readFromMedia( 3, 45 );
	SharedData<uint8_t> sram2Verification = srams.readFromMedia( 3, 45 + Sram_23K256::SRAM_SIZE );
	SharedData<uint8_t> sram3Verification = srams.readFromMedia( 3, 45 + Sram_23K256::SRAM_SIZE * 2 );
	SharedData<uint8_t> sram4Verification = srams.readFromMedia( 3, 45 + Sram_23K256::SRAM_SIZE * 3 );
	if ( sram1Verification[0] == 25 && sram1Verification[1] == 16 && sram1Verification[2] == 8 &&
			sram2Verification[0] == 25 && sram2Verification[1] == 16 && sram2Verification[2] == 8 &&
			sram3Verification[0] == 25 && sram3Verification[1] == 16 && sram3Verification[2] == 8 &&
			sram4Verification[0] == 25 && sram4Verification[1] == 16 && sram4Verification[2] == 8 )
	{
		LLPD::usart_log( USART_NUM::USART_3, "srams verified..." );
	}
	else
	{
		LLPD::usart_log( USART_NUM::USART_3, "WARNING!!! srams failed verification..." );
	}

	// lilks setup
	AudioBuffer audioBuffer;
	MidiHandler midiHandler;
	LilKSVoiceManager voiceManager( &srams );

	voiceManager.bindToKeyEventSystem();

	audioBuffer.registerCallback( &voiceManager );

	audioBufferPtr  = &audioBuffer;
	midiHandlerPtr  = &midiHandler;
	voiceManagerPtr = &voiceManager;

	lilKSSetupComplete = true;

	LLPD::usart_log( USART_NUM::USART_3, "Gen_FX_SYN setup complete, entering while loop -------------------------------" );

	while ( true )
	{
		audioBuffer.pollToFillBuffers();

		/*
		if ( ! LLPD::gpio_input_get(EFFECT1_BUTTON_PORT, EFFECT1_BUTTON_PIN) )
		{
			LLPD::usart_log( USART_NUM::USART_3, "BUTTON 1 PRESSED" );
		}

		if ( ! LLPD::gpio_input_get(EFFECT2_BUTTON_PORT, EFFECT2_BUTTON_PIN) )
		{
			LLPD::usart_log( USART_NUM::USART_3, "BUTTON 2 PRESSED" );
		}

		LLPD::usart_log_int( USART_NUM::USART_3, "POT 1 VALUE: ", LLPD::adc_get_channel_value(EFFECT1_ADC_CHANNEL) );
		LLPD::usart_log_int( USART_NUM::USART_3, "POT 2 VALUE: ", LLPD::adc_get_channel_value(EFFECT2_ADC_CHANNEL) );
		LLPD::usart_log_int( USART_NUM::USART_3, "POT 3 VALUE: ", LLPD::adc_get_channel_value(EFFECT3_ADC_CHANNEL) );
		*/
	}
}

extern "C" void TIM6_DAC_IRQHandler (void)
{
	if ( ! LLPD::tim6_isr_handle_delay() ) // if not currently in a delay function,...
	{
		if ( lilKSSetupComplete )
		{
			LLPD::adc_perform_conversion_sequence();

			uint16_t outputVal = audioBufferPtr->getNextSample() * 4095;

			LLPD::dac_send( outputVal );
		}

		if ( keepBlinking && ledIncr > ledMax )
		{
			if ( ledState )
			{
				LLPD::gpio_output_set( LED_PORT, LED_PIN, false );
				ledState = false;
			}
			else
			{
				LLPD::gpio_output_set( LED_PORT, LED_PIN, true );
				ledState = true;
			}

			ledIncr = 0;
		}
		else
		{
			ledIncr++;
		}
	}

	LLPD::tim6_counter_clear_interrupt_flag();
}

extern "C" void USART3_IRQHandler (void)
{
	// loopback test code for usart recieve
	uint16_t data = LLPD::usart_receive( USART_NUM::USART_3 );

	midiHandlerPtr->processByte( data );
	midiHandlerPtr->dispatchEvents();
}
