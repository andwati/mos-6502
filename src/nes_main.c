#include <stdio.h>
#include <SDL2/SDL.h>
#include "nes.h"

int main(int argc,char **argv)
{
    NES nes;SDL_Window *w;SDL_Renderer *r;SDL_Texture *t;SDL_AudioDeviceID audio=0;uint32_t pixels[256*240];bool run=true;uint8_t pad=0;
    if(argc!=2){fprintf(stderr,"usage: %s game.nes\n",argv[0]);return 2;}
    if(nes_load(&nes,argv[1])){fprintf(stderr,"unsupported or invalid iNES ROM (supported mappers: 0, 2, 3)\n");return 1;}nes_reset(&nes);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER)!=0)return 1;
    w=SDL_CreateWindow("MOS 6502 - NES",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,768,720,SDL_WINDOW_RESIZABLE);
    r=w?SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC):NULL;if(w&&!r)r=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);
    t=r?SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,256,240):NULL;if(!t){SDL_Quit();return 1;}SDL_RenderSetLogicalSize(r,256,240);
    {SDL_AudioSpec want={0};want.freq=48000;want.format=AUDIO_F32SYS;want.channels=1;want.samples=1024;audio=SDL_OpenAudioDevice(NULL,0,&want,NULL,0);if(audio)SDL_PauseAudioDevice(audio,0);}
    while(run){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)run=false;if(e.type==SDL_KEYDOWN||e.type==SDL_KEYUP){bool on=e.type==SDL_KEYDOWN;uint8_t bit=0;switch(e.key.keysym.sym){case SDLK_x:bit=1;break;case SDLK_z:bit=2;break;case SDLK_RSHIFT:bit=4;break;case SDLK_RETURN:bit=8;break;case SDLK_UP:bit=16;break;case SDLK_DOWN:bit=32;break;case SDLK_LEFT:bit=64;break;case SDLK_RIGHT:bit=128;break;case SDLK_ESCAPE:run=false;break;default:break;}if(on)pad|=bit;else pad&=(uint8_t)~bit;nes_set_controller(&nes,pad);}}
        nes.frame_ready=false;while(run&&!nes.frame_ready){cpu_step_result_t step=nes_step(&nes);if(step.status!=CPU_STEP_OK){fprintf(stderr,"illegal opcode $%02X at $%04X\n",step.opcode,(uint16_t)(nes.cpu.PC-1));run=false;}}
        if(run){float samples[801];for(unsigned i=0;i<801;i++)samples[i]=nes_audio_sample(&nes,48000);if(audio&&SDL_GetQueuedAudioSize(audio)<sizeof(samples)*4)SDL_QueueAudio(audio,samples,sizeof samples);nes_render_frame(&nes,pixels);SDL_UpdateTexture(t,NULL,pixels,256*4);SDL_RenderClear(r);SDL_RenderCopy(r,t,NULL,NULL);SDL_RenderPresent(r);}
    }
    if(audio)SDL_CloseAudioDevice(audio);
    SDL_DestroyTexture(t);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
