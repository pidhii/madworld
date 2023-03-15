#include "random.hpp"


std::mt19937 mw::random_generator {std::random_device()()};

