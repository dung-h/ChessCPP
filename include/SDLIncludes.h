#pragma once

#ifdef USE_SDL2
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_image.h>
    
    // Event type mapping
    #define SDL_EVENT_QUIT SDL_QUIT
    #define SDL_EVENT_MOUSE_BUTTON_DOWN SDL_MOUSEBUTTONDOWN
    #define SDL_EVENT_WINDOW_RESIZABLE SDL_WINDOW_RESIZABLE
    
    // Function compatibility wrappers
    inline SDL_Window* SDL_CreateWindow(const char* title, int width, int height, Uint32 flags) {
        return SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
    }
    
    inline SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, void* unused) {
        return SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }
    
    // Rect handling for SDL2
    inline int SDL_RenderFillRectF(SDL_Renderer* renderer, const SDL_FRect* frect) {
        if (!frect) return SDL_RenderFillRect(renderer, NULL);
        SDL_Rect rect;
        rect.x = static_cast<int>(frect->x);
        rect.y = static_cast<int>(frect->y);
        rect.w = static_cast<int>(frect->w);
        rect.h = static_cast<int>(frect->h);
        return SDL_RenderFillRect(renderer, &rect);
    }
    
    inline int SDL_RenderDrawRectF(SDL_Renderer* renderer, const SDL_FRect* frect) {
        if (!frect) return SDL_RenderDrawRect(renderer, NULL);
        SDL_Rect rect;
        rect.x = static_cast<int>(frect->x);
        rect.y = static_cast<int>(frect->y);
        rect.w = static_cast<int>(frect->w);
        rect.h = static_cast<int>(frect->h);
        return SDL_RenderDrawRect(renderer, &rect);
    }
    
    inline int SDL_RenderTexture(SDL_Renderer* renderer, SDL_Texture* texture, 
                               const SDL_Rect* srcrect, const SDL_FRect* dstrect) {
        if (!dstrect) return SDL_RenderCopy(renderer, texture, srcrect, NULL);
        SDL_Rect rect;
        rect.x = static_cast<int>(dstrect->x);
        rect.y = static_cast<int>(dstrect->y);
        rect.w = static_cast<int>(dstrect->w);
        rect.h = static_cast<int>(dstrect->h);
        return SDL_RenderCopy(renderer, texture, srcrect, &rect);
    }
    
    // Mouse state handling
    inline Uint32 SDL_GetMouseState(float* x, float* y) {
        int ix, iy;
        Uint32 state = SDL_GetMouseState(&ix, &iy);
        if (x) *x = static_cast<float>(ix);
        if (y) *y = static_cast<float>(iy);
        return state;
    }
    
    // Surface functions
    inline Uint32 SDL_MapSurfaceRGB(SDL_Surface* surface, Uint8 r, Uint8 g, Uint8 b) {
        return SDL_MapRGB(surface->format, r, g, b);
    }
    
    #define SDL_SetSurfaceColorKey SDL_SetColorKey
    #define SDL_DestroySurface SDL_FreeSurface
    #define SDL_RenderRect SDL_RenderDrawRectF
    #define SDL_RenderFillRect SDL_RenderFillRectF
    #define SDL_BUTTON_LEFT 1
    
#else
    #include <SDL3/SDL.h>
    #include <SDL3_image/SDL_image.h>
#endif