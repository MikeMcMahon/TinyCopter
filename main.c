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
const float DOWN_VELOCITY = 0.05;
const float UP_VELOCITY = 0.06;
const float PIPE_ACCELERATION = 0.06;

#ifdef DEBUG
const char* GAME_FONT_FILE =
        "E:\\Development\\dependencies\\fonts\\PressStart2P-Regular.ttf";
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
        float x;
        int y;
        int h;
        int w;
        float velocity;
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
 * Initializes the pipe with some defaults
 */
void Pipe_Init(struct Pipe *pipe, struct Scene *scene)
{
        pipe->x = scene->w;
        pipe->y = 0;
        pipe->w = 24;
        pipe->h = 100;
        pipe->velocity = 2;
}

/**
 * translates the pipes along the page to the left
 * returns non-zero if wrapped the screen
 */
int Pipe_Move(struct Pipe *pipe, struct Scene *scene)
{
        pipe->x -= pipe->velocity;
        if (pipe->x + pipe->w < 0) {
                pipe->x = scene->w;
                return 1;
        }
        return 0;
}

/**
 * Renders the pipe with interpolation, returns the drawn position
 */
SDL_Rect Pipe_Render(float p, struct Pipe *pipe, SDL_Surface *surface)
{
        SDL_Rect pipe_rect = SDL_CreateRect(pipe->x, pipe->y,
                                            pipe->h, pipe->w);

        //pipe_rect.x += (pipe->x - old_x) * p;

        SDL_FillRect(surface, &pipe_rect,
                     SDL_MapRGBA(surface->format, 0, 255, 0, 255));

        return pipe_rect;
}

/**
 * Generates a new set of top/bottom pipes when they translate off the page
 */
void Pipe_Generate(struct Pipe *top, struct Pipe *bottom, struct Scene *scene)
{
        int gap = (rand() % 30) + 30;
        int offset = (float) (scene->h - gap) * ((float)(rand() % 100) / 100.0);
        printf("GAP: %d OFFSET: %d\n", gap, offset);
        top->h = offset;
        bottom->y = gap + offset;
        bottom->h = scene->h - (gap + offset);
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

void TC_Init(struct TinyCopter *tc, struct Scene *scene)
{
        tc->x = 40;
        tc->y = (scene->h / 2) - 16;
        tc->h = 16;
        tc->w = 20;
        tc->velocity = 0;
}

int Pipe_Collision(struct Pipe *top, struct Pipe *bottom, struct TinyCopter *tc)
{
        SDL_Rect tc_hitbox;
        SDL_Rect pipe_hitbox;

        tc_hitbox.x = tc->x;
        tc_hitbox.y = tc->y;
        tc_hitbox.h = tc->h;
        tc_hitbox.w = tc->w;

        pipe_hitbox.x = top->x,
        pipe_hitbox.y = top->y;
        pipe_hitbox.h = top->h;
        pipe_hitbox.w = top->w;
        if (SDL_HasIntersection(&tc_hitbox, &pipe_hitbox) == SDL_TRUE)
                return 1;

        pipe_hitbox.x = bottom->x,
        pipe_hitbox.y = bottom->y;
        pipe_hitbox.h = bottom->h;
        pipe_hitbox.w = bottom->w;

        if (SDL_HasIntersection(&tc_hitbox, &pipe_hitbox) == SDL_TRUE)
                return 1;
        return 0;
}

int main(int argc, char* argv[])
{
        int quit = 0;
        SDL_Event e;
        struct Scene scene;
        struct TinyCopter tinycopter;
        struct Pipe pipe_top;
        struct Pipe pipe_bottom;
        unsigned int previous;
        unsigned int current = 0;
        float lag = 0;
        unsigned int elapsed = 0;
        float frames_per_second = 60;
        float ms_per_update = (1000 / frames_per_second);
        int do_generate;
        SDL_Rect draw_rect;
        TTF_Font *game_font;

        // Score handling
        int score = 0;
        char score_display[4];
        SDL_Color score_color;
        SDL_Surface *score_surf;

        srand(time(NULL));

        if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
                return (DEBUG_MSG(SDL_GetError()) -1);

        if (TTF_Init() != 0)
                return (DEBUG_MSG(TTF_GetError()), -1);

        game_font = TTF_OpenFont(GAME_FONT_FILE, 16);

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

        TC_Init(&tinycopter, &scene);


        /**
         * Setup the TOP PIPE
         */
        Pipe_Init(&pipe_top, &scene);
        Pipe_Init(&pipe_bottom, &scene);
        Pipe_Generate(&pipe_top, &pipe_bottom, &scene);
        pipe_bottom.y = scene.h - pipe_bottom.h;

        // setup the score
        score_color.a = 255;
        score_color.r = 0;
        score_color.g = 0;
        score_color.b = 0;
        sprintf(score_display, "%d", score);
        score_surf = TTF_RenderText_Blended(game_font, score_display,
                               score_color);

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


                //pipe_old_x = pipe_top.x;
                while (lag >= ms_per_update)
                {
                        if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LEFT) {
                                TC_Ascend(&tinycopter);
                        } else {
                                TC_Descend(&tinycopter);
                        }

                        tinycopter.y += tinycopter.velocity;

                        do_generate = Pipe_Move(&pipe_top, &scene);
                        Pipe_Move(&pipe_bottom, &scene);
                        if (do_generate) {
                                Pipe_Generate(&pipe_top, &pipe_bottom, &scene);
                                pipe_top.velocity += PIPE_ACCELERATION;
                                pipe_bottom.velocity += PIPE_ACCELERATION;
                                score++;
                                sprintf(score_display, "%d", score);
                                score_surf = TTF_RenderText_Blended(
                                                        game_font,
                                                        score_display,
                                                        score_color);
                        }


                        if (Pipe_Collision(&pipe_top, &pipe_bottom, &tinycopter) != 0 ||
                            tinycopter.y < 0 ||
                            tinycopter.y > scene.h) {
                                // reset score counter
                                TC_Init(&tinycopter, &scene);
                                score = 0;
                                Pipe_Init(&pipe_top, &scene);
                                Pipe_Init(&pipe_bottom, &scene);

                                sprintf(score_display, "%d", score);
                                score_surf = TTF_RenderText_Blended(
                                                        game_font,
                                                        score_display,
                                                        score_color);
                        }

                        lag -= ms_per_update;
                }

                SDL_RenderClear(scene.renderer);
                draw_rect = SDL_CreateRect(
                                   tinycopter.x,
                                   tinycopter.y,
                                   tinycopter.h,
                                   tinycopter.w);

                /** RENDER HERE **/
                SDL_FillRect(scene.surface, NULL,
                             SDL_MapRGBA(scene.surface->format, 255, 255, 255, 255));

                Pipe_Render(0, &pipe_top, scene.surface);
                Pipe_Render(0, &pipe_bottom, scene.surface);

                SDL_BlitSurface(tinycopter.surface, NULL, scene.surface, &draw_rect);

                draw_rect.x = (scene.w / 2) - (score_surf->w / 2);
                draw_rect.y = 5;
                draw_rect.h = score_surf->h;
                draw_rect.w = score_surf->w;
                SDL_BlitScaled(score_surf, NULL, scene.surface, &draw_rect);
                SDL_UpdateTexture(scene.texture, NULL, scene.surface->pixels, scene.surface->pitch);
                SDL_RenderCopy(scene.renderer, scene.texture, NULL, NULL);
                SDL_RenderPresent(scene.renderer);

        }

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
