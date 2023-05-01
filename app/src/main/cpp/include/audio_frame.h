//
// Created by 裴俊博 on 2023/4/28.
//

#ifndef PLAYER_AUDIO_FRAME_H
#define PLAYER_AUDIO_FRAME_H

class AudioFrame {
public:
    AudioFrame(uint8_t * data, int dataSize, bool hardCopy = true) {
        this->dataSize = dataSize;
        this->data = data;
        this->hardCopy = hardCopy;
        if(hardCopy) {
            this->data = static_cast<uint8_t *>(malloc(this->dataSize));
            memcpy(this->data, data, dataSize);
        }
    }

    ~AudioFrame() {
        if(hardCopy && this->data)
            free(this->data);
        this->data = nullptr;
    }

    uint8_t * data = nullptr;
    int dataSize = 0;
    bool hardCopy = true;
};
#endif //PLAYER_AUDIO_FRAME_H
