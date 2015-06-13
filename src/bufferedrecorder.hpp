#ifndef BUFFEREDRECORDER_HPP_INCLUDED
#define BUFFEREDRECORDER_HPP_INCLUDED

#include <SFML/Audio/SoundRecorder.hpp>
#include <SFML/System/Mutex.hpp>
#include <SFML/System/Lock.hpp>

#include <deque>

class BufferedRecorder : public sf::SoundRecorder {
    sf::Mutex buffermutex;
    short int capacity;

    std::deque<sf::Int16> growingbuffer;
    std::deque<sf::Int16> readingbuffer;

    virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount) override;

public:
int totalsamples = 0;
    BufferedRecorder(short int capacity, size_t interval = 0);

    std::deque<sf::Int16> get_samples();
};


#endif //BUFFEREDRECORDER_HPP_INCLUDED
