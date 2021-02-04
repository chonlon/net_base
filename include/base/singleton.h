#pragma once
#include "nocopyable.h"

namespace lon {
//Meyers' Singleton
template <typename T>
class Singleton : public Noncopyable
{
public:
    Singleton()  = delete;
    ~Singleton() = delete;


    static T* getInstance() {
        static T instance;
        return &instance;
    }
};

}
