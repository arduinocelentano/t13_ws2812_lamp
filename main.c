#include <avr/io.h>
#include <util/delay.h>
#include "light_ws2812.h"
#include <avr/eeprom.h> 
#define	PIXEL_NUM	8
#define	DELAY		70
#define RAINBOW_SIZE 3
#define BUTTON 4
#define STATE_EEPROM_ADDRESS (uint8_t*)0x10 //Current mode is stored here

struct cRGB pixels[PIXEL_NUM];

const struct cRGB rainbow [RAINBOW_SIZE] ={{0, 200, 0},{200, 0, 0},{0, 0, 200}};//Three primary rainbow colors
static struct cRGB current = rainbow[0]; //Current color (a shade of primary rainbow) in rainbow effects

static void setpixel(uint8_t id, struct cRGB *p);//Lightin a certain pixel
static void updateCurrentColor(); //Recalculate current color
static uint8_t random(); //Very simple PRNG

static struct cRGB blank = {0, 0, 0};//blank color
static struct cRGB p = {0, 0, 0};//a variable for temporary colors
static uint8_t heat[PIXEL_NUM];//heat value for fire simulation

int main(void)
{
	DDRB &= ~(1<<BUTTON); // Init button pin as Input
	PORTB &= ~(1 << BUTTON);
	int8_t animationMax = 32;
	uint8_t cooldown;//a cooldown speed for fire animation
	uint8_t i;
	uint8_t pressed = 0;
	uint8_t animationFrame = 0;//the number of current animation frame - needed for animation calculations
	uint8_t shift = 0;//a shift for sparkle animation
	uint8_t mode = eeprom_read_byte(STATE_EEPROM_ADDRESS);
	if (mode>3)
		mode =3;
    
	while (1)
	{
		if ((PINB & (1<<BUTTON))) {//processing the touch button
			if ((PINB & (1<<BUTTON)))
				pressed = 1;
		}
		else
			if (pressed){
				pressed = 0;
				mode++;
				if(mode>3)
					mode = 0;
				animationFrame = 0;
				eeprom_write_byte(STATE_EEPROM_ADDRESS,mode);
				}
		
		_delay_ms(DELAY);
		if (mode == 0){//Rainbow spark mode
			animationFrame++;
			if (animationFrame == animationMax){
				animationFrame = 0;
				updateCurrentColor();
			}
			for (i = 0; i < PIXEL_NUM ; ++i) {
				if(i<animationFrame){
					shift = (animationFrame-i);
					p.r = current.r >> shift;
					p.g = current.g >> shift;
					p.b = current.b >> shift;
					setpixel(i, &p);
				}
				else{
					setpixel(i, &blank);
				}
			}
		}
		else if (mode == 1){//Rainbow glow mode
			for (i = 0; i < PIXEL_NUM; ++i) {
					setpixel(i, &current);
				}
			updateCurrentColor();
		}
		else if (mode==2){//Fire mode
			random();
			for (i = 0; i < PIXEL_NUM ; ++i) {
				cooldown = random()%64;
				if(cooldown>heat[i]) {
					heat[i]=0;
				} else {
					heat[i]=heat[i]-cooldown;
				}
			}
			for( int i= PIXEL_NUM - 1; i >= 2; --i) {
				heat[i] = (heat[i - 1] + heat[i - 2]) / 2;
			}
			if( random() < 180 ) {
				i = random()%3;
				heat[i] = heat[i] + random()%100+100;
			}
			for(i = 0; i < PIXEL_NUM; ++i) {
				p.r = heat[i];
				p.g = heat[i]>>3;
				p.b = 0;
				setpixel(i, &p);
			}
		}
		else {//Off mode
			for(i = 0; i < PIXEL_NUM ; ++i) {
				setpixel(i, &blank);
			}
		}
	}
}

//Lighting an id pixel with p color
void
setpixel(uint8_t id, struct cRGB *p)
{
    pixels[id] = *p;
    ws2812_setleds(pixels, PIXEL_NUM);
}

//Calculating current color between primary rainbow colors
static void updateCurrentColor()
{
	static uint8_t currentRainbow = 0;
	if ((current.r == rainbow[currentRainbow].r)&&(current.g == rainbow[currentRainbow].g)&&(current.b == rainbow[currentRainbow].b)){
		currentRainbow++;
		if (currentRainbow>=RAINBOW_SIZE)
			currentRainbow = 0;
	}
	if(current.r<rainbow[currentRainbow].r)
		current.r++;
	if(current.r>rainbow[currentRainbow].r)
		current.r--;
	if(current.g<rainbow[currentRainbow].g)
		current.g++;
	if(current.g>rainbow[currentRainbow].g)
		current.g--;
	if(current.b<rainbow[currentRainbow].b)
		current.b++;
	if(current.b>rainbow[currentRainbow].b)
		current.b--;
}

///a primitive LFSR random number generator
static uint8_t random()
{
	static uint8_t current = 155;
	current = (current >> 0x01U) ^ (-(current & 0x01U) & 0xB4U);
	return current;
}
