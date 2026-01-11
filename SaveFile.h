#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include "Vector.h"

using namespace MATH;


inline void SavePositionBinary(const Vec3& pos, const std::string& filename)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return;
    file.write(reinterpret_cast<const char*>(&pos), sizeof(Vec3));
}

inline bool LoadPositionBinary(Vec3& pos, const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    file.read(reinterpret_cast<char*>(&pos), sizeof(Vec3));
    return true;
}