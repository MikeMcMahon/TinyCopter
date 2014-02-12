#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"

#ifdef DEBUG
#define DEBUG_MSG(X) printf("Occurred: %s, at line %d in %s\n", X, __LINE__, __FILE__)
#else
#define DEBUG_MSG(X) printf("")
#endif




const int window_width = 250;
const int window_height = 140;
const float TERM_VELOCITY_UP = -2;
const float TERM_VELOCITY_DOWN = 2;
const float DOWN_VELOCITY = 0.04;
const float UP_VELOCITY = 0.06;

#ifdef DEBUG
const char* GAME_FONT_FILE = "E:\\Development\\dependencies\\fonts\\PressStart2P-Regular.ttf";
#else
const char* GAME_FONT_FILE = "PressStart2P-Regular.ttf"
#endif

struct Scene
{
        int w;
        int h;
        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Texture *texture;
        SDL_Surface *surface;
};

struct TinyCopter
{
        int x;
        float y;
        int h;
        int w;
        float velocity;
        SDL_Surface *surface;
};

struct Pipe
{
        int x;
        int y;
        int h;
        int w;
        SDL_Surface *surface;
};

SDL_Rect SDL_CreateRect(int x, int y, int h, int w)
{
        SDL_Rect r;
        r.x = x; r.y = y;
        r.h = h; r.w = w;

        return r;
}

SDL_Surface * SDL_Create32BitSurface(int w, int h)
{
    SDL_Surface *surface;
    Uint32 rmask, gmask, bmask, amask;

    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
       on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
        surface = SDL_CreateRGBSurface(0, w, h, 32,
                                       rmask, gmask, bmask, amask);
        if (surface == NULL)
                return (DEBUG_MSG(SDL_GetError()), NULL);

        return surface;
}

/**
 * translates the pipes along the page to the left
 * returns non-zero if wrapped the screen
 */
int Pipe_Move(struct Pipe *pipe, struct Scene *scene)
{
        pipe->x--;
        if (pipe->x + pipe->w < 0) {
                pipe->x = scene->w;
                return 1;
        }
        return 0;
}

/**
 * Generates a new set of top/bottom pipes when they translate off the page
 */
void Pipe_Update(struct Pipe *top, struct Pipe *bottom, struct Scene *scene)
{
        SDL_Rect pipe_rect;
        top->h = (rand() % (scene->h - 30)) + 30;

        SDL_FillRect(top->surface, NULL,
                     SDL_MapRGBA(top->surface->format, 0, 0, 0, 0));
        SDL_FillRect(bottom->surface, NULL,
                     SDL_MapRGBA(bottom->surface->format, 0, 0, 0, 0));

        // top pipe
        pipe_rect.h = top->h;
        pipe_rect.w = 18;
        pipe_rect.x = 6;
        pipe_rect.y = 0;
        SDL_FillRect(top->surface, &pipe_rect,
                     SDL_MapRGBA(top->surface->format, 0, 255, 0, 255));

        pipe_rect.w = 30;
        pipe_rect.h = 16;
        pipe_rect.y = top->h - 16;
        pipe_rect.x = 0;
        SDL_FillRect(top->surface, &pipe_rect,
                     SDL_MapRGBA(top->surface->format, 0, 255, 0, 255));

        // bottom pipe
        int remainder = scene->h - top->h;
        int size = (rand() % remainder) + 30;
        if (size + top->h > scene->h)
                size = 0;

        bottom->h = size; //(rand() % ((scene.h - pipe_top.h) + 35));
        pipe_rect.h = bottom->h;
        pipe_rect.w = 18;
        pipe_rect.x = 6;
        pipe_rect.y = 0; //scene->h - bottom->h;
        SDL_FillRect(bottom->surface, &pipe_rect,
                     SDL_MapRGBA(bottom->surface->format, 0, 255, 0, 255));

        pipe_rect.w = 30;
        pipe_rect.h = 16;
        pipe_rect.x = 0;
        SDL_FillRect(botom->surface, &pipe_rect,
                     SDL_MapRGBA(bottom->surface->format, 0, 255, 0, 255));

}

/**
 * Translates the tc up!
 */
void TC_Ascend(struct TinyCopter *tinycopter)
{
        if (tinycopter->velocity > TERM_VELOCITY_UP)
                tinycopter->velocity -= UP_VELOCITY;
}

/**
 * translates the tc down
 */
void TC_Descend(struct TinyCopter *tinycopter)
{
        if (tinycopter->velocity < TERM_VELOCITY_DOWN)
                tinycopter->velocity += DOWN_VELOCITY;
}

int main(int argc, char* argv[])
{
        int quit = 0;
        SDL_Event e;
        struct Scene scene;
        unsigned int previous;
        unsigned int current = 0;
        float lag = 0;
        unsigned int elapsed = 0;
        float frames_per_second = 60;
        float ms_per_update = (1000 / frames_per_second);
        float interpolation;
        struct TinyCopter tinycopter;
        struct Pipe pipe_top;
        struct Pipe pipe_bottom;
        SDL_Rect draw_rect;
        SDL_Rect pipe_top_rect;
        SDL_Rect pipe_bottom_rect;

        srand(time(NULL));

        if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
                return (DEBUG_MSG(SDL_GetError()) -1);

        if (TTF_Init() != 0)
                return (DEBUG_MSG(TTF_GetError()), -1);

        scene.w = window_width;
        scene.h = window_height;

        scene.window = SDL_CreateWindow(
                 "TinyCopter",
                 SDL_WINDOWPOS_CENTERED,
                 SDL_WINDOWPOS_CENTERED,
                 scene.w,
                 scene.h,
                 SDL_WINDOW_SHOWN);
        if (scene.window == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        scene.renderer = SDL_CreateRenderer(
                                    scene.window,
                                    -1,
                                    SDL_RENDERER_ACCELERATED);
        if (scene.window == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        scene.surface = SDL_GetWindowSurface(scene.window);
        if (scene.surface == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        scene.texture = SDL_CreateTextureFromSurface(
                                             scene.renderer,
                                             scene.surface);
        if (scene.texture == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        IMG_Init(IMG_INIT_PNG);
        tinycopter.surface = IMG_Load("tinycopter.png");
        if (tinycopter.surface == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        /*
         * Configuring the TinyCopter..
         */
        tinycopter.x = 40;
        tinycopter.y = (scene.h / 2) - 16;
        tinycopter.h = 16;
        tinycopter.w = 20;
        tinycopter.velocity = 0;

        draw_rect.x = 40;
        draw_rect.y = tinycopter.y;
        draw_rect.h = 16;
        draw_rect.w = 20;

        /**
         * Setup the TOP PIPE
         */
        pipe_top.x = 140;
        pipe_top.y = 0;
        pipe_top.w = 30;
        pipe_top.surface = SDL_Create32BitSurface(30, scene.h);

        if (pipe_top.surface == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);


        pipe_bottom.x = 140;
        pipe_bottom.w = 30;
        pipe_bottom.surface = SDL_Create32BitSurface(30, scene.h);

        if (pipe_bottom.surface == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        Pipe_Update(&pipe_top, &pipe_bottom, &scene);
        pipe_bottom.y = scene.h - pipe_bottom.h;


        previous = SDL_GetTicks();
        while (!quit) {
                current = SDL_GetTicks();
                elapsed = current - previous;
                previous = current;
                lag += elapsed;

                SDL_PollEvent(&e);
                if (e.type == SDL_QUIT)
                        quit = -1;
                if (e.type == SDL_KEYDOWN)
                        if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                                quit = -1;

                while (lag >= ms_per_update)
                {
                        if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LEFT) {
                                TC_Ascend(&tinycopter);
                        } else {
                                TC_Descend(&tinycopter);
                        }

                        tinycopter.y += tinycopter.velocity;

                        if (Pipe_Move(&pipe_bottom, &scene) != 0 ||
                            Pipe_Move(&pipe_top, &scene) != 0)
                            Pipe_Update(&pipe_top, &pipe_bottom, &scene);


                        lag -= ms_per_update;
                }

                interpolation = lag / ms_per_update;
                draw_rect.x = tinycopter.x;
                //float polated = (float)(draw_rect.y - tinycopter.y) * interpolation;
                draw_rect.y = tinycopter.y;

                pipe_top_rect = SDL_CreateRect(pipe_top.x, pipe_top.y, pipe_top.h, pipe_top.w);
                pipe_bottom_rect = SDL_CreateRect(pipe_bottom.x, pipe_bottom.y, pipe_bottom.h, pipe_bottom.w);

                draw_rect.h = tinycopter.h;
                draw_rect.w = tinycopter.w;

                SDL_RenderClear(scene.renderer);

                /** RENDER HERE **/
                SDL_FillRect(scene.surface, NULL,
                             SDL_MapRGBA(scene.surface->format, 255, 255, 255, 255));

                SDL_BlitSurface(pipe_top.surface, NULL, scene.surface, &pipe_top_rect);
                SDL_BlitSurface(pipe_bottom.surface, NULL, scene.surface, &pipe_bottom_rect);
                SDL_BlitSurface(tinycopter.surface, NULL, scene.surface, &draw_rect);

                SDL_UpdateTexture(scene.texture, NULL, scene.surface->pixels, scene.surface->pitch);
                SDL_RenderCopy(scene.renderer, scene.texture, NULL, NULL);
                SDL_RenderPresent(scene.renderer);

        }

        SDL_FreeSurface(pipe_top.surface);
        SDL_FreeSurface(pipe_bottom.surface);
        SDL_FreeSurface(tinycopter.surface);
        SDL_FreeSurface(scene.surface);
        SDL_DestroyTexture(scene.texture);
        SDL_DestroyRenderer(scene.renderer);
        SDL_DestroyWindow(scene.window);

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();

        return 0;
}
