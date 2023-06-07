#ifndef __TIMEMEASURER_H__
#define __TIMEMEASURER_H__

#include <SuperSensorServer/common.h>

namespace almalence {

/**
 * Time measurement class.
 */
class TimeMeasurer
{
private:
    const int width;
    bool initialized;
    double measurementLast;
    long long measurementLastTime;

    double measurementAverage;
    double measurementLowest;
    double measurementHighest;

public:
    TimeMeasurer();
    virtual ~TimeMeasurer();

    void flush();

    void measure();
    void addMeasurement(const double measurement);
    void addMeasurement(const double last, const double average);

    void mark();

    bool isValid() const;

    double getLastMeasurement() const;

    double getAverageMeasurement() const;

    double getInstantRate() const;

    double getRate() const;

    double getRate(const double measurement) const;

    std::string getString() const;
    std::string getRateString() const;
};

} // namespace almalence

#endif //__TIMEMEASURER_H__
