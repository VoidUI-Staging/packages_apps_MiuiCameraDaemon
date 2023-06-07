#include <sys/types.h>
struct miBokehScene_t
{
    miBokehScene_t() : sceneCategory(0), sceneseq(0) {}
    int64_t timestamp;
    int sceneCategory;
    int sceneseq;
};

class RingQueue
{
public:
    RingQueue(int maxSize);
    ~RingQueue();
    void insert(miBokehScene_t item);
    miBokehScene_t getCurItem();
    miBokehScene_t getItem(int64_t timestamp);

private:
    miBokehScene_t *m_pQueue; //数组指针
    int m_capacity;           //最大长度
    int m_iHead;              //队头
};