#include <stdio.h>
#include "frontend.h"

#ifdef HAVE_SDL_UI
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <string.h>
#include "state.h"

static const uint32_t palette[16] = {
    0x071018FF,0x62F294FF,0xFF5D73FF,0x8FFFAFFF,0x638CFFFF,0xD879FFFF,
    0x4DE1FFFF,0xFFD166FF,0xFF934FFF,0x9B6B43FF,0xFF9AA9FF,0x34445AFF,
    0x65758BFF,0xB6FFC9FF,0xA9BCFFFF,0xEAF2FFFF
};

static uint8_t load_high_score(void)
{
    FILE *file=fopen("snake.highscore","rb"); int value;
    if(!file) return 0;
    value=fgetc(file); fclose(file);
    return value==EOF?0:(uint8_t)value;
}

static void save_high_score(uint8_t score)
{
    FILE *file=fopen("snake.highscore","wb");
    if(file){fputc(score,file);fclose(file);}
}

static void play_tone(SDL_AudioDeviceID device, int frequency, int milliseconds)
{
    int16_t samples[4410];int count=44100*milliseconds/1000;
    if(!device)return;
    if(count>(int)(sizeof samples/sizeof samples[0]))count=(int)(sizeof samples/sizeof samples[0]);
    for(int i=0;i<count;i++)samples[i]=((i*frequency/44100)&1)?3500:-3500;
    SDL_QueueAudio(device,samples,(Uint32)(count*sizeof samples[0]));SDL_PauseAudioDevice(device,0);
}

static void update_title(SDL_Window *window, uint8_t score, uint8_t high, bool paused)
{
    char title[96];
    snprintf(title,sizeof title,"MOS 6502 Snake - Score: %u  High: %u%s",
             score,high,paused?"  [PAUSED]":"");
    SDL_SetWindowTitle(window,title);
}

static void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                      SDL_Color color, int center_x, int y)
{
    SDL_Surface *surface=TTF_RenderUTF8_Blended(font,text,color);
    SDL_Texture *texture;
    SDL_Rect target;
    if(!surface) return;
    texture=SDL_CreateTextureFromSurface(renderer,surface);
    target=(SDL_Rect){center_x-surface->w/2,y,surface->w,surface->h};
    SDL_FreeSurface(surface);
    if(texture){SDL_RenderCopy(renderer,texture,NULL,&target);SDL_DestroyTexture(texture);}
}

static void render_menu(SDL_Renderer *renderer, TTF_Font *title_font,
                        TTF_Font *item_font, TTF_Font *small_font,
                        unsigned selected, bool started, uint8_t high)
{
    const char *items[4]={started?"Resume Game":"Start Game","New Game",
                          "Reset High Score","Quit"};
    char high_text[48];
    SDL_RenderSetLogicalSize(renderer,832,560);
    SDL_SetRenderDrawColor(renderer,7,12,22,255);SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer,14,25,40,255);
    for(int x=-32;x<864;x+=32)for(int y=-32;y<592;y+=32){SDL_Rect tile={x,y,30,30};SDL_RenderDrawRect(renderer,&tile);}
    draw_text(renderer,title_font,"SNAKE 6502",(SDL_Color){105,240,150,255},416,38);
    draw_text(renderer,small_font,"A TINY 8-BIT ARCADE",(SDL_Color){115,135,165,255},416,95);
    for(unsigned i=0;i<4;i++) {
        SDL_Rect shadow={245,143+(int)i*67,350,50};
        SDL_Rect card={241,139+(int)i*67,350,50};
        SDL_SetRenderDrawColor(renderer,2,5,10,180);SDL_RenderFillRect(renderer,&shadow);
        if(i==selected){SDL_SetRenderDrawColor(renderer,31,105,72,255);SDL_RenderFillRect(renderer,&card);SDL_SetRenderDrawColor(renderer,105,240,150,255);SDL_RenderDrawRect(renderer,&card);}
        else {SDL_SetRenderDrawColor(renderer,20,31,48,255);SDL_RenderFillRect(renderer,&card);}
        draw_text(renderer,item_font,items[i],i==selected?(SDL_Color){245,255,248,255}:(SDL_Color){170,184,205,255},416,149+(int)i*67);
    }
    snprintf(high_text,sizeof high_text,"HIGH SCORE  %u",high);
    draw_text(renderer,small_font,high_text,(SDL_Color){250,198,82,255},416,420);
    draw_text(renderer,small_font,"W/S OR ARROWS TO MOVE  -  ENTER TO SELECT",
              (SDL_Color){100,120,145,255},416,468);
    SDL_RenderPresent(renderer);
}

static void render_game(SDL_Renderer *renderer, SDL_Texture *texture,
                        TTF_Font *title_font, TTF_Font *item_font,
                        TTF_Font *small_font, uint8_t score, uint8_t high,
                        bool paused, bool game_over)
{
    SDL_Rect board_shadow={28,28,520,520},board={20,20,520,520};
    SDL_Rect screen={24,24,512,512},panel={560,20,248,520};
    char value[32];
    SDL_RenderSetLogicalSize(renderer,832,560);
    SDL_SetRenderDrawColor(renderer,5,10,18,255);SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer,1,3,8,255);SDL_RenderFillRect(renderer,&board_shadow);
    SDL_SetRenderDrawColor(renderer,41,62,82,255);SDL_RenderFillRect(renderer,&board);
    SDL_RenderCopy(renderer,texture,NULL,&screen);
    SDL_SetRenderDrawColor(renderer,98,242,148,255);SDL_RenderDrawRect(renderer,&screen);
    SDL_SetRenderDrawColor(renderer,15,25,39,255);SDL_RenderFillRect(renderer,&panel);
    SDL_SetRenderDrawColor(renderer,31,51,72,255);SDL_RenderDrawRect(renderer,&panel);
    draw_text(renderer,title_font,"SNAKE",(SDL_Color){98,242,148,255},684,45);
    draw_text(renderer,small_font,"MOS 6502 ARCADE",(SDL_Color){108,132,158,255},684,96);
    draw_text(renderer,small_font,"SCORE",(SDL_Color){130,150,175,255},684,145);
    snprintf(value,sizeof value,"%03u",score);
    draw_text(renderer,title_font,value,(SDL_Color){245,250,255,255},684,164);
    draw_text(renderer,small_font,"HIGH SCORE",(SDL_Color){130,150,175,255},684,232);
    snprintf(value,sizeof value,"%03u",high);
    draw_text(renderer,item_font,value,(SDL_Color){255,209,102,255},684,253);
    SDL_SetRenderDrawColor(renderer,38,55,73,255);{SDL_Rect line={588,310,192,1};SDL_RenderFillRect(renderer,&line);}
    draw_text(renderer,small_font,"MOVE",(SDL_Color){98,242,148,255},684,334);
    draw_text(renderer,small_font,"W A S D  /  ARROWS",(SDL_Color){190,202,218,255},684,359);
    draw_text(renderer,small_font,"PAUSE",(SDL_Color){98,242,148,255},684,399);
    draw_text(renderer,small_font,"SPACE  /  P",(SDL_Color){190,202,218,255},684,424);
    draw_text(renderer,small_font,"ESC  MENU",(SDL_Color){108,132,158,255},684,488);
    if(paused||game_over){
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer,3,8,14,215);SDL_RenderFillRect(renderer,&screen);
        draw_text(renderer,title_font,game_over?"GAME OVER":"PAUSED",
                  game_over?(SDL_Color){255,93,115,255}:(SDL_Color){255,209,102,255},280,224);
        draw_text(renderer,small_font,game_over?"PRESS R TO PLAY AGAIN":"PRESS SPACE OR P TO RESUME",
                  (SDL_Color){225,234,244,255},280,284);
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
    }
    SDL_RenderPresent(renderer);
}

int frontend_run(CPU6502 *cpu, Bus6502 *bus, unsigned hz, unsigned scale)
{
    SDL_Window *window=NULL; SDL_Renderer *renderer=NULL; SDL_Texture *texture=NULL;
    SDL_GameController *controller=NULL;
    SDL_AudioDeviceID audio=0;SDL_AudioSpec desired={0};
    TTF_Font *title_font=NULL,*item_font=NULL,*small_font=NULL;
    uint32_t pixels[32*32]; uint64_t last, freq; int64_t budget=0;
    uint8_t score=0, high=load_high_score(), game_state=0; unsigned selected=0;
    bool running=true, paused=false, menu=true, started=false;
    const char *font_path=getenv("MOS6502_FONT");
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER)!=0) goto fail;
    if(TTF_Init()!=0) goto fail;
    if(!font_path) font_path="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    title_font=TTF_OpenFont(font_path,42);item_font=TTF_OpenFont(font_path,24);small_font=TTF_OpenFont(font_path,14);
    if(!title_font||!item_font||!small_font) goto fail;
    desired.freq=44100;desired.format=AUDIO_S16SYS;desired.channels=1;desired.samples=512;
    audio=SDL_OpenAudioDevice(NULL,0,&desired,NULL,0);
    window=SDL_CreateWindow("MOS 6502 Snake",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
                            32*(int)scale+320,32*(int)scale+48,SDL_WINDOW_RESIZABLE);
    renderer=window?SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC):NULL;
    if(window&&!renderer) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    texture=renderer?SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,32,32):NULL;
    if (!texture) goto fail;
    for(int i=0;i<SDL_NumJoysticks()&&!controller;i++)
        if(SDL_IsGameController(i)) controller=SDL_GameControllerOpen(i);
    bus->memory.data[0x00FC]=high;
    SDL_RenderSetLogicalSize(renderer,32,32); freq=SDL_GetPerformanceFrequency(); last=SDL_GetPerformanceCounter();
    render_menu(renderer,title_font,item_font,small_font,selected,started,high);
    while(running) {
        SDL_Event event; uint64_t now=SDL_GetPerformanceCounter(), elapsed=now-last; last=now;
        while(SDL_PollEvent(&event)) {
            if(event.type==SDL_QUIT) running=false;
            if(event.type==SDL_KEYDOWN) {
                SDL_Keycode k=event.key.keysym.sym;
                if(menu) {
                    if(!event.key.repeat && (k==SDLK_UP||k==SDLK_w)){selected=(selected+3)%4;render_menu(renderer,title_font,item_font,small_font,selected,started,high);}
                    else if(!event.key.repeat && (k==SDLK_DOWN||k==SDLK_s)){selected=(selected+1)%4;render_menu(renderer,title_font,item_font,small_font,selected,started,high);}
                    else if(!event.key.repeat && (k==SDLK_RETURN||k==SDLK_SPACE)) {
                        if(selected==0){menu=false;started=true;budget=0;bus->framebuffer_dirty=true;update_title(window,score,high,paused);}
                        else if(selected==1){cpu_reset(cpu,bus);score=0;paused=false;menu=false;started=true;budget=0;bus->framebuffer_dirty=true;update_title(window,score,high,false);}
                        else if(selected==2){high=0;bus->memory.data[0x00FC]=0;save_high_score(0);render_menu(renderer,title_font,item_font,small_font,selected,started,high);}
                        else running=false;
                    } else if(!event.key.repeat && k==SDLK_ESCAPE && started){menu=false;budget=0;bus->framebuffer_dirty=true;update_title(window,score,high,paused);}
                }
                else if(k==SDLK_ESCAPE) {
                    menu=true;selected=0;budget=0;render_menu(renderer,title_font,item_font,small_font,selected,started,high);
                }
                else if(!event.key.repeat && (k==SDLK_SPACE||k==SDLK_p)) {
                    paused=!paused; budget=0; bus->framebuffer_dirty=true; update_title(window,score,high,paused);
                }
                else if(!event.key.repeat && k==SDLK_F5) {
                    if(state_save("snake.state",cpu,bus)==0) SDL_SetWindowTitle(window,"MOS 6502 Snake - State saved");
                }
                else if(!event.key.repeat && k==SDLK_F9) {
                    if(state_load("snake.state",cpu,bus)==0){score=bus->memory.data[0x00FD];game_state=bus->memory.data[0x00FB];update_title(window,score,high,paused);}
                }
                else if(k==SDLK_w||k==SDLK_UP) bus_set_keyboard(bus,'w');
                else if(k==SDLK_a||k==SDLK_LEFT) bus_set_keyboard(bus,'a');
                else if(k==SDLK_s||k==SDLK_DOWN) bus_set_keyboard(bus,'s');
                else if(k==SDLK_d||k==SDLK_RIGHT) bus_set_keyboard(bus,'d');
                else if(k==SDLK_r) bus_set_keyboard(bus,'r');
            }
            if(!menu&&event.type==SDL_CONTROLLERBUTTONDOWN){
                if(event.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP)bus_set_keyboard(bus,'w');
                else if(event.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)bus_set_keyboard(bus,'s');
                else if(event.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_LEFT)bus_set_keyboard(bus,'a');
                else if(event.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_RIGHT)bus_set_keyboard(bus,'d');
                else if(event.cbutton.button==SDL_CONTROLLER_BUTTON_START){paused=!paused;budget=0;bus->framebuffer_dirty=true;}
            }
        }
        if(paused||menu) budget=0;
        else {
            budget += (int64_t)(elapsed*hz);
            if (budget > (int64_t)(freq*hz/4)) budget=(int64_t)(freq*hz/4);
        }
        while(running && !paused && !menu && budget>=(int64_t)freq) {
            cpu_step_result_t step=cpu_step(cpu,bus);
            if(step.status!=CPU_STEP_OK){fprintf(stderr,"illegal opcode $%02X at $%04X\n",step.opcode,(uint16_t)(cpu->PC-1));running=false;break;}
            budget -= (int64_t)step.cycles*(int64_t)freq;
        }
        if(score!=bus->memory.data[0x00FD]) {
            uint8_t old_score=score;score=bus->memory.data[0x00FD];
            if(score>high){high=score;bus->memory.data[0x00FC]=high;save_high_score(high);}
            if(score>old_score)play_tone(audio,880,70);
            update_title(window,score,high,paused);
            bus->framebuffer_dirty=true;
        }
        if(game_state!=bus->memory.data[0x00FB]){
            game_state=bus->memory.data[0x00FB];bus->framebuffer_dirty=true;
            if(game_state)play_tone(audio,150,220);
        }
        if(!menu && bus->framebuffer_dirty) {
            for(unsigned i=0;i<1024;i++) pixels[i]=palette[bus->memory.data[0x0200+i]&15];
            SDL_UpdateTexture(texture,NULL,pixels,32*sizeof(*pixels));
            render_game(renderer,texture,title_font,item_font,small_font,score,high,paused,
                        bus->memory.data[0x00FB]!=0);
            bus->framebuffer_dirty=false;
        }
        SDL_Delay(1);
    }
    save_high_score(high);if(audio)SDL_CloseAudioDevice(audio);if(controller)SDL_GameControllerClose(controller);TTF_CloseFont(small_font);TTF_CloseFont(item_font);TTF_CloseFont(title_font);SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();return 0;
fail:
    fprintf(stderr,"SDL UI error: %s%s%s\n",SDL_GetError(),*TTF_GetError()?" / ":"",TTF_GetError());
    if(small_font) TTF_CloseFont(small_font);
    if(item_font) TTF_CloseFont(item_font);
    if(title_font) TTF_CloseFont(title_font);
    if(controller) SDL_GameControllerClose(controller);
    if(audio) SDL_CloseAudioDevice(audio);
    if(texture) SDL_DestroyTexture(texture);
    if(renderer) SDL_DestroyRenderer(renderer);
    if(window) SDL_DestroyWindow(window);
    TTF_Quit();SDL_Quit();
    return 1;
}
#else
int frontend_run(CPU6502 *cpu, Bus6502 *bus, unsigned hz, unsigned scale)
{
    (void)cpu;(void)bus;(void)hz;(void)scale;
    fprintf(stderr,"SDL2/SDL2_ttf support was not built; install their development files and rebuild\n"); return 1;
}
#endif
