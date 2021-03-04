#include "../lib/STM32f302x8-HAL/llpd/include/LLPD.hpp"
#include "../lib/STM32f302x8-HAL/stm32cubef3/include/stm32f302xc.h"

#include <math.h>

#include "EEPROM_CAT24C64.hpp"
#include "SRAM_23K256.hpp"

#include "AudioBuffer.hpp"
#include "MidiHandler.hpp"
#include "LilKSVoiceManager.hpp"

// to disassemble -- arm-none-eabi-objdump -S --disassemble main_debug.elf > disassembled.s

#define SYS_CLOCK_FREQUENCY 64000000

// global variables
volatile bool ledState = false; // led state: true for on, false for off
volatile bool keepBlinking = false; // a test variable that determines whether or not to flash the led
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
	// set system clock to PLL with HSE (32MHz / 2) as input, so 64MHz system clock speed
	LLPD::rcc_clock_setup( RCC_CLOCK_SOURCE::EXTERNAL, true, RCC_PLL_MULTIPLY::BY_4, SYS_CLOCK_FREQUENCY );

	// prescale APB1 by 2, since the maximum clock speed is 36MHz
	LLPD::rcc_set_periph_clock_prescalers( RCC_AHB_PRES::BY_1, RCC_APB1_PRES::AHB_BY_2, RCC_APB2_PRES::AHB_BY_1 );

	// enable all gpio clocks
	LLPD::gpio_enable_clock( GPIO_PORT::A );
	LLPD::gpio_enable_clock( GPIO_PORT::B );
	LLPD::gpio_enable_clock( GPIO_PORT::C );
	LLPD::gpio_enable_clock( GPIO_PORT::F );

	// USART setup
	// TODO remove logging entirely once fully tested
	// LLPD::usart_init( USART_NUM::USART_3, USART_WORD_LENGTH::BITS_8, USART_PARITY::EVEN, USART_CONF::TX_AND_RX,
	// 			USART_STOP_BITS::BITS_1, SYS_CLOCK_FREQUENCY, 9600 );
	// LLPD::usart_log( USART_NUM::USART_3, "Gen_FX_SYN starting up -----------------------------" );

	// disable the unused pins
	disableUnusedPins();

	// i2c init
	LLPD::i2c_master_setup( I2C_NUM::I2C_2, 0x10B07EBA );
	// LLPD::usart_log( USART_NUM::USART_3, "I2C initialized..." );

	// spi init
	LLPD::spi_master_init( SPI_NUM::SPI_2, SPI_BAUD_RATE::SYSCLK_DIV_BY_2, SPI_CLK_POL::LOW_IDLE, SPI_CLK_PHASE::FIRST,
				SPI_DUPLEX::FULL, SPI_FRAME_FORMAT::MSB_FIRST, SPI_DATA_SIZE::BITS_8 );
	// LLPD::usart_log( USART_NUM::USART_3, "spi initialized..." );

	// LED pin
	LLPD::gpio_output_setup( LED_PORT, LED_PIN, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL, GPIO_OUTPUT_SPEED::HIGH );
	LLPD::gpio_output_set( LED_PORT, LED_PIN, ledState );

	// audio timer setup (for 40 kHz sampling rate at 64 MHz system clock)
	LLPD::tim6_counter_setup( 0, 1600, 40000 );
	LLPD::tim6_counter_enable_interrupts();
	// LLPD::usart_log( USART_NUM::USART_3, "tim6 initialized..." );

	// DAC setup
	LLPD::dac_init( true );
	// LLPD::usart_log( USART_NUM::USART_3, "dac initialized..." );

	// Op Amp setup
	LLPD::gpio_analog_setup( GPIO_PORT::A, GPIO_PIN::PIN_5 );
	LLPD::gpio_analog_setup( GPIO_PORT::A, GPIO_PIN::PIN_6 );
	LLPD::gpio_analog_setup( GPIO_PORT::A, GPIO_PIN::PIN_7 );
	LLPD::opamp_init();
	// LLPD::usart_log( USART_NUM::USART_3, "op amp initialized..." );

	// audio timer start
	LLPD::tim6_counter_start();
	// LLPD::usart_log( USART_NUM::USART_3, "tim6 started..." );

	// ADC setup (note, this must be done after the tim6_counter_start() call since it uses the delay function)
	LLPD::gpio_analog_setup( EFFECT1_ADC_PORT, EFFECT1_ADC_PIN );
	LLPD::gpio_analog_setup( EFFECT2_ADC_PORT, EFFECT2_ADC_PIN );
	LLPD::gpio_analog_setup( EFFECT3_ADC_PORT, EFFECT3_ADC_PIN );
	LLPD::gpio_analog_setup( AUDIO_IN_PORT, AUDIO_IN_PIN );
	LLPD::adc_init( ADC_CYCLES_PER_SAMPLE::CPS_2p5 );
	LLPD::adc_set_channel_order( 4, EFFECT1_ADC_CHANNEL, EFFECT2_ADC_CHANNEL, EFFECT3_ADC_CHANNEL, AUDIO_IN_CHANNEL );
	adcSetupComplete = true;
	// LLPD::usart_log( USART_NUM::USART_3, "adc initialized..." );

	// pushbutton setup
	LLPD::gpio_digital_input_setup( EFFECT1_BUTTON_PORT, EFFECT1_BUTTON_PIN, GPIO_PUPD::PULL_UP );
	LLPD::gpio_digital_input_setup( EFFECT2_BUTTON_PORT, EFFECT2_BUTTON_PIN, GPIO_PUPD::PULL_UP );

	// SRAM setup and test
	LLPD::gpio_output_setup( SRAM1_CS_PORT, SRAM1_CS_PIN, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL,
							GPIO_OUTPUT_SPEED::HIGH, false );
	LLPD::gpio_output_set( SRAM1_CS_PORT, SRAM1_CS_PIN, true );
	Sram_23K256 sram( SPI_NUM::SPI_2, SRAM1_CS_PORT, SRAM1_CS_PIN );
	SharedData<uint8_t> sramValsToWrite = SharedData<uint8_t>::MakeSharedData( 3 );
	sramValsToWrite[0] = 25; sramValsToWrite[1] = 16; sramValsToWrite[2] = 8;
	sram.writeToMedia( sramValsToWrite, 45 );
	SharedData<uint8_t> sram1Verification = sram.readFromMedia( 3, 45 );
	if ( sram1Verification[0] == 25 && sram1Verification[1] == 16 && sram1Verification[2] == 8 )
	{
		// LLPD::usart_log( USART_NUM::USART_3, "sram verified..." );
	}
	else
	{
		// LLPD::usart_log( USART_NUM::USART_3, "WARNING!!! sram failed verification..." );
	}
	bool usingSeqMode = sram.setSequentialMode( true );
	while ( ! usingSeqMode ) { usingSeqMode = sram.setSequentialMode( true ); }

	// lilks setup
	AudioBuffer audioBuffer;
	MidiHandler midiHandler;
	LilKSVoiceManager voiceManager( &sram );

	voiceManager.bindToKeyEventSystem();

	audioBuffer.registerCallback( &voiceManager );

	audioBufferPtr  = &audioBuffer;
	midiHandlerPtr  = &midiHandler;
	voiceManagerPtr = &voiceManager;

	// enable usart for MIDI
	LLPD::usart_init( USART_NUM::USART_3, USART_WORD_LENGTH::BITS_8, USART_PARITY::EVEN, USART_CONF::TX_AND_RX,
				USART_STOP_BITS::BITS_1, SYS_CLOCK_FREQUENCY, 31250 );

	lilKSSetupComplete = true;
	keepBlinking = true;

	// LLPD::usart_log( USART_NUM::USART_3, "Gen_FX_SYN setup complete, entering while loop -------------------------------" );

	while ( true )
	{
		midiHandler.dispatchEvents();
		audioBuffer.pollToFillBuffers();

		if ( ! LLPD::gpio_input_get(EFFECT1_BUTTON_PORT, EFFECT1_BUTTON_PIN) )
		{
			// LLPD::usart_log( USART_NUM::USART_3, "BUTTON 1 PRESSED" );
		}

		if ( ! LLPD::gpio_input_get(EFFECT2_BUTTON_PORT, EFFECT2_BUTTON_PIN) )
		{
			// LLPD::usart_log( USART_NUM::USART_3, "BUTTON 2 PRESSED" );
		}

		// LLPD::usart_log_int( USART_NUM::USART_3, "POT 1 VALUE: ", LLPD::adc_get_channel_value(EFFECT1_ADC_CHANNEL) );
		// LLPD::usart_log_int( USART_NUM::USART_3, "POT 2 VALUE: ", LLPD::adc_get_channel_value(EFFECT2_ADC_CHANNEL) );
		// LLPD::usart_log_int( USART_NUM::USART_3, "POT 3 VALUE: ", LLPD::adc_get_channel_value(EFFECT3_ADC_CHANNEL) );
	}
}

extern "C" void TIM6_DAC_IRQHandler (void)
{
	if ( ! LLPD::tim6_isr_handle_delay() ) // if not currently in a delay function,...
	{
		if ( lilKSSetupComplete )
		{
			unsigned int kpVal = static_cast<unsigned int>( audioBufferPtr->getNextSample() * 4095.0f );
			LLPD::dac_send( kpVal );
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
	uint16_t data = LLPD::usart_receive( USART_NUM::USART_3 );

	if ( data != MIDI_TIMING_CLOCK && data != MIDI_ACTIVE_SENSING ) // not using any timing clock or active sensing stuff, slow
	{
		midiHandlerPtr->processByte( data );
	}
}
