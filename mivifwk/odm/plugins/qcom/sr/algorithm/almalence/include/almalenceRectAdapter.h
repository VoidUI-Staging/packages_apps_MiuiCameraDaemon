#ifndef __ALMALENCERECTADAPTER_H__
#define __ALMALENCERECTADAPTER_H__

class AlmalenceRectAdapter : public AlgorithmAdapter::AlgoRectAdapter
{
public:
    void set(int left, int top, int width, int height);
    almalence::Rect *getRect();

private:
    almalence::Rect rect;
};

#endif
