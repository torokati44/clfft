#include <iomanip>

#include <SDL2/SDL.h>
#include <GL/gl.h>

#include "clwrapper.hpp"
#include "clbuffer.hpp"
#include "clkernel.hpp"

CLWrapper cl;

const int window_width = 1600;
const int window_height = 900;
const int sample_rate = 44100;

CLBuffer<cl_float2> cos_sin_buffer{sample_rate};
CLKernel<cl_mem> fill_cos_sin_kernel{"fill_cos_sin"};

CLBuffer<cl_int> index_buffer{sample_rate / 2};
CLKernel<cl_mem> increment_indices_kernel{"increment_indices"};

CLBuffer<cl_float4> spectrum_buffer{sample_rate / 2};

CLKernel<cl_float, cl_float, cl_mem, cl_mem, cl_mem> add_sample_kernel{"add_sample"};


void add_sample(float s, float alpha) {
    add_sample_kernel.execute(sample_rate / 2, s, alpha, index_buffer, spectrum_buffer, cos_sin_buffer);
    increment_indices_kernel.execute(sample_rate / 2, index_buffer);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(window_width, window_height, SDL_WINDOW_OPENGL, &window, &renderer);

    glViewport(0, 0, window_width, window_height);

    glClearColor(0.2, 0.2, 0.2, 1);


    fill_cos_sin_kernel.execute(sample_rate, cos_sin_buffer);


    glEnable(GL_DEPTH_TEST);
    glClearDepth(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    bool quit = false;
    bool paused = false;

    int frame = 0;
    int samples_per_frame = 100;
    float alpha = 0.999;
    float freq = 1000;

    while (!quit) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_P:
                            paused = !paused;
                            std::cout << (paused ? "PAUSED" : "RESUMED") << std::endl;
                            break;
                        case SDL_SCANCODE_UP:
                            alpha *= 1.0001;
                            break;
                        case SDL_SCANCODE_DOWN:
                            alpha /= 1.0001;
                            break;
                        case SDL_SCANCODE_LEFT:
                            freq -= 100;
                            break;
                        case SDL_SCANCODE_RIGHT:
                            freq += 100;
                            break;
                        default:
                            break;
                    }
                    std::cout << std::setprecision(10) << "freq = " << freq << ", alpha = " << alpha << std::endl;
                    break;
            }
        }


        const Uint8 *state = SDL_GetKeyboardState(NULL);

        if (state[SDL_SCANCODE_SPACE]) {

        }

        int x, y;
        int buttonstate = SDL_GetRelativeMouseState(&x, &y);
        if (buttonstate & 1) { // left button pressed

        }

        if (buttonstate & 4) { // right button pressed
        }

        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        if (!paused) {
            // update
        }

        // render

        Uint32 ticks1 = SDL_GetTicks();

        for (int i = 0; i < samples_per_frame; ++i) {
            float sample = cosf(2.0f * 3.1415926536f * freq * (samples_per_frame * frame + i) / sample_rate);

            if (paused) {
                sample += cosf(2.0f * 3.1415926536f *  (freq-1000) * (samples_per_frame * frame + i) / sample_rate);
            }

            add_sample(sample, alpha);
        }
        std::cout << "sample " << samples_per_frame * frame << std::endl;
        Uint32 ticks2 = SDL_GetTicks();

        cl_float4 *spectrum = spectrum_buffer.map();
        glBegin(GL_LINE_STRIP);
        glColor3f(0.2, 0.4, 0.8);
        for (int i = 0; i < sample_rate/2; ++i) {
            glVertex3f(((float)i / sample_rate) * 4 - 1, spectrum[i].s[2], 0);
        }
        glEnd();

        Uint32 ticks3 = SDL_GetTicks();

        std::cout << "transform: " << (ticks2 - ticks1) << "ms, rendering: " << (ticks3 - ticks1) << "ms "
            << "(" << (samples_per_frame * 1000.0 / (ticks2 - ticks1)) << " samples/s)" << std::endl;
        spectrum_buffer.unmap();

        SDL_RenderPresent(renderer);

        ++frame;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
