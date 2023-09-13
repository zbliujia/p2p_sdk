#ifndef X_UUID_GENERATOR_H
#define X_UUID_GENERATOR_H

#include <string>
#include <uuid/uuid.h>

namespace x {

class UUIDGenerator {
public:
    static std::string get() {
        char buffer[256] = {0};
        uuid_t id;
        uuid_generate_random(id);
        uuid_unparse_lower(id, buffer);

        return std::string(buffer);
    }

};

}//namespace x{

#endif //X_UUID_GENERATOR_H
