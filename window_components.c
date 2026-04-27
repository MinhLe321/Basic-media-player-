//Basically, I just don't want people to access the struct :)
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_rect.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_stdinc.h>

#include "window_components.h"

typedef struct SDL_Application{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    TTF_Font *font;

    char window_title[255];
    int window_width;
    int window_height;
    int font_size;
} AppState;



AppState* init_window(const char *window_title, int window_width, int window_height, int font_size){
    AppState* as = SDL_calloc(1, sizeof(AppState));
    if (!as){
        return NULL;
    }

    if (SDL_utf8strlen(window_title) >= 255){
        SDL_Log("String (window's name) is too long, SDL will use default name: Basic Window.");
        SDL_strlcpy(as->window_title, "Basic Window", sizeof(as->window_title));
    }
    else{
        SDL_strlcpy(as->window_title, window_title, sizeof(as->window_title));
    }

    as->window_width = window_width;
    as->window_height = window_height;
    as->font_size = font_size;

    if (!SDL_CreateWindowAndRenderer(as->window_title, as->window_width, as->window_height, SDL_WINDOW_OPENGL, &as->window, &as->renderer)){
        SDL_Log("Couldn't create window/renderer: %s\n", SDL_GetError());
        SDL_free(as);
        return NULL;
    }

     //Initialize SDL_ttf libraries :D
    if (!TTF_Init()){
        SDL_Log("Couldn't initialize the library: %s\n", SDL_GetError());
        SDL_DestroyWindow(as->window);
        SDL_DestroyRenderer(as->renderer);
        SDL_free(as);
        return NULL;
    }

    as->font = TTF_OpenFont("Satoshi-Regular.ttf", as->font_size);
    if (!as->font){
        SDL_Log("Couldn't open the font the font: %s\n", SDL_GetError());
        SDL_DestroyWindow(as->window);
        SDL_DestroyRenderer(as->renderer);
        SDL_free(as);
        return NULL;
    }

    return as;
}

int get_window_height(AppState *as){
    return as->window_height;
}
int get_window_width(AppState *as){
    return as->window_width;
}

SDL_Window* get_window(AppState *as){
    return as->window;
}
SDL_Renderer* get_renderer(AppState *as){
    return as->renderer;
}
SDL_Texture* get_texture(AppState *as){
    return as->texture;
}
TTF_Font* get_ttf_font(AppState *as){
    return as->font;
}