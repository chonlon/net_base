#include "io/hook.h"

namespace lon{
    struct GlobalIniter {
        GlobalIniter() {
            void hook_init();
        }
    };

    static GlobalIniter initer;
}
