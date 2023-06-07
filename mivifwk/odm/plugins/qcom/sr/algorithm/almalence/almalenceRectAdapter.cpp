#include "almalenceRectAdapter.h"

void AlmalenceRectAdapter::set(int left, int top, int width, int height)
{
    rect.set(left, top, width, height);
}

almalence::Rect *AlmalenceRectAdapter::getRect()
{
    return &rect;
}
