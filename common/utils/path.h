#pragma once

#ifndef _GAME_COMMON_UTILS_PATH_H
#define _GAME_COMMON_UTILS_PATH_H

#include <string>

namespace gamepath
{

bool CreatePath(const std::string& sPath);

bool FileExists(const std::string& sFilepath);

bool RemoveFile(const std::string& sFilepath);

} // namespace gamepath

#endif
