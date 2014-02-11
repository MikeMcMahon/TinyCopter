#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_image.h"

#ifdef DEBUG
#define DEBUG_MSG(X) printf("Occurred: %s, at line %d in %s\n", X, __LINE__, __FILE__)
#else
#define DEBUG_MSG(X) printf("")
#endif

const int window_width = 250;
const int window_height = 140;

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
        int y;
        int h;
        int w;
        int can_go_up;
        int just_went_up;

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

int main(int argc, char* argv[])
{
        int quit = 0;
        SDL_Event e;
        struct Scene *scene = malloc(sizeof(struct Scene));
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
        SDL_Rect pipe_rect;
        SDL_Rect pipe_bottom_rect;

        srand(time(NULL));

        if (scene == NULL)
                return (DEBUG_MSG("Failed to create scene"), -1);

        if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
                return (DEBUG_MSG(SDL_GetError()) -1);

        scene->w = window_width;
        scene->h = window_height;

        scene->window = SDL_CreateWindow(
                 "TinyCopter",
                 SDL_WINDOWPOS_CENTERED,
                 SDL_WINDOWPOS_CENTERED,
                 scene->w,
                 scene->h,
                 SDL_WINDOW_SHOWN);
        if (scene->window == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        scene->renderer = SDL_CreateRenderer(scene->window, -1, SDL_RENDERER_ACCELERATED);
        if (scene->window == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        scene->surface = SDL_GetWindowSurface(scene->window);
        if (scene->surface == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        scene->texture = SDL_CreateTextureFromSurface(scene->renderer, scene->surface);
        if (scene->texture == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        IMG_Init(IMG_INIT_PNG);
        tinycopter.surface = IMG_Load("tinycopter.png");
        if (tinycopter.surface == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        /*
         * Configuring the TinyCopter..
         */
        tinycopter.x = 40;
        tinycopter.y = (scene->h / 2) - 16;
        tinycopter.h = 16;
        tinycopter.w = 20;
        tinycopter.can_go_up = 1;

        draw_rect.x = 40;
        draw_rect.y = tinycopter.y;
        draw_rect.h = 16;
        draw_rect.w = 20;

        // setup the pipes
        pipe_top.w = 30;
        pipe_top.x = 30; // scene->w + pipe_top.w;
        pipe_top.y = 0;
        pipe_top.h = (rand() % (scene->h - 30)) + 1;
        pipe_rect.h = pipe_top.h;
        pipe_rect.w = 18;
        pipe_rect.x = 6;
        pipe_rect.y = 0;
        pipe_top.surface = SDL_CreateRGBSurface(0, pipe_top.w, pipe_top.h, 32,
                             0xFF000000,
                             0x00FF0000,
                             0x0000FF00,
                             0x000000FF);
        if (pipe_top.surface == NULL)
                return (DEBUG_MSG(SDL_GetError()), -1);

        SDL_FillRect(pipe_top.surface, &pipe_rect, 0x00FF00FF);
        pipe_rect.w = 30;
        pipe_rect.h = 30;
        pipe_rect.y = pipe_top.h - 16;
        pipe_rect.x = 0;
        SDL_FillRect(pipe_top.surface, &pipe_rect, 0x00FF00FF);

        pipe_bottom = pipe_top;

        pipe_top_rect = SDL_CreateRect(pipe_top.x, pipe_top.y, pipe_top.h, pipe_top.w);
        pipe_bottom_rect = SDL_CreateRect(pipe_bottom.x, pipe_bottom.y, pipe_bottom.h, pipe_bottom.w);

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
                                if (tinycopter.can_go_up == 1) {
                                        tinycopter.y -= 40;
                                        tinycopter.can_go_up = 0;
                                        tinycopter.just_went_up = 1;
                                }
                        }

                        if (!(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LEFT)) {
                                tinycopter.can_go_up = 1;
                        }

                        if (tinycopter.just_went_up != 1)
                                tinycopter.y++;
                        else
                                tinycopter.just_went_up = 0;


                        lag -= ms_per_update;
                }

                interpolation = lag / ms_per_update;
                draw_rect.x = tinycopter.x;
                float polated = (float)(draw_rect.y - tinycopter.y) * interpolation;
                draw_rect.y -= polated;

                draw_rect.h = tinycopter.h;
                draw_rect.w = tinycopter.w;

                SDL_RenderClear(scene->renderer);

                /** RENDER HERE **/
                SDL_FillRect(scene->surface, NULL, 0xFFFFFFFF);
                SDL_BlitSurface(tinycopter.surface, NULL, scene->surface, &draw_rect);

                SDL_BlitSurface(pipe_top.surface, NULL, scene->surface, &pipe_top_rect);

                SDL_BlitSurface(pipe_bottom.surface, NULL, scene->surface, &pipe_bottom_rect);


                SDL_UpdateTexture(scene->texture, NULL, scene->surface->pixels, scene->surface->pitch);
                SDL_RenderCopy(scene->renderer, scene->texture, NULL, NULL);/**/
                SDL_RenderPresent(scene->renderer);

        }

        SDL_FreeSurface(tinycopter.surface);
        SDL_FreeSurface(scene->surface);
        SDL_DestroyTexture(scene->texture);
        SDL_DestroyRenderer(scene->renderer);
        SDL_DestroyWindow(scene->window);
        IMG_Quit();
        SDL_Quit();

        return 0;
}
