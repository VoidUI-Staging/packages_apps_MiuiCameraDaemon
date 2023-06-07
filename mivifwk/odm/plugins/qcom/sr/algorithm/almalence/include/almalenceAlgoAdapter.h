#ifndef __ALMALENCEADPATER_H__
#define __ALMALENCEADPATER_H__

class AlmalenceAdapter : public AlgorithmAdapter
{
public:
    AlmalenceAdapter();
    //    mia_node_type_t getType();
    const Updater **getUpdater();
    unsigned int getUpdaterCount();
    int process(const struct MiImageBuffer *in, struct MiImageBuffer *out, uint32_t input_num);
    int getDumpMsg(string &dumpMSG);

private:
    static const char *const name;
};

#endif
