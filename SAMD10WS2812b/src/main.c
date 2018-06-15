// References
// https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
// SAM D21 SERCOM SPI Configuration
// http://ww1.microchip.com/downloads/en/AppNotes/00002465A.pdf
// https://github.com/jeelabs/embello/blob/master/explore/1450-dips/leds/main.cpp
// https://github.com/rogerclarkmelbourne/WS2812B_STM32_Libmaple
// https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/


#include <asf.h>
	
	
#define NUM_LEDS 60


// SPI uses SERCOM1
// Configuration C
// 0 MOSI : PA22
// 1 SCK  : PA23
// 2 MISO : PA24
// 3 SS   : PA17

#define SLAVE_SELECT_PIN PIN_PA17
#define CONF_MASTER_MUX_SETTING SPI_SIGNAL_MUX_SETTING_C
#define CONF_MASTER_PINMUX_PAD0 PINMUX_PA22C_SERCOM1_PAD0
#define CONF_MASTER_PINMUX_PAD1 PINMUX_PA23C_SERCOM1_PAD1
#define CONF_MASTER_PINMUX_PAD2 PINMUX_PA24C_SERCOM1_PAD2
#define CONF_MASTER_PINMUX_PAD3 PINMUX_UNUSED
#define CONF_MASTER_SPI_MODULE SERCOM1

struct spi_module spi_master_instance;
struct spi_slave_inst slave;



static const uint16_t bits[] = {
	0b100100100100, // 0000
	0b100100100110, // 0001
	0b100100110100, // 0010
	0b100100110110, // 0011
	0b100110100100, // 0100
	0b100110100110, // 0101
	0b100110110100, // 0110
	0b100110110110, // 0111
	0b110100100100, // 1000
	0b110100100110, // 1001
	0b110100110100, // 1010
	0b110100110110, // 1011
	0b110110100100, // 1100
	0b110110100110, // 1101
	0b110110110100, // 1110
	0b110110110110, // 1111
};





	
// baud_rate = ((clock input freq/clock_divider+1)/2)) 
// fref =  48MHz
// fbaud = fref/2(BAUD+1)

void configure_spi_master(void)
{
	struct spi_config config_spi_master;
	struct spi_slave_inst_config slave_dev_config;
	/* Configure and initialize software device instance of peripheral slave */
	spi_slave_inst_get_config_defaults(&slave_dev_config);
	slave_dev_config.ss_pin = SLAVE_SELECT_PIN;
	spi_attach_slave(&slave, &slave_dev_config);
	/* Configure, initialize and enable SERCOM SPI module */
	spi_get_config_defaults(&config_spi_master);
	config_spi_master.transfer_mode = SPI_TRANSFER_MODE_1;
	config_spi_master.mux_setting = CONF_MASTER_MUX_SETTING;
	config_spi_master.pinmux_pad0 = CONF_MASTER_PINMUX_PAD0;
	config_spi_master.pinmux_pad1 = CONF_MASTER_PINMUX_PAD1;
	config_spi_master.pinmux_pad2 = CONF_MASTER_PINMUX_PAD2;
	config_spi_master.pinmux_pad3 = CONF_MASTER_PINMUX_PAD3;
	config_spi_master.mode_specific.master.baudrate =  2500000; //2.5MHz  = 400ns per signal
	spi_init(&spi_master_instance, CONF_MASTER_SPI_MODULE, &config_spi_master);
	spi_enable(&spi_master_instance);
}

static void spiSend(uint16_t cmd){
	while(!spi_is_ready_to_write(&spi_master_instance)){;}
	spi_write(&spi_master_instance,cmd>>8);
	while(!spi_is_ready_to_write(&spi_master_instance)){;}
	spi_write(&spi_master_instance,cmd);
}

static void sendByte (int value) {
	spiSend(bits[value >> 4]);
	spiSend(bits[value & 0xF]);
}

static void sendRGB (int r, int g, int b) {
	sendByte(g);
	sendByte(r);
	sendByte(b);
}

static void cometTail (int count, int r, int g, int b) {
	spiSend(0); // we need to send zero bytes for the sytem to reset itself!
	spiSend(0);
	for (int i = NUM_LEDS - count; i <= NUM_LEDS; ++i) {
		int phase = 4 * i;
		if (phase < 0)
		phase = 0;
		sendRGB(r * phase, g * phase, b * phase);
	}
}

static void cometRacer(int r, int g, int b) {
	for (int i = 0; i < NUM_LEDS*3; ++i) {
		cometTail(i, r, g, b);
		delay_ms(5);
	}
}

int main (void)
{
	system_init();
	delay_init();

	
	configure_spi_master();
	
	
	while (1) {
		        cometRacer(1, 0, 0);    // red
		        cometRacer(0, 1, 0);    // green
		        cometRacer(0, 0, 1);    // blue
		



	}
}
