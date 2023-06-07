#ifndef __ALGORITHM_ADAPTER_H__
#define __ALGORITHM_ADAPTER_H__

#include "request.h"
#include "sessionInfo.h"

class AlgorithmAdapter
{
public:
    AlgorithmAdapter(const char *name) : name(name) {}

    /*
     * get algo name
     * to confirm the algo
     * to select algo by name
     * to show the algo name
     */
    const char *getName() const { return name; }

    /*
     * process SR
     */
    virtual void process(const SessionInfo *, const Request) = 0;

    virtual ~AlgorithmAdapter() {}

private:
    /*
     * algorithm name
     */
    const char *const name;
};

/*
 * For algorithm adapter implementations that are unlikely to have multiple instances,
 * there is no need to write a new class that implements the process function.
 * Just write the global handler function and give the function pointer to the instance of this
 * class.
 */
class SingletonAlgorithmAdapter : public AlgorithmAdapter
{
public:
    /*
     * Constructors
     * @param algorithm name
     * @param function pointer which can process sr
     * @param function pointer which init the process
     * @param function pointer which deinit the process
     */
    SingletonAlgorithmAdapter(const char *name,
                              void (*process)(const AlgorithmAdapter *, const SessionInfo *,
                                              const Request *),
                              void (*init)(const AlgorithmAdapter *) = nullptr,
                              void (*deinit)(const AlgorithmAdapter *) = nullptr)
        : AlgorithmAdapter(name), mProcess(process), deinit(deinit)
    {
        if (init)
            init(this);
    }

    void process(const SessionInfo *sessionInfo, const Request request)
    {
        if (mProcess)
            mProcess(this, sessionInfo, &request);
    }

    ~SingletonAlgorithmAdapter()
    {
        if (deinit)
            deinit(this);
    }

private:
    void (*const mProcess)(const AlgorithmAdapter *, const SessionInfo *, const Request *);
    void (*const deinit)(const AlgorithmAdapter *);
};

typedef AlgorithmAdapter *const Entrance;

#endif
