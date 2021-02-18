#pragma once
#include "executor.h"
#include <queue>
namespace lon {
namespace coroutine {
class Blancer
{
public:
    using BlancerTask = Executor::ExectutorFunc;
    void push(BlancerTask task) {
        tasks_.push(task);
    }

    size_t pendingTaskCount() {
        return tasks_.size();
    }
protected:
    virtual void blanceTo() = 0;

    std::queue<BlancerTask> tasks_;
};

class PrioBlancer : public Blancer{
public:

};


}  // namespace coroutine

}  // namespace lon
