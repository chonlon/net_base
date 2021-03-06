#include "base/lstring.h"

namespace lon {

std::vector<StringPiece> splitStringPiece(StringPiece str_v,
                                          StringPiece split_world) {
    size_t index            = 0;
    size_t split_world_size = split_world.size();
    size_t str_size         = str_v.size();
    if (split_world_size == 0)
        return {};
    std::vector<StringPiece> result;

    while (index < str_size) {
        if (auto current = str_v.find(split_world, index);
            current != std::string::npos) {
            result.emplace_back(str_v.data() + index,
                                current - index - split_world_size + 1);
            index = current + 1;
        } else {
            result.emplace_back(str_v.data() + index, str_v.size() - index);
            break;
        }
    }
    return result;
}
}
