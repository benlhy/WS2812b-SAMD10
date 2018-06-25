// References
// https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
// SAM D21 SERCOM SPI Configuration
// http://ww1.microchip.com/downloads/en/AppNotes/00002465A.pdf
// https://github.com/jeelabs/embello/blob/master/explore/1450-dips/leds/main.cpp
// https://github.com/rogerclarkmelbourne/WS2812B_STM32_Libmaple
// https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/


#include <asf.h>
#include <math.h>
	
	
#define NUM_LEDS 34


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

uint8_t pixelArray[NUM_LEDS][3]={};//empty array




	
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
	spiSend(0); // we need to send zero bytes for the system to reset itself!
	spiSend(0);
	for (int i = NUM_LEDS - count; i <= NUM_LEDS; ++i) {
		int phase = 4 * i;
		if (phase < 0)
		phase = 0;
		sendRGB(r * phase, g * phase, b * phase);
	}
}

static void cometRacer(int r, int g, int b) {
	for (int i = 0; i < NUM_LEDS*2; ++i) {
		cometTail(i, r, g, b);
		delay_ms(50); 
	}
}

// wrapper for LED animations
static void showStrip(void){
	spiSend(0);
	spiSend(0);
	for(int i=0;i<NUM_LEDS;i++){
		sendRGB(pixelArray[i][0],pixelArray[i][1],pixelArray[i][2]);
	}
}

static void setPixel(int count, int r, int g, int b){
	pixelArray[count][0] = r;
	pixelArray[count][1] = g;
	pixelArray[count][2] = b;
}

static uint32_t getPixel(int count){
	uint32_t val = pixelArray[count][0]<<16|pixelArray[count][1]<<8|pixelArray[count][2];
	return val;
}

static void setAll(int r, int g, int b){
	for (int i = 0;i<NUM_LEDS;i++){
		setPixel(i,r,g,b);
	}
}

static int random_range(int min, int max){
	return rand()%(max-min)+min;
}

static void delay(int delay_time){
	delay_ms(delay_time);
}

void RGBLoop(){
  for(int j = 0; j < 3; j++ ) { 
    // Fade IN
    for(int k = 0; k < 256; k++) { 
      switch(j) { 
        case 0: setAll(k,0,0); break;
        case 1: setAll(0,k,0); break;
        case 2: setAll(0,0,k); break;
      }
      showStrip();
      delay(3);
    }
    // Fade OUT
    for(int k = 255; k >= 0; k--) { 
      switch(j) { 
        case 0: setAll(k,0,0); break;
        case 1: setAll(0,k,0); break;
        case 2: setAll(0,0,k); break;
      }
      showStrip();
      delay(3);
    }
  }
}

void FadeInOut(uint8_t red, uint8_t green, uint8_t blue){
  float r, g, b;
      
  for(int k = 0; k < 256; k=k+1) { 
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAll(r,g,b);
    showStrip();
  }
     
  for(int k = 255; k >= 0; k=k-2) {
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAll(r,g,b);
    showStrip();
  }
}

void setPixelHeatColor (int Pixel, uint8_t temperature) {
	// Scale 'heat' down from 0-255 to 0-191
	uint8_t t192 = ((float)temperature/255.0)*191;
	
	// calculate ramp up from
	uint8_t heatramp = t192 & 0x3F; // 0..63
	heatramp <<= 2; // scale up to 0..252
	
	// figure out which third of the spectrum we're in:
	if( t192 > 0x80) {                     // hottest
		setPixel(Pixel, 255, 255, heatramp);
		} else if( t192 > 0x40 ) {             // middle
		setPixel(Pixel, 255, heatramp, 0);
		} else {                               // coolest
		setPixel(Pixel, heatramp, 0, 0);
	}
}

void Fire(int Cooling, int Sparking, int SpeedDelay) {
	static uint8_t heat[NUM_LEDS];
	int cooldown;
	
	// Step 1.  Cool down every cell a little
	for( int i = 0; i < NUM_LEDS; i++) {
		cooldown = random_range(0, ((Cooling * 10) / NUM_LEDS) + 2);
		
		if(cooldown>heat[i]) {
			heat[i]=0;
			} else {
			heat[i]=heat[i]-cooldown;
		}
	}
	
	// Step 2.  Heat from each cell drifts 'up' and diffuses a little
	for( int k= NUM_LEDS - 1; k >= 2; k--) {
		heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
	}
	
	// Step 3.  Randomly ignite new 'sparks' near the bottom
	if( random_range(0,255) < Sparking ) {
		int y = random_range(0,7);
		heat[y] = heat[y] + random_range(160,255);
		//heat[y] = random(160,255);
	}

	// Step 4.  Convert heat to LED colors
// 	spiSend(0); // we need to send zero bytes for the system to reset itself!
// 	spiSend(0);
	for( int j = 0; j < NUM_LEDS; j++) {
		setPixelHeatColor(j, heat[j] );
	}

	showStrip();
	delay(SpeedDelay);
}

void fadeToBlack(int ledNo, uint8_t fadeValue) {

	uint32_t oldColor;
	uint8_t r, g, b;
	int value;
	
	oldColor = getPixel(ledNo);
	r = (oldColor & 0x00ff0000UL) >> 16;
	g = (oldColor & 0x0000ff00UL) >> 8;
	b = (oldColor & 0x000000ffUL);

	r=(r<=10)? 0 : (int) r-(r*fadeValue/256);
	g=(g<=10)? 0 : (int) g-(g*fadeValue/256);
	b=(b<=10)? 0 : (int) b-(b*fadeValue/256);
	
	setPixel(ledNo, r,g,b);


}

void meteorRain(uint8_t red, uint8_t green, uint8_t blue, uint8_t meteorSize, uint8_t meteorTrailDecay, bool meteorRandomDecay, int SpeedDelay) {
	setAll(0,0,0);
	
	for(int i = 0; i < NUM_LEDS+NUM_LEDS; i++) {
		
		
		// fade brightness all LEDs one step
		for(int j=0; j<NUM_LEDS; j++) {
			if( (!meteorRandomDecay) || (random_range(0,10)>5) ) {
				fadeToBlack(j, meteorTrailDecay );
			}
		}
		
		// draw meteor
		for(int j = 0; j < meteorSize; j++) {
			if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
				setPixel(i-j, red, green, blue);
			}
		}
		
		showStrip();
		delay(SpeedDelay);
	}
}


// PA22
int main (void)
{
	system_init();
	delay_init();

	
	configure_spi_master();
	setAll(0,0,0);
	showStrip();
	
	while (1) {
		meteorRain(0xf0,0x00,0xf0,1, 200, true, 40);
		delay(50);
// 		setPixel(6,128,0,0);
// 		showStrip();
// 		delay(5);
		//FadeInOut(0xff, 0x77, 0x00);
		//RGBLoop();
// 		        cometRacer(1, 0, 0);    // red
// 		        cometRacer(0, 1, 0);    // green
// 		        cometRacer(0, 0, 1);    // blue

	}
}
