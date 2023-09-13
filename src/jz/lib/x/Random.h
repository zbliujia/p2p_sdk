#ifndef X_RANDOM_H_
#define X_RANDOM_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

namespace x {

class Random {
public:
    static uint32_t next() {
        srand(time(NULL));
        return rand();
    }

    static uint32_t next(uint32_t mod) {
        srand(time(NULL));
        uint32_t r = rand();
        return (mod > 0) ? (r % mod) : r;
    }

};

}//namespace x{

#endif //X_RANDOM_H_
