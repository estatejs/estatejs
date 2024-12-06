//
// Created by scott on 5/14/20.
//

#pragma once

namespace estate {
    template<class T>
    inline T operator~(T a) { return (T) ~(int) a; }
    template<class T>
    inline T operator|(T a, T b) { return (T) ((int) a | (int) b); }
    template<class T>
    inline T operator&(T a, T b) { return (T) ((int) a & (int) b); }
    template<class T>
    inline T operator^(T a, T b) { return (T) ((int) a ^ (int) b); }
    template<class T>
    inline T &operator|=(T &a, T b) { return (T &) ((int &) a |= (int) b); }
    template<class T>
    inline T &operator&=(T &a, T b) { return (T &) ((int &) a &= (int) b); }
    template<class T>
    inline T &operator^=(T &a, T b) { return (T &) ((int &) a ^= (int) b); }
}