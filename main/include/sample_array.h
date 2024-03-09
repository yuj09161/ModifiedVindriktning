#ifndef __ST_COMMON_REST_CLIENT_H_INCLUDED__
#define __ST_COMMON_REST_CLIENT_H_INCLUDED__

class SampleArray {
    public:
        SampleArray(int size);
        ~SampleArray();
        void setOffset(float offset);
        void writeValue(float value);
        bool isValid();
        void invalidate();
        float getAverage();
    private:
        float *arr, offset = 0.0f;
        int size, pos = 0, item_cnt = 0;
};

class IntSampleArray {
    public:
        IntSampleArray(int size);
        ~IntSampleArray();
        void setOffset(int offset);
        void writeValue(int value);
        bool isValid();
        void invalidate();
        int getAverage();
    private:
        int *arr, offset = 0;
        int size, pos = 0, item_cnt = 0;
};

#endif
