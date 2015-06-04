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
const int max_samples = 4096;

CLBuffer<cl_float2> cos_sin_buffer{sample_rate};
CLKernel<cl_mem> fill_cos_sin_kernel{"fill_cos_sin"};

CLBuffer<cl_int> index_buffer{sample_rate / 2};
CLKernel<cl_mem> increment_indices_kernel{"increment_indices"};

CLBuffer<cl_float> samples_buffer{max_samples};
CLBuffer<cl_float4> spectrum_buffer{sample_rate / 2};

CLKernel<cl_int, cl_mem, float, cl_mem, cl_mem, cl_mem> add_samples_kernel{"add_samples"};

int audio_index = 0;

void audio_callback(void* userdata,
                    Uint8* stream,
                    int len) {

    for (int i = 0; i < len; ++i) {
        stream[i] = ((Uint8 *)userdata)[audio_index + i];
    }
    audio_index += len;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(window_width, window_height, SDL_WINDOW_OPENGL, &window, &renderer);

    SDL_AudioSpec wav_spec;
    Uint32 wav_length;
    Uint8 *wav_buffer;

    if (SDL_LoadWAV("data/sound.wav", &wav_spec, &wav_buffer, &wav_length) == NULL) {
       throw std::runtime_error("Could not open WAV file!");
    }


    SDL_AudioSpec want, have;
    SDL_AudioDeviceID audio_device_id;

    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16LSB;
    want.channels = 1;
    want.samples = 441;
    want.callback = audio_callback;
    want.userdata = wav_buffer;

    audio_device_id = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device_id == 0) {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_PauseAudioDevice(audio_device_id, 0);

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
    float alpha = 0.997;
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
        static int last_audio_index = 0;
        int x, y;
        int buttonstate = SDL_GetMouseState(&x, &y);
        if (buttonstate & 1) { // left button pressed
            last_audio_index = audio_index = (int)(wav_length * ((float)x / window_width)) & (~1);
        }

        if (buttonstate & 4) { // right button pressed
        }

        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        if (!paused) {
            // update
        }

        // render


        Uint32 ticks1 = SDL_GetTicks();

        int num_samples = (audio_index - last_audio_index) / 2;

        cl_float *samples = samples_buffer.map();
        for (int i = 0; i < num_samples; ++i) {
            Sint16 *sint_ptr = (Sint16 *)wav_buffer;

            samples[i] = sint_ptr[(audio_index / 2) + i] / 32768.0f;
        }

        samples_buffer.unmap();

        add_samples_kernel.execute(sample_rate / 2, num_samples, samples_buffer, alpha, index_buffer, spectrum_buffer, cos_sin_buffer);
        Uint32 ticks2 = SDL_GetTicks();
        last_audio_index = audio_index;

        // 0 - 100
        // 100 - 1000
        // 1000 - 1000
        // 1000 - ...
        float regions[4] = {0.0f};

        cl_float4 *spectrum = spectrum_buffer.map();
        glBegin(GL_LINE_STRIP);
        glColor3f(0.2, 0.4, 0.8);
        for (int i = 0; i < sample_rate/2; ++i) {
            if (i < 50) {
                regions[0] += spectrum[i].s[2] / 2000;
            } else if (i < 500) {
                regions[1] += spectrum[i].s[2] / 10000;
            } else if (i < 10000) {
                regions[2] += spectrum[i].s[2] / 20000;
            } else {
                regions[3] += spectrum[i].s[2] / 10000;
            }

            glVertex3f(0.15f + 10*log10f(((float)i / sample_rate) + 1) - 1, log2f(spectrum[i].s[2] + 1) * 0.1f, 0);
        }
        glEnd();

        glBegin(GL_QUADS);
        glColor3f(regions[0], regions[0], regions[0]);

        glVertex3f( -0.8, -0.8, 0);
        glVertex3f( -0.6, -0.8, 0);
        glVertex3f( -0.6, -0.6, 0);
        glVertex3f( -0.8, -0.6, 0);

        glColor3f(regions[1], regions[1], regions[1]);
        glVertex3f( -0.5, -0.8, 0);
        glVertex3f( -0.3, -0.8, 0);
        glVertex3f( -0.3, -0.6, 0);
        glVertex3f( -0.5, -0.6, 0);

        glColor3f(regions[2], regions[2], regions[2]);

        glVertex3f( -0.1, -0.8, 0);
        glVertex3f(  0.1, -0.8, 0);
        glVertex3f(  0.1, -0.6, 0);
        glVertex3f( -0.1, -0.6, 0);

        glColor3f(regions[3], regions[3], regions[3]);
        glVertex3f( 0.3, -0.8, 0);
        glVertex3f( 0.5, -0.8, 0);
        glVertex3f( 0.5, -0.6, 0);
        glVertex3f( 0.3, -0.6, 0);

        glEnd();

        //std::cout << "regions: " << regions[0] << " " << regions[1] << " " << regions[2] << " " << regions[3] << std::endl;

        Uint32 ticks3 = SDL_GetTicks();

        //std::cout << "transform: " << (ticks2 - ticks1) << "ms, rendering: " << (ticks3 - ticks2) << "ms " << std::endl;
        spectrum_buffer.unmap();

        SDL_RenderPresent(renderer);

        ++frame;
    }

    SDL_CloseAudioDevice(audio_device_id);

    SDL_FreeWAV(wav_buffer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
