#include <cstdlib>
#include <climits>
#include <cfloat>

#include "esp_log.h"

#include "config.h"
#include "sample_array.h"


// SampleArray
SampleArray::SampleArray(int size) {
    this->size = size;
    this->arr = (float *)malloc(sizeof(float) * size);
    if (arr == NULL) {
        ESP_LOGE("SampleArray", "Insufficient memory to create internal array. Rebooting...");
        abort();
    }
}

SampleArray::~SampleArray() {
    free(arr);
}

void SampleArray::setOffset(float offset) {
    this->offset = offset;
};

void SampleArray::writeValue(float value) {
    this->arr[this->pos++] = value;
    this->pos %= size;
    if (item_cnt < size)
        item_cnt++; 
}

bool SampleArray::isValid() {
    return item_cnt == size;
}

void SampleArray::invalidate() {
    item_cnt = 0;
}

float SampleArray::getAverage() {
    if (!isValid())
        return FLT_MIN;

    float result = arr[0];
    for (int i = 1; i < size; i++) {
        result += this->arr[i];
    }
    return result / size + offset;
}
// End SampleArray

// IntSampleArray
IntSampleArray::IntSampleArray(int size) {
    this->size = size;
    this->arr = (int *)malloc(sizeof(int) * size);
    if (arr == NULL) {
        ESP_LOGE("IntSampleArray", "Insufficient memory to create internal array. Rebooting...");
        abort();
    }
}

IntSampleArray::~IntSampleArray() {
    free(arr);
}

void IntSampleArray::setOffset(int offset) {
    this->offset = offset;
};

void IntSampleArray::writeValue(int value) {
    this->arr[this->pos++] = value;
    this->pos %= size;
    if (item_cnt < size)
        item_cnt++; 
}

bool IntSampleArray::isValid() {
    return item_cnt == size;
}

void IntSampleArray::invalidate() {
    item_cnt = 0;
}

int IntSampleArray::getAverage() {
    if (!isValid())
        return INT_MIN;

    long long sum = arr[0];
    for (int i = 1; i < size; i++) {
        sum += this->arr[i];
    }
    return (int)((float)sum / size + 0.5f) + offset;
}
// End IntSampleArray
