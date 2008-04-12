//---------------------------------------------------------------------------
// File:	demo1.c
// Author:	Tony Saveski, t_saveski@yahoo.com
//---------------------------------------------------------------------------
#include "g2.h"

//---------------------------------------------------------------------------
#define RAND_A 9301
#define RAND_C 49297
#define RAND_M 233280
static uint32 seed=0x12345678;

float crap_rand(void)
{
// small delay...
uint32 i;
for(i=0; i<0xFFF; i++) {}

	seed = (seed*RAND_A+RAND_C) % RAND_M;
	return((float)seed / (float)RAND_M);
}

//---------------------------------------------------------------------------
void pixel_demo(void)
{
uint32 i;

	g2_set_fill_color(0, 0, 0);
	g2_fill_rect(0, 0, g2_get_max_x(), g2_get_max_y());

	for(i=0; i<0xFFF; i++)
	{
		g2_set_color(
			(uint8)(crap_rand()*255), 
			(uint8)(crap_rand()*255), 
			(uint8)(crap_rand()*255));

		g2_put_pixel(
			(uint16)(crap_rand()*g2_get_max_x()), 
			(uint16)(crap_rand()*g2_get_max_y()));
	}
}

//---------------------------------------------------------------------------
void line_demo(void)
{
uint32 i;

	g2_set_fill_color(0, 0, 0);
	g2_fill_rect(0, 0, g2_get_max_x(), g2_get_max_y());

	for(i=0; i<0xFFF; i++)
	{
		g2_set_color(
			(uint8)(crap_rand()*255), 
			(uint8)(crap_rand()*255), 
			(uint8)(crap_rand()*255));

		g2_line(
			(uint16)(crap_rand()*g2_get_max_x()), 
			(uint16)(crap_rand()*g2_get_max_y()),
			(uint16)(crap_rand()*g2_get_max_x()), 
			(uint16)(crap_rand()*g2_get_max_y()));
	}
}

//---------------------------------------------------------------------------
void rect_demo(void)
{
uint32 i;

	g2_set_fill_color(0, 0, 0);
	g2_fill_rect(0, 0, g2_get_max_x(), g2_get_max_y());

	for(i=0; i<0xFFF; i++)
	{
		g2_set_color(
			(uint8)(crap_rand()*255), 
			(uint8)(crap_rand()*255), 
			(uint8)(crap_rand()*255));

		g2_rect(
			(uint16)(crap_rand()*g2_get_max_x()), 
			(uint16)(crap_rand()*g2_get_max_y()),
			(uint16)(crap_rand()*g2_get_max_x()), 
			(uint16)(crap_rand()*g2_get_max_y()));
	}
}

//---------------------------------------------------------------------------
void fill_demo(void)
{
uint32 i;
	g2_set_fill_color(0, 0, 0);
	g2_fill_rect(0, 0, g2_get_max_x(), g2_get_max_y());

	for(i=0; i<0xFFF; i++)
	{
		g2_set_fill_color(
			(uint8)(crap_rand()*255), 
			(uint8)(crap_rand()*255), 
			(uint8)(crap_rand()*255));

		g2_fill_rect(
			(uint16)(crap_rand()*g2_get_max_x()), 
			(uint16)(crap_rand()*g2_get_max_y()),
			(uint16)(crap_rand()*g2_get_max_x()),
			(uint16)(crap_rand()*g2_get_max_y()));
	}
}

//---------------------------------------------------------------------------
int main(int argc, char **argv)
{
	argc=argc;
	argv=argv;

	g2_init(PAL_512_256_32);

	// clear the screen
	g2_set_fill_color(0, 0, 0);
	g2_fill_rect(0, 0, g2_get_max_x(), g2_get_max_y());

	while(1)
	{
		pixel_demo();
		line_demo();
		rect_demo();
		fill_demo();
	}

	// ok...it will never get here...
	g2_end();
	return(0);
}


