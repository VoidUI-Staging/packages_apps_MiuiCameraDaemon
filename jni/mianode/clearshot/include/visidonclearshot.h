#ifndef _VISIDON_H_
#define _VISIDON_H_

namespace mialgo {

class QVisidonClearshot {
public:
    QVisidonClearshot();
    virtual ~ QVisidonClearshot();
    void init();
    void deinit(void);
    void process(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num, int iso);
    void process(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num, int iso, int *output_baseframe);

private:
    int ParseAlogLib(void);
    void *m_VDlib_handle;
};

}
#endif
