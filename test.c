#include "miniaudio.h"
#include "miniaudio.c"

int main(void) {
    ma_result result;
    ma_engine engine;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        printf("Error");
    }
}