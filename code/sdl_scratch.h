#if !defined(SDL_SCRATCH_H)

    struct sdl_offscreen_buffer
    {
        // NOTE: Pixels are 32-bits wide, Memory order BB GG RR XX
        SDL_Texture *Texture;
        void *Memory;
        int Width;
        int Height;
        int Pitch; //TODO(Alex): figure out why we use Pitch here
        int BytesPerPixel;
    };
    
    struct sdl_window_dimension
    {
        int Width;
        int Height;
    }

    #define SDL_SCRATCH_H

#endif
