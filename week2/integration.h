#pragma once

#include "session.h"

#include <cstddef>
#include <string>

using namespace std;

void loadSessionFile(const string& path, Session& session, string& name, size_t& recordCount);
