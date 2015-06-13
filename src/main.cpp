#include <iomanip>

#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <GL/gl.h>
#include <CL/cl_platform.h>

#include "clwrapper.hpp"
#include "clbuffer.hpp"
#include "clkernel.hpp"
#include "bufferedrecorder.hpp"

CLWrapper cl;

const int window_width = 1600;
const int window_height = 900;
const int sample_rate = 44100;
const int max_samples = 256;

CLBuffer<cl_float2> cos_sin_buffer{sample_rate};
CLKernel<cl_mem> fill_cos_sin_kernel{"fill_cos_sin"};

CLBuffer<cl_int> index_buffer{sample_rate / 2};

CLBuffer<cl_float> samples_buffer{max_samples};
CLBuffer<cl_float4> spectrum_buffer{sample_rate / 2};

CLKernel<cl_int, cl_mem, float, cl_mem, cl_mem, cl_mem> add_samples_kernel{"add_samples"};
CLKernel<cl_int, cl_mem, cl_mem> transform_image_kernel{"transform_image"};
CLKernel<cl_int, cl_mem, cl_mem> inverse_transform_image_kernel{"inverse_transform_image"};
CLKernel<cl_int, cl_mem> filter_bandpass_kernel{"filter_bandpass"};
CLKernel<cl_int, cl_mem, cl_mem, cl_int, cl_mem> filter_multiply_kernel{"filter_multiply"};

int main() {

    sf::Image image;
    image.loadFromFile("data/lena_128_flipped.png");
    CLBuffer<cl_float2> image_intensity_buffer(image.getSize().x * image.getSize().y);
    CLBuffer<cl_float2> image_transformed_buffer(image.getSize().x * image.getSize().y);
    CLBuffer<cl_float2> image_filtered_buffer(image.getSize().x * image.getSize().y);
    auto intensities = image_intensity_buffer.map();

    auto pixels = image.getPixelsPtr();

    for (size_t y = 0; y < image.getSize().y; ++y) {
        for (size_t x = 0; x < image.getSize().x; ++x) {
            auto intensity_index = y * image.getSize().x + x;
            auto pixels_index = intensity_index * 4;

            float value = (pixels[pixels_index] + pixels[pixels_index + 1] + pixels[pixels_index + 2]) / 3.0f;
            value /= 255.0f;
            value -= 0.5f;
            value *= 2.0f;

            intensities[intensity_index].s[0] = value;
            intensities[intensity_index].s[1] = 0.0f;
        }
    }

    image_intensity_buffer.unmap();

    transform_image_kernel.execute(image.getSize().x * image.getSize().y, image.getSize().x, image_intensity_buffer, image_transformed_buffer);
/*
    //filter_bandpass_kernel.execute(image.getSize().x * image.getSize().y, image.getSize().x, image_transformed_buffer);

    inverse_transform_image_kernel.execute(image.getSize().x * image.getSize().y, image.getSize().x, image_transformed_buffer, image_filtered_buffer);

    auto image_filtered = image_filtered_buffer.map();

    auto transformed_pixels = new sf::Uint8[4 * image.getSize().x * image.getSize().y];

    for (size_t y = 0; y < image.getSize().y; ++y) {
        for (size_t x = 0; x < image.getSize().x; ++x) {
            auto transformed_index = y * image.getSize().x + x;
            auto pixels_index = transformed_index * 4;


            //transformed_pixels[pixels_index] = transformed_pixels[pixels_index + 1] = transformed_pixels[pixels_index + 2] =
            //        (sf::Uint8)((sqrtf(image_filtered[transformed_index].s[0] * image_filtered[transformed_index].s[0] +
            //                           image_filtered[transformed_index].s[1] * image_filtered[transformed_index].s[1])) * 255.0f);

            transformed_pixels[pixels_index + 1] = transformed_pixels[pixels_index + 2] = transformed_pixels[pixels_index] = (sf::Uint8)((
            (image_filtered[transformed_index].s[0] + image_filtered[transformed_index].s[1])
            + 1.0f) * (255.0f / 2.0f));
            //transformed_pixels[pixels_index + 1] = (sf::Uint8)((image_filtered[transformed_index].s[1] + 1.0f) * (255.0f / 2.0f));

            //transformed_pixels[pixels_index + 2] = 0;
            transformed_pixels[pixels_index + 3] = 255;

        }
    }


    sf::Image transformed;
    transformed.create(image.getSize().x, image.getSize().y, transformed_pixels);
    transformed.saveToFile("transformed.png");

    image_filtered_buffer.unmap();
*/


    sf::Window window{sf::VideoMode(window_width, window_height), "lel"};

    BufferedRecorder recorder(max_samples);


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

    sf::Clock clock;
    clock.restart();
    recorder.start(sample_rate);
    while (!quit) {

        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    quit = true;
                    break;
                case sf::Event::KeyPressed:
                    switch (event.key.code) {
                        case sf::Keyboard::P:
                            paused = !paused;
                            std::cout << (paused ? "PAUSED" : "RESUMED") << std::endl;
                            break;
                        case sf::Keyboard::Up:
                            alpha *= 1.0001;
                            break;
                        case sf::Keyboard::Down:
                            alpha /= 1.0001;
                            break;
                        case sf::Keyboard::Left:
                            freq -= 100;
                            break;
                        case sf::Keyboard::Right:
                            freq += 100;
                            break;
                        default:
                            break;
                    }
                    std::cout << std::setprecision(10) << "freq = " << freq << ", alpha = " << alpha << std::endl;
                    break;
                default:
                    // ignoring any other kind of event
                    break;
            }
        }


        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        if (!paused) {
            // update


            // render

            auto samples_recorded = recorder.get_samples();

            auto samples = samples_buffer.map();

            for (size_t i = 0; i < samples_recorded.size(); ++i) {
                samples[i] = samples_recorded[i] / 32768.0f;
            }

            samples_buffer.unmap();
            add_samples_kernel.execute(sample_rate / 2, samples_recorded.size(), samples_buffer, alpha, index_buffer,
                                       spectrum_buffer, cos_sin_buffer);






            filter_multiply_kernel.execute(image.getSize().x * image.getSize().y, image.getSize().x, image_transformed_buffer, image_filtered_buffer, sample_rate / 2, spectrum_buffer);

            inverse_transform_image_kernel.execute(image.getSize().x * image.getSize().y, image.getSize().x, image_filtered_buffer, image_intensity_buffer);

            auto image_filtered = image_intensity_buffer.map();

            auto transformed_pixels = new sf::Uint8[4 * image.getSize().x * image.getSize().y];

            for (size_t y = 0; y < image.getSize().y; ++y) {
                for (size_t x = 0; x < image.getSize().x; ++x) {
                    auto transformed_index = y * image.getSize().x + x;
                    auto pixels_index = transformed_index * 4;

                    /*
                    transformed_pixels[pixels_index] = transformed_pixels[pixels_index + 1] = transformed_pixels[pixels_index + 2] =
                            (sf::Uint8)((sqrtf(image_filtered[transformed_index].s[0] * image_filtered[transformed_index].s[0] +
                                               image_filtered[transformed_index].s[1] * image_filtered[transformed_index].s[1])) * 255.0f);
        */

                    float value = std::max(std::min((image_filtered[transformed_index].s[0] + image_filtered[transformed_index].s[1] + 1) / 2.0f, 1.0f), 0.0f);

                    transformed_pixels[pixels_index + 1] = transformed_pixels[pixels_index + 2] = transformed_pixels[pixels_index] =
                            (sf::Uint8)(value * 255.0f);
                    //transformed_pixels[pixels_index + 1] = (sf::Uint8)((image_filtered[transformed_index].s[1] + 1.0f) * (255.0f / 2.0f));

                    //transformed_pixels[pixels_index + 2] = 0;
                    transformed_pixels[pixels_index + 3] = 255;

                }
            }


            //sf::Image transformed;
            //transformed.create(image.getSize().x, image.getSize().y, transformed_pixels);
            //transformed.saveToFile("transformed.png");
            glDrawPixels(image.getSize().x, image.getSize().y, GL_RGBA, GL_UNSIGNED_BYTE, transformed_pixels);

            image_intensity_buffer.unmap();
        }


        // 0 - 100
        // 100 - 1000
        // 1000 - 1000
        // 1000 - ...
        /*
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

        glVertex3f( -0.8f, -0.8f, 0);
        glVertex3f( -0.6f, -0.8f, 0);
        glVertex3f( -0.6f, -0.6f, 0);
        glVertex3f( -0.8f, -0.6f, 0);

        glColor3f(regions[1], regions[1], regions[1]);
        glVertex3f( -0.5f, -0.8f, 0);
        glVertex3f( -0.3f, -0.8f, 0);
        glVertex3f( -0.3f, -0.6f, 0);
        glVertex3f( -0.5f, -0.6f, 0);

        glColor3f(regions[2], regions[2], regions[2]);

        glVertex3f( -0.1f, -0.8f, 0);
        glVertex3f(  0.1f, -0.8f, 0);
        glVertex3f(  0.1f, -0.6f, 0);
        glVertex3f( -0.1f, -0.6f, 0);

        glColor3f(regions[3], regions[3], regions[3]);
        glVertex3f( 0.3f, -0.8f, 0);
        glVertex3f( 0.5f, -0.8f, 0);
        glVertex3f( 0.5f, -0.6f, 0);
        glVertex3f( 0.3f, -0.6f, 0);

        glEnd();

        //std::cout << "regions: " << regions[0] << " " << regions[1] << " " << regions[2] << " " << regions[3] << std::endl;



        //std::cout << "transform: " << (ticks2 - ticks1) << "ms, rendering: " << (ticks3 - ticks2) << "ms " << std::endl;
        spectrum_buffer.unmap();
*/
        window.display();

        ++frame;
    }

    recorder.stop();
    window.close();

    return EXIT_SUCCESS;
}
