#ifndef Awideview_H
#define Awideview_H

#include <cstring>
#include <sstream>
#include <chrono>
#include <cmath>
#include <thread>
#include <algorithm>
#include "awideviewprocess.h"
#include "awideviewvideo.h"
//#include "version.hpp"
//#include "cxxopts.hpp"

using namespace DerperView;

//cxxopts::Options options("derperview", "creates derperview (like superview) from 4:3 videos");


using namespace std;

int Go(const string inputFilename, const string outputFilename, const int totalThreads);

#endif // Awideview_H
