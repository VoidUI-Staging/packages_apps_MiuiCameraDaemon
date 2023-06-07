#ifndef __SUPERPARCEL_H__
#define __SUPERPARCEL_H__

#include <SuperSensorServer/common.h>

namespace almalence {

/**
 * Base type for all service data exchange types.
 * Responsible for time measurements and error delivery.
 */
struct SuperParcel
{
private:
    unsigned long long timeLongest;
    unsigned long long timePush;
    int error = 0;
    char errorMessage[256];

public:
    unsigned long long timestamp;

    void push(const SuperParcel &in)
    {
        this->timeLongest = in.timeLongest;
        this->timePush = currentTimeInMilliseconds();
        if (in.error) {
            this->setError(in.errorMessage);
        }
    }

    void pop()
    {
        this->timeLongest =
            std::max(currentTimeInMilliseconds() - this->timePush, this->timeLongest);
    }

    void propagate(const SuperParcel &in)
    {
        this->timeLongest = in.timeLongest;
        this->timestamp = in.timestamp;
    }

    long long getBottleneckTime() const { return this->timeLongest; }

    void setError(const char *errorMessage)
    {
        if (this->error == 0) {
            this->error = 1;
            strcpy(this->errorMessage, errorMessage);
        }
    }

    int getError() { return this->error; }

    char *getErrorMessage() { return this->errorMessage; }
};

}; // namespace almalence

#endif // __SUPERPARCEL_H__
