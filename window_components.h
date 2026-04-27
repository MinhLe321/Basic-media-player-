#ifndef WINDOW_COMPONENTS_H
#define WINDOW_COMPONENTS_H

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct TTF_Font TTF_Font;

typedef struct SDL_Application AppState;

AppState* init_window(
    const char *window_title, int window_width,
    int window_height, int font_size
);

int get_window_height(AppState *as);
int get_window_width(AppState *as);

SDL_Window* get_window(AppState *as);
SDL_Renderer* get_renderer(AppState *as);
SDL_Texture* get_texture(AppState *as);
TTF_Font* get_ttf_font(AppState *as);

#endif