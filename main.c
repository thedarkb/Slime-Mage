#include <SDL2/SDL_image.h>
#include <SDL2/SDL.h>

#define RESX 160
#define RESY 120

#define MAPH 26
#define MAPW 38
#define MAPS 12
#define TILESIZE 8
#define FRAMERATE 60
#define MOVSPD 2

#define ALIVE (1<<0)
#define WALL (1<<1)
#define FRAME (1<<2)
#define VICTORY (1<<3)

#define CAMERAX (RESX/2-state.playerX-TILESIZE/2)
#define CAMERAY (RESY/2-state.playerY-TILESIZE/2)

#define PGRIDX (((state.playerX+TILESIZE/2)-TILESIZE)/TILESIZE)
#define PGRIDY (((state.playerY+TILESIZE/2)-TILESIZE*3)/TILESIZE)

#define GRIDX(x) ((x-TILESIZE)/TILESIZE)
#define GRIDY(x) ((x-TILESIZE*3)/TILESIZE)

SDL_Window* w;
SDL_Surface* s;
SDL_Renderer* r;
SDL_Event keyIn;
SDL_Texture* sheet;
SDL_Texture* bgLayer;
const uint8_t* k;

typedef struct level {
	unsigned char grid[MAPH][MAPW];
	unsigned int lightSpeed;
	uint32_t mapColour;
	uint32_t cellColour;
	int spawnX;
	int spawnY;
} level;

typedef struct gameObject {
	char title[32];
	level levels[MAPS];
} gameObject;

struct {
	int playerX;
	int playerY;
	char freeze;
	unsigned char levelNo;
	level current;
} state;

gameObject loaded;

void renderTile(int tile, int x, int y) {
	SDL_Rect tileP={0,tile*TILESIZE,TILESIZE,TILESIZE};
	SDL_Rect screenP={x+CAMERAX,y+CAMERAY,TILESIZE,TILESIZE};
	SDL_RenderCopy(r,sheet,&tileP,&screenP);
}

void move(int x, int y) {
	int checkX=state.playerX+x;
	int checkY=state.playerY+y;

	printf("Player Grid: %u,%u\nContent: %u\n\n",GRIDX(checkX+TILESIZE/2),GRIDY(checkY+TILESIZE/2),state.current.grid[GRIDX(checkX+TILESIZE/2)][GRIDY(checkY+TILESIZE/2)]);

	if(GRIDX(checkX+TILESIZE/2)<0 || GRIDX(checkX+TILESIZE/2)>MAPW-1) return;
	if(GRIDY(checkY+TILESIZE/2)<0 || GRIDY(checkY+TILESIZE/2)>MAPH-1) return;

	if(state.current.grid[GRIDX(checkX+TILESIZE/2)][GRIDY(checkY+TILESIZE/2)] & ALIVE) return;
	if(state.current.grid[GRIDX(checkX+TILESIZE/2)][GRIDY(checkY+TILESIZE/2)] & WALL) return;

	/*for(int yC=0;yC<=TILESIZE;yC++) {
		for(int xC=0;xC<=TILESIZE;xC++) {
			
			if(state.current.grid[GRIDY(checkY+yC)][GRIDX(checkX+xC)] & WALL) return;
			if(state.current.grid[GRIDY(checkY+yC)][GRIDX(checkX+xC)] & ALIVE) return;
		}
	}*/
	
	state.playerX=checkX;
	state.playerY=checkY;
}

void input() {
	static int lastTap=0;

	if(k[SDL_SCANCODE_W]) move(0,-MOVSPD);
	if(k[SDL_SCANCODE_S]) move(0,MOVSPD);
	if(k[SDL_SCANCODE_A]) move(-MOVSPD,0);
	if(k[SDL_SCANCODE_D]) move(MOVSPD,0);

	if(k[SDL_SCANCODE_RIGHT]) state.current.grid[PGRIDY][PGRIDX+1]|=ALIVE;
	if(k[SDL_SCANCODE_LEFT]) state.current.grid[PGRIDY][PGRIDX-1]|=ALIVE;
	if(k[SDL_SCANCODE_UP]) state.current.grid[PGRIDY-1][PGRIDX]|=ALIVE;
	if(k[SDL_SCANCODE_DOWN]) state.current.grid[PGRIDY+1][PGRIDX]|=ALIVE;

	if(k[SDL_SCANCODE_SPACE] && !lastTap) {
		lastTap=1;
		if(state.freeze) state.freeze=0;
		else state.freeze=1;
	}

	if(!k[SDL_SCANCODE_SPACE]) lastTap=0;
}

int checkTile(level inState, int x, int y) {
	if(x<0 || x>MAPW-1) return 0;
	if(y<0 || y>MAPH-1) return 0;
	if(inState.grid[y][x] & WALL) return 0;
	if(inState.grid[y][x] & VICTORY) return 0;


	if(inState.grid[y][x] & ALIVE) return 1;
	return 0;
}

void propagate() {
	level initState=state.current;

	for(int y=0;y<MAPH;y++){
		for(int x=0;x<MAPW;x++) {
			char neighbours=0;
			if(state.current.grid[y][x] & WALL) continue;

			if(checkTile(initState,x-1,y-1)) neighbours++;
			if(checkTile(initState,x-1,y)) neighbours++;
			if(checkTile(initState,x-1,y+1)) neighbours++;

			if(checkTile(initState,x,y-1)) neighbours++;
			if(checkTile(initState,x,y+1)) neighbours++;

			if(checkTile(initState,x+1,y-1)) neighbours++;
			if(checkTile(initState,x+1,y)) neighbours++;
			if(checkTile(initState,x+1,y+1)) neighbours++;

			if(neighbours<2) state.current.grid[y][x] &= ~ALIVE;
			if(neighbours==3) {
				state.current.grid[y][x] |= ALIVE;
				state.current.grid[y][x] |= FRAME;
			}
			if(neighbours>3) state.current.grid[y][x] &= ~ALIVE;
		}
	}
}

int loop() {
	static unsigned int lastTick=0;
	static unsigned int frameCounter=0;

	int delay=1000/FRAMERATE-(SDL_GetTicks()-lastTick);
	if(delay>0 && lastTick) SDL_Delay(delay);

	input();
	if(frameCounter%state.current.lightSpeed == 0 && !state.freeze) propagate();

	SDL_RenderClear(r);
	SDL_Rect bgRect={CAMERAX,CAMERAY,320,240};
	SDL_RenderCopy(r,bgLayer,NULL,&bgRect);

	for(int y=0; y<MAPH; y++) {
		for(int x=0;x<MAPW;x++) {
			renderTile(0,x*TILESIZE+TILESIZE,y*TILESIZE+3*TILESIZE);

			if(state.current.grid[y][x] & WALL) {
				renderTile(1,x*TILESIZE+TILESIZE, y*TILESIZE+3*TILESIZE);
			} else if(state.current.grid[y][x] & ALIVE) {
				renderTile((2+((state.current.grid[y][x]&FRAME)>>2)),x*TILESIZE+TILESIZE, y*TILESIZE+3*TILESIZE);
			}
			if(rand()%10==0) state.current.grid[y][x]^=FRAME;
		}
	}
	renderTile(7,state.playerX,state.playerY);

	SDL_RenderPresent(r);
	lastTick=SDL_GetTicks();
	frameCounter++;
	return 0;
}

int main() {
	SDL_Init(SDL_INIT_VIDEO);
	w=SDL_CreateWindow("Slime Mage",0,0,640,480,SDL_WINDOW_OPENGL);
	s=SDL_GetWindowSurface(w);
	r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	k=SDL_GetKeyboardState(NULL);

	SDL_Surface* loader=IMG_Load("sheet.png");
	sheet=SDL_CreateTextureFromSurface(r, loader);
	SDL_FreeSurface(loader);

	loader=IMG_Load("bg.png");
	bgLayer=SDL_CreateTextureFromSurface(r,loader);
	SDL_FreeSurface(loader);

	SDL_RenderSetScale(r,4,4);

	memset(&state,0,sizeof state);
	memset(&loaded,0,sizeof loaded);
	state.current.lightSpeed=10;
	state.playerX=80;
	state.playerY=60;


	state.current.grid[0][0] |= WALL;

	state.current.grid[3][3]|=ALIVE;
	state.current.grid[4][3]|=ALIVE;
	state.current.grid[5][2]|=ALIVE;
	state.current.grid[6][2]|=ALIVE;
	state.current.grid[4][1]|=ALIVE;

	while(keyIn.type != SDL_QUIT){
		if(loop()) return 0;
		SDL_PollEvent(&keyIn);
	}
	return 0;
}