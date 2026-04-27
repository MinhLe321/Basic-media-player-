#define SDL_MAIN_USE_CALLBACKS 1 //Use call-back instead of main
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_rect.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_stdinc.h>

// #include "screencaps_utils.h"
#include "window_components.h"

#define RED_FOR_WHITE 255
#define GREEN_FOR_WHITE 255
#define BLUE_FOR_WHITE 255
#define SDL_ALPHA_OPAQUE 255

#define MAX_TEXT_INPUT_AMOUNT 20 //Large amount for the relative patth to the file

static SDL_FRect f_rect;
static SDL_Rect rect;

static char input_buffer[MAX_TEXT_INPUT_AMOUNT];
static bool accepting_input = false;

char *append_two_string(char *string1, const char *string2, unsigned int n){
    if ((string1 == NULL) && (string2 == NULL)){
        return NULL;
    }

    /*Make a copy of string1*/
    char *destination = string1;

    /*Find the end of the copy string of string 1*/
    while (*destination != '\0'){
        destination++;
    }

    /*Append the source string characters 
    until the null character of string 2 or hit the amount of string limit to copy of n*/
    while (n--){
        if (!(*destination++ = *string2++)){
            return string1;
        }
    }
    /*append the null terminator in the end of the string*/
    *destination = '\0';

    return string1;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]){ //THis function will initialize window so we can use :D
    AppState *as = init_window("Basic video player", 800, 600, 18);
    SDL_Window *window = get_window(as);
    SDL_Renderer *renderer = get_renderer(as);

    if (!as){
        return SDL_APP_FAILURE;
    }
    else{
        *appstate = as;
    }

    int window_width = get_window_width(as);
    int window_height = get_window_height(as);

    /*Initialize the floating point rectangle's position that will always appear at the start of the program*/
    f_rect.x = window_width/4;
    f_rect.w = (window_width/4)*2;
    f_rect.h =  50;
    f_rect.y = (window_height/2) - f_rect.h;

    /*Initialize the integer rectangle that has the same coordiante as the rect above to use it as an input space for textr*/
    rect.x = rect.y = f_rect.x;
    rect.w = f_rect.w;
    rect.h = f_rect.h;

    int cursor_position = f_rect.x + 50; /*the offset of the current cursor location relative to f_rect->x, in window coordinates.*/
    if (!SDL_SetTextInputArea(window, &rect, cursor_position)){
        SDL_Log("Failed to set text input area: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    AppState *as = appstate;
    SDL_Window *window = get_window(as);
    SDL_Renderer *renderer = get_renderer(as);

    switch (event->type){
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:{ //Trigger when we click the left mouse to enable or disable text input event
            if (event->motion.x > f_rect.x && event->motion.x < f_rect.x + f_rect.w){
                if (event->motion.y > f_rect.y && event->motion.y < f_rect.y + f_rect.h){
                    if (!accepting_input){ //If we are not accepting inputt, the will open to accept input
                        if (!SDL_StartTextInput(window)){
                            SDL_Log("Couldn't accepting Unicode text input events in this window: %s\n", SDL_GetError());
                            accepting_input = false;
                        }
                        
                        else {
                            accepting_input = true;
                            printf("Accepting text input\n");
                        }
                    }
                    else {
                        SDL_StopTextInput(window);
                        accepting_input = false;
                        printf("Stopped accepting text input\n");
                        printf("Input: %s\n", input_buffer);
                    }
                }
            }
            break;
        }
        case SDL_EVENT_KEY_DOWN:{ //This will trigger every key on the keyboard, included control keys like shift, backspace, enter, etc
            if (accepting_input){
                SDL_Keycode sym = event->key.key;
                size_t input_length = strlen(input_buffer);

                if (sym == SDLK_BACKSPACE && input_length > 0){
                    input_buffer[strlen(input_buffer) - 1] = '\0'; //Remove the last word
                }
                else if (sym == SDLK_RETURN){
                    SDL_StopTextInput(window);
                    accepting_input = false;
                    printf("Stopped accepting text input\n");
                    printf("Input: %s\n", input_buffer);
                }

            }
            break;
        }
        case SDL_EVENT_TEXT_INPUT:{ //This event only take text input based key like characters and not control keys like space, enter or backspace
            if (accepting_input){
                size_t input_length = strlen(input_buffer);
                if (input_length < MAX_TEXT_INPUT_AMOUNT -1){
                    append_two_string(input_buffer, &event->text.text[0], 1);
                }
                else {
                    SDL_Log("Max input buffer, please delete character.\n");
                }
                SDL_Log("%c ", event->text.text[0]);
            }
            break;
        }

    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate){
    AppState *as = appstate;
    SDL_Window *window = get_window(as);
    SDL_Renderer *renderer = get_renderer(as);
    /*Window color*/
    SDL_SetRenderDrawColor(renderer, RED_FOR_WHITE, GREEN_FOR_WHITE, BLUE_FOR_WHITE, SDL_ALPHA_OPAQUE);

    //Clear the window to draw the color, start with a blank canvas
    SDL_RenderClear(renderer);

    /*Draw a filled rectangle in the middle*/
    SDL_SetRenderDrawColor(renderer, 249, 224, 195, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &f_rect);

    //Put the newly-created rendering on the screen
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result){
    AppState *as = appstate;
    TTF_Font *font = get_ttf_font(as);
    //Will help freeing all the memory after quitting the program
    if (font){
        TTF_CloseFont(font);
        font = NULL;
    }
    TTF_Quit();

    SDL_free(appstate); // just free the memory, SDL will clean up the window/renderer for us.
}
