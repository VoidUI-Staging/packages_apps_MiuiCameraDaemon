#include "RingQueue.h"

#include <utils/Log.h>

#include <iostream>
using namespace std;

RingQueue::RingQueue(int maxSize)
{
    m_capacity = maxSize;
    m_iHead = 0;
    m_pQueue = new miBokehScene_t[maxSize];
}

RingQueue::~RingQueue()
{
    delete[] m_pQueue;
}

void RingQueue::insert(miBokehScene_t item)
{
    m_pQueue[m_iHead] = item;
    m_iHead++;
    m_iHead %= m_capacity;
}

miBokehScene_t RingQueue::getCurItem()
{
    int curIndex = (m_iHead + m_capacity - 1) % m_capacity;
    return m_pQueue[curIndex];
}

miBokehScene_t RingQueue::getItem(int64_t timestamp)
{
    miBokehScene_t scene;
    for (int i = 0; i < m_capacity; ++i) {
        int index = (m_iHead - i + m_capacity - 1) % m_capacity;
        int64_t lastTimeStamp = m_pQueue[index].timestamp;
        if (lastTimeStamp <= timestamp || i == m_capacity - 1) {
            scene.timestamp = m_pQueue[index].timestamp;
            scene.sceneCategory = m_pQueue[index].sceneCategory;
            scene.sceneseq = m_pQueue[index].sceneseq;
            break;
        }
    }
    return scene;
}