/*
Must be compiled with this command:
gcc -Wall -o cacarpet cacarpet.c -lSDL -lSDL_ttf -fmax-errors=10
The SDL and SDL_ttf libraries must be installed
To generate a gif animation, the following programs must be installed:
pnmtopng
apng2gif
gifsicle
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#define FONT_FILENAME "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-C.ttf"
#define FONT_HEIGHT 20

// Will try to create a window with this resolution
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 900

#define FRAMEINFO_X (vid_x-SIDEBARWIDTH)
#define FRAMEINFO_Y Y_MARGIN
#define FRAMEINFO_W SIDEBARWIDTH
#define SIDEBARWIDTH 80

#define X_MARGIN 3
#define Y_MARGIN 3

#define SIZEMAX 2000

#define GETS_BUFFER_SIZE 100

#define TEXTCOLOR 0x00ff00
#define INPUTCOLOR1 0x00ffff
#define INPUTCOLOR2 0xff0000
//#define CURSORCOLOR 0xff0000

#define COLOR0_RED 64
#define COLOR0_GREEN 0
#define COLOR0_BLUE 0

#define COLOR1_RED 224
#define COLOR1_GREEN 224
#define COLOR1_BLUE 0


unsigned char pic[2][SIZEMAX][SIZEMAX];
int layer_draw=1;
int layer_get;

int vid_x;
int vid_y;

int max_width;
int max_height;

int doublepixel;

SDL_Surface *screen;
int vid_bpp;
int vid_pitch;
int colorcodes[16];
int cursorcolor;
TTF_Font *ttfont;

SDL_Event sdl_event;
int keydown;
int keycode;
int keysdown;
int quit;
int quit2;
int info_lines;

int width;
int halfwidth;
int halfwidth1;
int height;
int halfheight;
int halfheight1;
int cycles_done;

unsigned char row[200000];

char mkanim_cmd[50000];
char gets_buffer[GETS_BUFFER_SIZE+1];
int anim_on;

void initfont(void)
{
	if(TTF_Init())
	{
		printf("TTF_Init: %s\n",TTF_GetError());
		exit(1);
	}
	ttfont=TTF_OpenFont(FONT_FILENAME,FONT_HEIGHT);
	if (!ttfont)
	{
		printf("TTF_OpenFont: %s\n",TTF_GetError());
		exit(1);
	}
}

void initsdl(void)
{
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER))
	{
		printf("SDL_Init: %s\n",SDL_GetError());
		exit(1);
	}
	screen=SDL_SetVideoMode(WINDOW_WIDTH,WINDOW_HEIGHT,8,SDL_SWSURFACE);
	if(!screen)
	{
		printf("SDL_SetVideoMode: %s\n",SDL_GetError());
		exit(1);
	}
	//SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(150,30);
	vid_x=screen->w;
	vid_y=screen->h;
	vid_bpp = screen->format->BytesPerPixel;
	vid_pitch=screen->pitch;
	colorcodes[0]=SDL_MapRGB(screen->format,COLOR0_RED,COLOR0_GREEN,COLOR0_BLUE);
	colorcodes[1]=SDL_MapRGB(screen->format,COLOR1_RED,COLOR1_GREEN,COLOR1_BLUE);
	cursorcolor=SDL_MapRGB(screen->format,255,0,0);
}

void drawtext(int x, int y, int width, char *text, int rgb,int cursorcolor)
{
	SDL_Surface *textsur;
	SDL_Color ccc;
	SDL_Rect rrr;
//	SDL_Rect rrr2;
	rrr.w=0;
	if(text[0])
	{
		ccc.r=rgb>>16;
		ccc.g=(rgb>>8)&0xff;
		ccc.b=rgb&0xff;
		textsur=TTF_RenderUTF8_Solid(ttfont,text,ccc);
		if(!width)
			width=textsur->w;
	}
	rrr.x=x;
	rrr.y=y;
	rrr.w=width;
	rrr.h=30;
	SDL_FillRect(screen,&rrr,0);
	if(text[0])
	{
		SDL_BlitSurface(textsur,NULL,screen,&rrr);
		SDL_FreeSurface(textsur);
	}
	else
		width=FONT_HEIGHT;
	SDL_UpdateRect(screen,rrr.x,rrr.y,width,rrr.h);
	if(cursorcolor>0)
	{
		rrr.h=FONT_HEIGHT;
		rrr.x=x;
		if(text[0])
			rrr.x+=+rrr.w;
		rrr.w=3;
		SDL_FillRect(screen,&rrr,cursorcolor);
		SDL_UpdateRect(screen,rrr.x,rrr.y,rrr.w,rrr.h);	
	}
	SDL_UpdateRect(screen,x,y,width,FONT_HEIGHT);
}

void drawint(int x, int y, int width, int value, int rgb)
{
	char temp[50];
	sprintf(temp,"%d",value);
	drawtext(x,y,width,temp,rgb,0);
}

int getch_SDL(void)
{
	if( SDL_PollEvent( &sdl_event ) )
	{
		switch( sdl_event.type )
		{
			case SDL_QUIT:
				quit = 1;
				//return -1;
				break;
			case SDL_KEYDOWN:
				keydown=sdl_event.key.keysym.sym;
				keycode=sdl_event.key.keysym.unicode;
				keysdown++;
				return keydown;
			case SDL_KEYUP:
				keysdown=0;
				break;
		}
	}
	keydown=keycode=0;
	SDL_Delay(10);
	return -1;
}

int inputtext(int x, int y, int width, /*char *title, */char *initial_value)
{
	int ch;
	int len;
	int done;
	int result;
	SDL_Rect rrr;
	strcpy(gets_buffer,initial_value);
	//drawtext(x,y,width,title,INPUTCOLOR1,0);
	done=0;
	len=strlen(gets_buffer);
	do
	{
		drawtext(x,y/*+FONT_HEIGHT*/,width,gets_buffer,INPUTCOLOR2,cursorcolor);
		ch=getch_SDL();
		switch(ch)
		{
			case -1:
				break;
			case 10:
			case 13:
				result=done=1;
				break;
			case 8:
			case 127:
				if(len)
				{
					gets_buffer[len-1]=0;
					len--;
				}
				break;
			case 3:
			case 1:
			case 27:
				done=1;
				result=0;
			default:
				if ((len<GETS_BUFFER_SIZE-1)&&(ch>=' ')&&(ch<=255))
				{
					gets_buffer[len++]=ch;
					gets_buffer[len]=0;
				}
		}
	}
	while(!done);
	rrr.x=x;
	rrr.y=y;
	rrr.w=width;
	rrr.h=FONT_HEIGHT;// *2;
	SDL_FillRect(screen,&rrr,0);
	SDL_UpdateRect(screen,rrr.x,rrr.y,width,rrr.h);
	return result;
}

void putpixel(int x, int y, Uint32 pixel)
{
	((unsigned char *)screen->pixels)[y*vid_pitch+x]=pixel;
}

int getpixel(int x, int y)
{
	return ((unsigned char *)screen->pixels)[y*vid_pitch+x];
}

void putdot(int x, int y, int code)
{
	int x2;
	int y2;
	int color;
	pic[layer_draw][x][y]=code;
	x2=X_MARGIN+x;
	if(doublepixel)
		x2+=x;
	y2=Y_MARGIN+y;
	if(doublepixel)
		y2+=y;
	color=colorcodes[code];
	putpixel(x2,y2,color);
	if(doublepixel)
	{
		putpixel(x2,y2+1,color);
		putpixel(x2+1,y2,color);
		putpixel(x2+1,y2+1,color);
	}
}

int getcell18(int x, int y)
{
	if(x>=halfwidth)
		x=width-x-1;
	if(y>=halfheight)
		y=height-y-1;
	if((x<0)||(y<0))
		return 0;
	if(y>x)
		return pic[layer_get][y][x];
	return pic[layer_get][x][y];
}

int getcell14(int x, int y)
{
	if(x>=halfwidth)
		x=width-x-1;
	if(y>=halfheight)
		y=height-y-1;
	if((x<0)||(y<0))
		return 0;
	return pic[layer_get][x][y];
}

void clear_drawing(void)
{
	SDL_Rect rrr;
	rrr.x=X_MARGIN;
	rrr.y=Y_MARGIN;
	rrr.w=max_width;
	rrr.h=max_height;
	SDL_FillRect(screen,&rrr,0);
	SDL_UpdateRect(screen,rrr.x,rrr.y,rrr.w,rrr.h);
	info_lines=0; 
}

void updatepic(int half)
{
	int w;
	int h;
	w=half?halfwidth1:width;
	h=half?halfheight1:height;
	if(doublepixel)
	{
		w*=2;
		h*=2;
	}
	SDL_UpdateRect(screen,X_MARGIN,Y_MARGIN,w,h);
}

void copy14(void)
{
	int x;
	int y;
	int x2;
	int y2;	
	x2=width-1;
	y2=height-1;
	for (y=0;y<halfheight1;y++)
		for (x=0;x<halfwidth;x++)
			putdot(x2-x,y,pic[layer_draw][x][y]);
	for (y=0;y<halfheight;y++)
		for (x=0;x<width;x++)
			putdot(x,y2-y,pic[layer_draw][x][y]);
	updatepic(0);
}

void copy18(void)
{
	int x;
	int y;
	for (y=1;y<halfheight1;y++)
		for (x=0;x<y;x++)
			putdot(x,y,pic[layer_draw][y][x]);
	copy14();
}

void swaplayers(void)
{
	int layertemp;
	layertemp=layer_get;
	layer_get=layer_draw;
	layer_draw=layertemp;
}

void frameinfo(void)
{
	drawint(FRAMEINFO_X,FRAMEINFO_Y,FRAMEINFO_W,cycles_done,0xffffff);	
}

void drawcycles_(int count)
{
	int x;
	int y;
	int x2;
	int y2;
	//int x3;
	//int y3;
	int count2;
	int pixtotal;
	int pixcolor;
	if(info_lines)
		clear_drawing();
	// optimized code
	for (count2=0;count2<count;count2++)
	{
		swaplayers();
		if (width==height)
		{
			for (y=0;y<halfheight1;y++)
			{
				for (x=y;x<halfwidth1;x++)
				{
					pixtotal=0;
					for (x2=-1;x2<2;x2++)
					{
						for (y2=-1;y2<2;y2++)
							pixtotal+=getcell18(x+x2,y+y2);
					}
					pixcolor=(pixtotal&1)?0:1;
					putdot(x,y,pixcolor);
				}
			}
		}
		else
		{
			for (y=0;y<halfheight1;y++)
			{
				for (x=0;x<halfwidth1;x++)
				{
					pixtotal=0;
					for (x2=-1;x2<2;x2++)
					{
						for (y2=-1;y2<2;y2++)
							pixtotal+=getcell14(x+x2,y+y2);
					}
					pixcolor=(pixtotal&1)?0:1;
					putdot(x,y,pixcolor);
				}
			}
		}
		if(count2<count-1)
			updatepic(1);
		cycles_done++;
		frameinfo();
	}
	if(width==height)
		copy18();
	else
		copy14();
	
	// non-optimized code to test the correctness of
	// optimized code
	/*
	for (count2=0;count2<count;count2++)
	{
		swaplayers();
		for (y=0;y<size;y++)
		{
			for (x=0;x<size;x++)
			{
				pixtotal=0;
				for (x2=-1;x2<2;x2++)
				{
					x3=x+x2;
					if((x3<0)||(x3>=size))
						continue;
					for (y2=-1;y2<2;y2++)
					{
						y3=y+y2;
						if((y3<0)||(y3>=size))
							continue;
						pixtotal+=pic[layer_get][x3][y3];
					}
				}
				pixcolor=(pixtotal&1)?0:1;
				putdot(x,y,pixcolor);
			}
		}
		if(count2==count-1)
			updatepic(0);
		cycles_done++;
		frameinfo();
	}
	*/
		
}

// SDL_SaveBMP didn't work for some reason
void savebmp(void) //int doublepx)
{
	char fname[100];
	int x;
	int y;
	int x2;
	int pixel;
	int temp;
	int rowsize;
	int width2;
	int height2;
	FILE *bmp;
	sprintf(fname,"ca-%04d-%04d-%04d.bmp",width,height,cycles_done);
	bmp=fopen(fname,"wb");
	if(!bmp)
		return;
	memset(row,0,1078);
	row[0]='B';
	row[1]='M';
	width2=width*2;
	height2=height*2;
	temp=width2*height2+1078;
	row[2]=temp&0xff;
	row[3]=(temp>>8)&0xff;
	row[4]=(temp>>16)&0xff;
	row[10]=54;
	row[11]=4;
	row[14]=40;
	row[18]=width2&0xff;
	row[19]=(width2>>8)&0xff;
	row[22]=height2&0xff;
	row[23]=(height2>>8)&0xff;
	row[26]=1;
	row[28]=8;
	temp=width2*height2;
	row[34]=temp&0xff;
	row[35]=(temp>>8)&0xff;
	row[36]=(temp>>16)&0xff;
	row[47]=1;
	row[56]=64;
	row[59]=row[60]=224;
	fwrite(row,1,1078,bmp);
	rowsize=width2;
	temp=rowsize&3;
	if(temp)
		rowsize+=(4-temp);
	for (y=height-1;y>=0;y--)
	{
		x2=0;
		for (x=0;x<width;x++)
		{
			pixel=pic[layer_draw][x][y];
			row[x2++]=pixel;
			row[x2++]=pixel;
		}
		fwrite(row,1,rowsize,bmp);
		fwrite(row,1,rowsize,bmp);		
	}
	fclose(bmp);
}

void anim_saveframe(void)
{
	FILE *temp;
	char basename[100];
	char ppmname[100];
	char *pngname="_frame_temp.png";
	char gifname[100];
	int x;
	int y;
	int fw;
	int fh;
	int rowlen;
	int rowpos;
	int looping;
	int r;
	int g;
	int rowlen_6;
	char cmdlocal[100];
	fw=12+width*3;
	fh=42+height*3;
	sprintf(basename,"f_%04d_%04d_%04d",width,height,cycles_done);
	strcpy(ppmname,basename);
	strcat(ppmname,".ppm");
	strcpy(gifname,basename);
	strcat(gifname,".gif");
	temp=fopen(ppmname,"wb");
	if(!temp)
		abort();
	fprintf(temp,"P6\n%d %d\n255\n",fw,fh);
	rowlen=fw*3;
	rowlen_6=rowlen*6;
	memset(row,32,rowlen_6);
	fwrite(row,1,rowlen_6,temp);
	for (y=0;y<height;y++)
	{
		rowpos=18;
		for (x=0;x<width;x++)
		{
			if(pic[layer_draw][x][y])
				r=g=224;
			else
			{
				r=64;
				g=0;
			}
			for (looping=0;looping<3;looping++)
			{
				row[rowpos++]=r;
				row[rowpos++]=g;
				rowpos++;
			}
		}
		for (looping=0;looping<3;looping++)
			fwrite(row,1,rowlen,temp);
	}
	memset(row,32,rowlen_6);
	fwrite(row,1,rowlen_6,temp);
	for (y=0;y<30;y++)
	{
		memset(row,0,rowlen);
		for (x=0;x<FRAMEINFO_W;x++)
		{
			if(getpixel(FRAMEINFO_X+x,FRAMEINFO_Y+y)!=0)
				row[x*3+1]=192;
		}
		fwrite(row,1,rowlen,temp);
	}
	fclose(temp);
	/*
	This two-step operation shouldn't be necessary, but gif framed generated
	by ppmtogif didn't work. The gif assembled by gifsicle appeared
	to be a static gif with Firefox.
	*/
	sprintf(cmdlocal,"pnmtopng %s >%s",ppmname,pngname);
	system(cmdlocal);
	sprintf(cmdlocal,"apng2gif %s %s",pngname,gifname);
	system(cmdlocal);
	/*
	sprintf(cmdlocal,"ppmtogif %s >%s",ppmname,gifname);
	system(cmdlocal);
	*/
	strcat(gifname," ");
	strcat(mkanim_cmd,gifname);
}

void anim_start(void)
{
	if(anim_on)
		return;
	strcpy(mkanim_cmd,"#!/bin/sh\n#Generated file, do not keep\ngifsicle ");
	anim_on=1;
	frameinfo();
	anim_saveframe();
}

// fast drawing can't be used if an animation is made
void drawcycles(int count)
{
	int count2;
	if(!anim_on)
		drawcycles_(count);
	else
	{
		for (count2=0;count2<count;count2++)
		{
			drawcycles_(1);
			anim_saveframe();			
		}
	}
}

void anim_end(void)
{
	FILE *temp;
	char shname[100];
	//if(!mkanim_cmd[0])
		//return;
	sprintf(shname,"_mkanim_%04d_%04d_%04d",width,height,cycles_done);
	temp=fopen(shname,"wb");
	if(!temp)
		abort();
	fputs(mkanim_cmd,temp);
	fprintf(temp,">anim_%04d_%04d_%04d.gif",width,height,cycles_done);
	fclose(temp);
	chmod(shname,0777);
	anim_on=0;
	mkanim_cmd[0]=0;
}

void reset(void)
{
	memset(pic,0,sizeof pic);
	cycles_done=0;
	if(anim_on)
		anim_end();
}

void info_line(char *text1,char *text2)
{
	int y;
	y=Y_MARGIN+info_lines*FONT_HEIGHT;
	drawtext(X_MARGIN,y,0,text1,TEXTCOLOR,0);
	if(text2)
		drawtext(X_MARGIN+100,y,0,text2,TEXTCOLOR,0);
	info_lines++;
}

void splash(void)
{
	char text[50];
	clear_drawing();
	sprintf(text,"Current resolution: %d * %d",width,height);
	info_line(text,NULL);
	info_line("Q","quit");
	info_line("C","change resolution");
	info_line("S","save .bmp");
	info_line("A","begin making animation");
	//info_line("N","enter number of cycles to make");
	info_line("1 to 9","make 1 to 9 iterations");
}

void g_main(void)
{
	int ch;
	reset();
	splash();
	frameinfo();
	doublepixel=((width*2<=max_width)&&(height*2<=max_height))?1:0;
	quit2=0;
	while (!quit2)
	{
		ch=getch_SDL();
		switch(ch)
		{
			case 'q':
				quit=quit2=1;
				break;
			case 'c':
				quit2=1;
				break;
			case -1:
				break;
			case 's':
				savebmp();
				break;
			case 'a':
				anim_start();
				break;
			default:
				if((ch>='1')&&(ch<='9'))
					drawcycles(ch-'0');
				//else
					//quit=1;
				break;
		}
	}
	if(anim_on)
		anim_end();	
}

int main(int argc, char *argv[])
{
	char temp[100];
	char temp2[100];
	int value;
	int value2;
	int count;
	initfont();
	initsdl();
	max_width=vid_x-X_MARGIN-X_MARGIN-SIDEBARWIDTH;
	max_height=vid_y-Y_MARGIN-Y_MARGIN;
	width=height=255;
	while(!quit)
	{
		sprintf(temp2,"Enter grid size: (max. %d %d)",max_width,max_height);
		drawtext(20,20,500,temp2,INPUTCOLOR1,0);
		drawtext(20,70+FONT_HEIGHT,500,"(\"width height\", or 1 number)",INPUTCOLOR1,0);
		drawtext(20,70+2*FONT_HEIGHT,500,"for a square grid, of",INPUTCOLOR1,0);
		drawtext(20,70+3*FONT_HEIGHT,500,"blank to exit)",INPUTCOLOR1,0);
		sprintf(temp,"%d %d",width,height);
		if(!inputtext(20,40,300,temp))
			break;
		count=sscanf(gets_buffer,"%d %d",&value,&value2);
		if((count<1)||(value<3)||(value2<3))
			break;
		else if(count==1)
			width=height=value;
		else
		{
			width=value;
			height=value2;
		}
		halfwidth=halfwidth1=width/2;
		if(width&1)
			halfwidth1++;
		halfheight=halfheight1=height/2;
		if(height&1)
			halfheight1++;
		g_main();
		
	}
	SDL_Quit();
}
