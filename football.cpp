#include <stdio.h>
#include <stdlib.h>
#include "hardware/spi.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "Pico_ST7789.h"
#include "pico/time.h"


//#define USBDEBUG 0
//#define USBREAD 1

#define PROGMEM


#include "FreeMono9pt7b.h"
#include "FreeMonoBold9pt7b.h"
#include "FreeMonoOblique12pt7b.h"
#include "FreeMonoBold18pt7b.h"
#include "FreeMono12pt7b.h"

#define BUT_MASK 0xFFC3FFFF

#define LIGHTRED 0xFA08
#define LAWN 0x0300

#define TEAM_SEL_MODE 0
#define TIME_SEL_MODE 1
#define MATCH_MODE 2

#define ONESEC -1000000

#define LONGPRESS 1000000

#define N_GROUPS 8
#define TEAMS_PER_GROUP 4

// Global variables
uint8_t gr[]={0,0}, team[]={0,1};
uint16_t minutes = 45;
uint16_t seconds = 0;
uint16_t accel = 1;
uint32_t foff = 0x10100000;
uint16_t height = 75;
uint16_t width = 100;
int16_t  sign = -1; // used in time adjust
bool dirty = false; //indicates that results are not saved

uint16_t tft_width = 320;
//uint16_t tft_width = 240;
uint16_t tft_height = 240;

uint8_t sector[4096];
int8_t cur_res_page;
int8_t write_page;

typedef struct team_group_result {
  uint8_t team_id;
  uint8_t matches;
  uint8_t winners;
  uint8_t losers;
  uint8_t draws;
  uint8_t goals_shot;
  uint8_t goals_received;
  int8_t goal_difference;
  uint8_t points;
} team_group_result_s;

team_group_result_s team_group_results[N_GROUPS * TEAMS_PER_GROUP];

// offsets for the flag pictures
uint32_t offsets[] = {0 , 15000 , 30000 , 45000 , 60000 , 75000 , 90000 , 105000 , 120000 , 135000 , 150000 , 165000 , 180000 , 195000 , 210000 , 225000 , 240000 , 255000 , 270000 , 285000 , 300000 , 315000 , 330000 , 345000 , 360000 , 375000 , 390000 , 405000 , 420000 , 435000 , 450000 , 465000  };

// offsets for the big introduction pictures
uint32_t picoffsets[] = {0x10175300, 0x10191500, 0x101AD700, 0x101C9900};
uint32_t resultoffset = 0x1FF000;
// stadium is 320x240 pixel and hence 153600 long
// hence the first free page is at 0x101C9900 + 0x25800 = 0x101EF100
// hence there are 0x10200000 - 0x101EF100 = 0x10f00 bytes libre (69376 bytes)
// the eeprom goes from 0x10000000 to 0x10200000
// last page offset is 0x10200000 - 0x1000 = 0x101FF000
// this is where we put the results
char countries[][13] = { "Qatar", "Ecuador", "Senegal", "Netherlands",
           "England", "Iran", "USA", "Wales",
           "Argentina", "Saudi Arabia", "Mexico", "Poland",
           "France", "Australia", "Denmark", "Tunisia",
           "Espagne", "Costa Rica", "Germany", "Japan",
           "Belgium", "Canada", "Marocco", "Croatia",
           "Brazil", "Serbia", "Switzerland", "Cameroon",
           "Portugal", "Ghana", "Uruguay", "S.Korea"
};

void eraseSector()
{
#ifdef USBDEBUG
  putchar('e');
#else
  //printf("erasing 4096 at %08x\n", resultoffset);
  uint32_t irqstat = save_and_disable_interrupts();
  flash_range_erase( resultoffset, 4096 );
  restore_interrupts( irqstat );
#endif

  return;
}

void writePage()
{
  uint32_t off = cur_res_page * 256;
#ifdef USBDEBUG
  // need to copy the current page to write_page
  // for usb
  putchar('w');
  putchar( write_page );
  for (int i=off; i<off+256; i++)
    {
      putchar(sector[i]);
    }
#else
  //printf( "XIP_BASE %08x\n", XIP_BASE);
  //printf("flash range programm %08x %08x 256\n", resultoffset, off);
  //sleep_ms(200);
  uint32_t irqstat = save_and_disable_interrupts();
  flash_range_program( resultoffset + off, &(sector[off]), 256 );
  restore_interrupts( irqstat );
  //printf("done\n");
#endif
}

void writeResults()
{
  write_page = (write_page + 1)%16;
  if (write_page == 0)
    eraseSector();

  // for eeprom: write page
  writePage();
  dirty = false;
}

// There are 16 result pages of 256 bytes in one sector of 4k
// We find the last page which is not empty: this is the working
// page. The rest of the memory could be released, however we
// do not do this since we do not need to save on RAM.

void getCurrentResPage()
{
  uint8_t page;
  for (page = 0; page<16; page++)
    {
      bool empty = true;
      for ( uint8_t *ptr = sector + page*256; ptr < sector + (page+1)*256; ptr++ )
	{
	  if ( *ptr != 0xff )
	    {
	      empty = false;
	      continue;
	    }
	}
      if ( empty )
	{
	  if ( page == 0 )
	    {
	      // only happens if no result ever was written
	      cur_res_page = 15;
	      write_page = 15;
	    }
	  else
	    {
	      cur_res_page = (page-1);	  
	      write_page = cur_res_page;
	    }
	  return;
	}
    }
  // nothing empty i.e. we are at the last page
  cur_res_page = ( page-1 );
  write_page = cur_res_page;
}

void readSector()
{
#if defined (USBDEBUG) || defined(USBREAD)
  putchar('r');
  for (int i=0; i<4096; i++)
    {
      sector[i] = getchar();
    }
#else
  // make a copy of the sector since we night want to edit it
  int i=0;
  for( uint8_t *ptr= (uint8_t*)(resultoffset+XIP_BASE);
       ptr<(uint8_t*)(resultoffset+XIP_BASE+4096);
       ptr++,i++ )
    {
      sector[i] = *ptr;
    }
#endif
  getCurrentResPage();
}


void waitNoButton()
{
  uint32_t gpios = 0;
  
  while ( gpios != 0xFFFFFFFF ) {
    gpios = gpio_get_all() | BUT_MASK;
    sleep_ms(50);
  }
}



uint8_t getButton()
{

  waitNoButton();

  uint32_t gpios = gpio_get_all() | BUT_MASK;
  
  while ( gpios == 0xFFFFFFFF ) {
    gpios = gpio_get_all() | BUT_MASK;
    sleep_ms(50);
  }
  absolute_time_t start = get_absolute_time();
  uint32_t i;
  for ( i = 0; i<30; i++ )
    {
      if ((gpios & 1) == 0 )
	break;
      gpios >>= 1;
    }

  int64_t pressDuration = 0;
    
  while( gpios != 0xFFFFFFFF)
    {
      sleep_ms(50);
      gpios = gpio_get_all() | BUT_MASK;
      pressDuration = absolute_time_diff_us( start, get_absolute_time());
      if ( pressDuration > LONGPRESS )
	{
	  i = i | 0x80;
	  return i;
	}
    }
  return i;      
}

bool checkbutton( uint but )
{
  bool val = gpio_get(but);
  if ( val == true ) {
    return false;
  }

  while (val == false)
    {
      sleep_ms(50);
      val = gpio_get(but);
    }
  return true;
}

void displayTime( Pico_ST7789 *tft )
{
  char mstr[3], sstr[3];
  snprintf( mstr, 3, "%02d", minutes );
  snprintf( sstr, 3, "%02d", seconds );
  
  uint16_t color = WHITE;
  if (sign == 1)
    color = RED;

  tft->setFont( &FreeMono12pt7b );
  tft->drawTextG( tft_width/2-35, 235, mstr,  color, BLACK,1 ); 
  tft->drawTextG( tft_width/2-7, 235, ":",  color, BLACK,1 ); 
  tft->drawTextG( tft_width/2+7, 235, sstr, color, BLACK,1 ); 
  tft->setFont( &FreeMonoBold18pt7b );

}
bool timercb ( struct repeating_timer *t )
{
  
  if ((seconds == 0) && (sign == -1))
    {
      seconds = 59;
      minutes -= 1;
    }
  else if (( seconds == 59 ) && ( sign == 1 ) )
    {
      seconds = 0;
      minutes += 1;
    }
  else
    {
      seconds = seconds + sign * 1;
    }

  if ((seconds == 0) and (minutes == 0)) {
    sign = 1;
  }

  displayTime( (Pico_ST7789*)t->user_data );

  return true;
}

uint8_t match_mode( Pico_ST7789 &tft )
{
  sign = -1;
  uint16_t goals[2] = {0,0};

  // draw left flag
  int i = gr[0]*4+team[0];
  uint32_t offset = offsets[i] + foff;
  int x1off, x2off;
  x1off = tft_width/4-width/2;
  tft.drawRectWH( x1off-1, 29, width+2, height+2, YELLOW );
  tft.drawImage( x1off, 30, width, height, (uint16_t*)offset );
  tft.drawText( x1off, 115, countries[i], YELLOW, BLACK,1 );
  
  // draw right flag
  i = gr[1]*4+team[1];
  offset= offsets[i] + foff;
  x2off = 0.75*tft_width - width/2;
  tft.drawRectWH( x2off-1, 29, width+2, height+2, YELLOW );
  tft.drawImage( x2off, 30, width, height, (uint16_t*)offset );
  tft.drawText( x2off, 115, countries[i], YELLOW, BLACK,1 );

  // draw goals
  tft.setFont(&FreeMonoBold18pt7b);
  tft.drawTextG( tft_width/2-21,190, ":", GREEN, BLACK,2 );

  // set up the timer
  struct repeating_timer timer_s;
  displayTime(&tft);
  bool playing = false;
  bool stop = false;
  uint8_t obut;
  while (! stop)
    {
      char g1[3],g2[3];
      snprintf( g1, 3, "%d", goals[0] );
      snprintf( g2, 3, "%d", goals[1] );
      x2off=tft_width / 4 - 21;
      x1off=tft_width * 3 / 4 - 21;
      if (goals[0] > 9)
	x1off -= 20;
      if (goals[1] > 9)
	x2off -= 20;
      tft.setFont(&FreeMonoBold18pt7b);
      tft.drawTextG(x1off,190, g1, GREEN, BLACK, 2 );
      tft.drawTextG(x2off,190, g2, GREEN, BLACK, 2 );
      
      uint8_t but = getButton();
      obut = but;
      int gsign = 1;
      if (but & 0x80)
	gsign = -1;
      but &= 0x7f;
      if (but == BUTTON1)
	{
	  goals[0] += gsign;
	}
      else if( but == BUTTON2 )
	{
	  goals[1] += gsign;
	}
      else if( but == BUTTON3 )
	{
	  if ( playing ) {
	    cancel_repeating_timer( &timer_s );
	    playing = false;
	  } else {
	    add_repeating_timer_us( uint64_t(ONESEC/accel), timercb, &tft, &timer_s );
	    playing = true;
	  }
	  //but=BUTTON3;
	}
      else
	{
	  if (playing) {
	    cancel_repeating_timer(&timer_s);
	    playing = false;
	  }
	  stop = true;
	}
    }
  return obut;
}



uint8_t timeAdjust( Pico_ST7789 &tft )
{
  uint16_t intervals[] = {45,15,10,5};
  uint16_t ix = 0;
  tft.drawTextG( tft_width/2 - 6.5*14,20,"TIME SETTINGS", YELLOW, BLACK,1 );
  tft.drawFastHLine( 0,40,tft_width,YELLOW);

  tft.drawTextG( 10,  105, "Duration: ", YELLOW, BLACK, 1);
  tft.drawTextG( 180, 105, "min", YELLOW, BLACK, 1);

  tft.drawTextG( 10,  150, "Accel   : ", YELLOW, BLACK, 1);
  tft.drawTextG( 180, 150, "x", YELLOW, BLACK, 1);

  

  if (minutes==45)
    ix = 1;
  bool stop = false;
  uint8_t but;
  while (! stop)
    {
      // draw the time
      char minstr[3];
      snprintf( minstr, 3, "%02d", minutes );
      char accstr[3];
      snprintf( accstr, 3, "%d", accel);
      tft.fillRect(  135,85, 40,20, BLACK );
      tft.drawTextG( 140,105, minstr, RED, BLACK, 1);
      tft.fillRect(  135,130, 40,20, BLACK );
      tft.drawTextG( 140,150,accstr,RED,BLACK,1);
    
      but = getButton();
      if (but == BUTTON3)
	{
	  minutes = intervals[ix];
	  ix = (ix+1)%4;
	}
      else if( but == BUTTON1 )
	{
	  if (accel < 10) {
	    accel += 1;
	  }
	}
      else if( but == BUTTON2 )
	{
	  if (accel > 1) {
	    accel -= 1;
	  }
	}
      else
	stop = true;
    }
  return but;
}

uint8_t teamSelection( Pico_ST7789 &tft )
{
  uint8_t cteam = 0;
  
  tft.drawTextG( (tft_width/2 - 7*14),20,"TEAM SELECTION", YELLOW, BLACK,1 );
  tft.drawFastHLine( 0,40,tft_width,YELLOW);
  tft.drawFastVLine( tft_width/2,40,190,YELLOW);
  tft.drawTextG( tft_width/4-width/2,70,"Team 1", YELLOW, BLACK,1 );
  tft.drawTextG( tft_width*3/4-width/2,70,"Team 2", YELLOW, BLACK,1 );
  tft.drawTextG( tft_width/4-width/2,100, "Gr:", YELLOW,BLACK,1 );
  tft.drawTextG( tft_width*3/4-width/2,100, "Gr:", YELLOW,BLACK,1 );


  bool stop = false;
  uint8_t but;
  while ( ! stop )
    {
      tft.fillRect( 0,40,tft_width,5,BLACK);
	if (cteam == 0) {
	  tft.fillRect( 0,40,tft_width/2,5,YELLOW);
	  tft.drawFastHLine( tft_width/2,40,tft_width/2,YELLOW);
	} else {
	  tft.fillRect( tft_width/2,40,tft_width/2,5,YELLOW);
	  tft.drawFastHLine( 0,40,tft_width/2,YELLOW);
	}
      // draw group letter
      char gr1 = gr[0] + 0x41; 
      char gr2 = gr[1] + 0x41; 
      tft.fillRect( tft_width/4,80,20,25,BLACK);
      tft.fillRect( tft_width/4*3,80,20,25,BLACK);

      tft.drawCharG( tft_width/4,100,gr1,LIGHTRED,BLACK,1);
      tft.drawCharG( tft_width/4*3,100,gr2,LIGHTRED,BLACK,1);

      // draw flags
      
      int i = gr[0]*4+team[0];
      uint32_t offset = offsets[i] + foff;      

      tft.drawRectWH( tft_width/4-width/2-1, 129, width+2, height+2, YELLOW );
      tft.drawImage( tft_width/4-width/2, 130, width, height, (uint16_t*)offset );
      tft.fillRect( tft_width/4-width/2,220, width, 20 , BLACK);
      tft.drawText( tft_width/4-width/2,220,countries[i], YELLOW,BLACK,1);

      i = gr[1]*4+team[1];
      offset= offsets[i] + foff;      
      tft.drawRectWH( tft_width*3/4-width/2-1, 129, width+2, height+2, YELLOW );
      tft.drawImage( tft_width*3/4-width/2, 130, width, height, (uint16_t*)offset );
      tft.fillRect( tft_width*3/4-width/2,220, width, 20 , BLACK);
      tft.drawText( tft_width*3/4-width/2,220,countries[i], YELLOW,BLACK,1);

      but = getButton();
      if (but == BUTTON1)
	{
	  gr[cteam] = (gr[cteam]+1)%8;
	}
      else if( but == BUTTON2 )
	{
	  team[cteam] = (team[cteam]+1) % 4;
	}
      else if( but == BUTTON3 )
	{
	  cteam = (cteam + 1)&1;
	}
      else
	stop = true;
    }
  return but;
}

uint8_t enterMode( uint32_t mode, Pico_ST7789 &tft )
{
  tft.fillScreen( BLACK );
  uint8_t but;
  switch (mode) {
  case TEAM_SEL_MODE :
    but = teamSelection( tft );
    break;
  case TIME_SEL_MODE :
    but = timeAdjust( tft );
    break;
  case MATCH_MODE :
    but = match_mode( tft );
    break;
  }
  return but;
}

void drawGroup( Pico_ST7789 &tft, uint8_t group, uint8_t isel )
{
  // erase all but the headline
  tft.fillRect( 0,60, tft_width, tft_height-31, BLACK );
  char groupc =  group + 0x41; 
  tft.fillRect( tft_width/2+9*14,5,20,30,BLACK);
  tft.setFont( &FreeMono12pt7b );
  tft.drawCharG( tft_width/2+9*14,30,groupc,YELLOW,BLACK,1);

  uint8_t eq0,eq1,ic0,ic1,iline = 0;
  // match table
  uint16_t xoff=0, yoff=90;
  uint8_t g0=0, g1=0;
  char g0str[8], g1str[8];
  for (eq0 = 0; eq0<3; eq0++)
    {
      ic0 = group*4+eq0;
      for( eq1=eq0+1; eq1 < 4; eq1++ )
	{
	  if ( iline == isel )
	    tft.setFont( &FreeMonoBold9pt7b );
	  else
	    tft.setFont( &FreeMono9pt7b );
	  ic1 = group*4 + eq1;
	  tft.drawTextG( 0,yoff,countries[ic0],YELLOW,BLACK,1);
	  tft.drawCharG( tft_width/2-28,yoff,'-',YELLOW,BLACK,1);	  
	  tft.drawTextG( tft_width/2-15,yoff,countries[ic1],YELLOW,BLACK,1);

	  // get the results:
	  uint16_t pix = (cur_res_page * 256) + group * 12 + iline * 2;
	  g0 = sector[pix];
	  g1 = sector[pix+1];
	  if ( g0 == 0xff )
	    {
	      strcpy( g0str, "-" );
	    }
	  else
	    {
	      sprintf(g0str,"%d",g0);
	    }
	  if ( g1 == 0xff )
	    {
	      strcpy( g1str, "-" );
	    }
	  else
	    {
	      sprintf(g1str,"%d",g1);
	    }
	  // calc offsets
	  uint8_t coff = strlen(g0str)+strlen(g1str)+1;
	  tft.setFont( &FreeMonoBold9pt7b );
	  tft.drawTextG( tft_width - coff*11, yoff, g0str, RED, BLACK, 1 );
	  coff -= strlen(g0str);
	  tft.drawTextG( tft_width - coff*11, yoff, ":", RED, BLACK, 1 );
	  coff -= 1;
	  tft.drawTextG( tft_width - coff*11, yoff, g1str, RED, BLACK, 1 );
	  yoff += 24;
	  iline += 1;
	}
      g1++;
    }
  return;
}

void changeGoal( Pico_ST7789 &tft, uint8_t group, uint8_t isel, uint8_t gsign, uint8_t but )
{
  // get the results:
  uint16_t pix = (cur_res_page * 256) + group * 12 + isel * 2;
  if (but == BUTTON1)
    pix += 1;
  sector[pix] += gsign;
  dirty = true;
  if (sector[pix] < 0)
    sector[pix] = 0;

  drawGroup(tft,group,isel);

}

void swap_teams(uint8_t t1, uint8_t t2)
{
  team_group_result_s tmp = team_group_results[t2];
  team_group_results[t2]=team_group_results[t1];
  team_group_results[t1]=tmp;
}

void groupRanking()
{
  // pointers to the matches in the group for the various teams in the group:
  uint8_t team_id = 0; // an index to identify the team and find it's name in the country array
  for ( int igr = 0; igr < N_GROUPS; igr++ )
    {
      for ( int iteam = 0; iteam < TEAMS_PER_GROUP; iteam++ )
	{
	  int ix = igr*TEAMS_PER_GROUP + iteam;	  
	  team_group_results[ix].team_id = team_id;
	  team_group_results[ix].matches = 0;
	  team_group_results[ix].winners = 0;
	  team_group_results[ix].losers = 0;
	  team_group_results[ix].draws = 0;
	  team_group_results[ix].goals_shot = 0;
	  team_group_results[ix].goals_received = 0;
	  team_group_results[ix].goal_difference = 0;
	  team_group_results[ix].points = 0;

	  // now loop through all results of the group matches and adjust according to the matches
	  // where this team was involved.
	  uint8_t eq0, eq1, mix, res_off, us, them, iline=0;
	  for ( eq0=0; eq0<3; eq0++ )
	    {
	      for ( eq1=eq0+1; eq1<4; eq1++)
		{
		  mix = 10;
		  if (eq0 == iteam)
		    mix = 0;
		  else if ( eq1 == iteam )
		    mix = 1;
		  if ( mix < 2 )
		    {
		      // we found our team, calculate the index to tha result of the match and update the
		      // match information
		      // offset into the results page:
		      res_off = (cur_res_page * 256) + igr * 12 + iline*2;
		      // is our team on the right or the left side of the match line?
		      if ( mix == 0 )
			{
			  us = sector[res_off];
			  them = sector[res_off+1];
			}
		      else
			{
			  us = sector[res_off + 1];
			  them = sector[res_off];
			}
		      if ( us == 255 )
			{
			  iline += 1;
			  continue;
			}
		      // now update the info
		      team_group_results[ix].matches += 1;
		      team_group_results[ix].goals_shot += us;
		      team_group_results[ix].goals_received += them;
		      if ( us > them )
			{
			  team_group_results[ix].winners += 1;
			  team_group_results[ix].points += 3;
			}
		      else if ( us == them )
			{
			  team_group_results[ix].draws += 1;
			  team_group_results[ix].points += 1;
			}
		      else
			{
			  team_group_results[ix].losers += 1;
			}
		      team_group_results[ix].goal_difference =
			(int8_t)team_group_results[ix].goals_shot - (int8_t)team_group_results[ix].goals_received;
			
		    }
		  iline += 1;
		  //team_id += 1;
		}
	    }
	  team_id += 1;
	}
    // the group results are evaluated, now we bubble sort the group.
    uint8_t eq0, eq1;
    for( eq0=0; eq0<3; eq0++ )
    	for ( eq1=3; eq1>eq0; eq1-- )
    	  {
    	    uint8_t t1 = igr*TEAMS_PER_GROUP + eq1;
    	    uint8_t t2 = t1 - 1;
    	    if ( team_group_results[t1].winners > team_group_results[t2].winners )
    	      {
    		swap_teams(t1,t2);
    	      }
    	    else if ( team_group_results[t1].winners == team_group_results[t2].winners )
    	      if ( team_group_results[t1].goal_difference > team_group_results[t2].goal_difference )
    		{
    		  swap_teams(t1,t2);
    		}
    	      else if (team_group_results[t1].goal_difference == team_group_results[t2].goal_difference )
    		if ( team_group_results[t1].goals_shot > team_group_results[t2].goals_shot )
    		  {
    		    swap_teams( t1,t2 );
    		  }    
	  }
    }
}

void displayGroupRanking( Pico_ST7789 &tft, uint8_t group )
{
  tft.setFont( &FreeMonoOblique12pt7b );
  char mtxt[] = {"Group Ranking"};
  tft.drawTextG( tft_width/2-7.5*14,30, mtxt, 0x07e0, 0x07e0, 1 );
  tft.setFont( &FreeMono9pt7b );
  tft.fillRect(0,60,tft_width,tft_height-60,BLACK);
  char thead[] = { " M W D L P  GD  G" };
  uint16_t yoff = 90;
  tft.drawTextG ( tft_width - 17*11, yoff, thead, GREEN, BLACK, 1 );
  uint8_t ix_t = group*4;
  char line[18];
  for (uint8_t iteam = ix_t; iteam < ix_t+TEAMS_PER_GROUP; iteam++ )
    {
      yoff += 24;
      tft.drawTextG(0, yoff, countries[team_group_results[iteam].team_id], YELLOW, BLACK, 1);
      snprintf(line, 17, "%2d%2d%2d%2d%2d%4d%3d",
	       team_group_results[iteam].matches,
	       team_group_results[iteam].winners,
	       team_group_results[iteam].draws,
	       team_group_results[iteam].losers,
	       team_group_results[iteam].points,
	       team_group_results[iteam].goal_difference,
	       team_group_results[iteam].goals_shot
	       );
      tft.drawTextG( tft_width - 17*11, yoff, line, YELLOW, BLACK, 1);
    }
  getButton();
  tft.fillRect(0,60,tft_width,tft_height-60,BLACK);
}

void results_page(Pico_ST7789 &tft)
{
  tft.fillScreen( BLACK );
  uint8_t group = 0;
  
  tft.setFont( &FreeMonoOblique12pt7b );
  char mtxt[] = {"Group Matches"};
  tft.drawTextG( tft_width/2-7.5*14,30, mtxt, 0x07e0, 0x07e0, 1 );
  uint8_t isel = 0;
  
  bool stop = false;
  while ( ! stop )
    {
      drawGroup( tft, group, isel );
      
      uint8_t but = getButton();
      uint8_t gsign = 1;
      
      if (but & 0x80)
	gsign = -1;
      but &= 0x7f;

      // sign contains the long press
      // but is 1 2 3 or 4
      if ( but == BUTTON3 )
	{
	  if ( gsign == 1 )
	    {
	      isel = (isel+1)%6;
	      if ( dirty )
		writeResults();
	    }
	  else
	    {
	      groupRanking();
	      displayGroupRanking( tft, group);
	    }
	}
      else if ( (but == BUTTON1) || (but == BUTTON2))
	{
	  changeGoal( tft, group, isel, gsign, but );
	}
      else if ( gsign == 1 )
	{
	  group = (group + 1) % 8;
	  if (dirty )
	    writeResults();
	}
      else
	stop = true; 
    }
}

void footballField( Pico_ST7789 &tft )
{

  // alternative: show a picture
  // 0 : Coup
  // 1 : Coup black
  // 2 : ball
  // 3 : stadium: length 153600 of 0x25800
  tft.drawImageF( (tft_width-320)/2, 0, 320, 240, (uint16_t*)picoffsets[3] );
  tft.setFont( &FreeMonoOblique12pt7b );
  char mtxt[] = {"Worldcup 2022"};
  tft.drawTextG( tft_width/2-7.5*14,30, mtxt, 0x07e0, 0x07e0, 1 );
  getButton();

  tft.fillScreen( 0xEF7D);
  tft.drawImageF(( tft_width-240)/2, 0, 240, 240, (uint16_t*)picoffsets[2] );
  getButton();
  tft.fillScreen( WHITE);
  tft.drawImageF( (tft_width-240)/2, 0, 240, 240, (uint16_t*)picoffsets[0] );
  for (int i=0; i<6; i++) {
    sleep_ms(500);
    gpio_xor_mask( 1<<TFT_BACKLIGHT );
  }
  return;
  
  // draw a football field
  //float mtp = 200./105.;
  //uint16_t l = int(105*mtp + 0.5);
  //uint16_t b = int(70*mtp + 0.5);
  //uint16_t rm = int(9.15*mtp + 0.5);
  //uint16_t bstr = int( 2*20.12*mtp + 0.5);
  //uint16_t lstr = int( 16.46*mtp + 0.5);
  //uint16_t btraum = int( 2*8.144 * mtp + 0.5);
  //uint16_t ltraum = int( 5.49 * mtp + 0.5);
  //uint16_t bt = int(7.32 * mtp + 0.5);
  //uint16_t rmid = int(9.144 * mtp + 0.5);
  //uint16_t elf = int(11 * mtp + 0.5);
  //uint16_t xoff = 20;
  //uint16_t yoff = 60;
  //tft.fillScreen( LAWN );
  //tft.drawRectWH(xoff,yoff,l,b,WHITE);
  //tft.drawFastVLine( xoff+l/2, yoff, b, WHITE);
  //tft.drawRectWH(xoff,          yoff+(14.88*mtp), lstr, bstr, WHITE);
  //tft.drawRectWH(xoff+(l-lstr), yoff+(14.88*mtp), lstr, bstr, WHITE);
  //tft.drawRectWH(xoff,            yoff+(25.86*mtp), ltraum, btraum,WHITE);
  //tft.drawRectWH(xoff+(l-ltraum), yoff+(25.86*mtp), ltraum, btraum,WHITE);
  //tft.drawCircle( xoff + l/2, yoff + b/2, rmid, WHITE);
  //tft.drawPixel( xoff+elf, yoff+b/2, WHITE );
  //tft.drawPixel( xoff+l-elf, yoff+b/2, WHITE );
}

int main()
{
  stdio_init_all();
    // the input oiutput functions of the spi and the rst and the dc are adjusted in the init routing of the lib.	    

  gpio_init(TFT_BACKLIGHT);
  gpio_set_dir( TFT_BACKLIGHT, true);
  // exchange width and height since we work in landscape
  Pico_ST7789 tft = Pico_ST7789( tft_height,tft_width,true,1);
  tft.init();
  
  tft.invertDisplay( true );
  tft.clearScreen();
  gpio_put( TFT_BACKLIGHT, 1 );
  
  //setup a gpio for the button
  gpio_init(BUTTON1);
  gpio_set_dir( BUTTON1, false);
  gpio_pull_up( BUTTON1 );
  gpio_init(BUTTON2);
  gpio_set_dir( BUTTON2, false);
  gpio_pull_up( BUTTON2 );
  gpio_init(BUTTON3);
  gpio_set_dir( BUTTON3, false);
  gpio_pull_up( BUTTON3 );
  gpio_init(BUTTON4);
  gpio_set_dir( BUTTON4, false);
  gpio_pull_up( BUTTON4 );
  bool but1 = true;
  bool but2 = true;
  bool but3 = true;
  bool but4 = true;

  footballField( tft );
  readSector();
  uint16_t nflag = 32;
  uint16_t iflag = 0;
  uint8_t width = 100;
  uint8_t height = 75;
  uint32_t offset;
  uint16_t i = 0;
  uint16_t xoff = 10;
  uint16_t yoff = 0 ;    
  uint32_t foff = 0x10100000;
  uint8_t but = 0;
  
  but = getButton();
  while(1) {
    tft.setFont( &FreeMonoOblique12pt7b );
    if( but & 0x80 )
      {
	results_page( tft );
      }
    but = enterMode( TEAM_SEL_MODE, tft );
    but = enterMode( TIME_SEL_MODE, tft );
    but = enterMode( MATCH_MODE, tft );
  }
}

