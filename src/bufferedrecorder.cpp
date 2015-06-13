
#include "bufferedrecorder.hpp"

#include <stdexcept>
#include <iostream>

BufferedRecorder::BufferedRecorder(short int capacity, size_t interval):
    capacity(capacity),
    growingbuffer{capacity},
    readingbuffer{capacity}
{
    setProcessingInterval(sf::milliseconds(interval));
}


bool BufferedRecorder::onProcessSamples(const sf::Int16* samples, std::size_t sampleCount)
{
totalsamples += sampleCount;
    sf::Lock bufferlock(buffermutex);

    for (size_t i = 0; i < sampleCount; ++i) {
        growingbuffer.push_back(samples[i]);
    }
    //std::cout << "recording " << sampleCount << " samples" << std::endl;

    while (growingbuffer.size() > (size_t)capacity) {
        growingbuffer.pop_front();
    }
    return true;
}

std::deque<sf::Int16> BufferedRecorder::get_samples() {
    sf::Lock bufferlock(buffermutex);

    std::swap(growingbuffer, readingbuffer);
    //std::cout << "giving out " << readingbuffer.size() << " samples" << std::endl;

    growingbuffer.clear();

    return readingbuffer;
}
