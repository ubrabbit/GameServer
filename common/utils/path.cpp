#include "path.h"
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>


namespace gamepath
{

bool CreatePath(const std::string& sPath)
{
    struct stat statbuf;
    if (stat(sPath.c_str(), &statbuf) == 0)
    {
        return S_ISDIR(statbuf.st_mode) != 0 ? true : false;
    }

    if (mkdir(sPath.c_str(), 0775) == 0)
    {
        return true;
    }

    std::size_t index = sPath.find('/');
    while (index != std::string::npos)
    {
        if (mkdir(sPath.substr(0, index + 1).c_str(), 0775) != 0 && EEXIST != errno)
        {
            return false;
        }
        index = sPath.find('/', index + 1);
    }
    if (mkdir(sPath.c_str(), 0775) == 0)
    {
        return true;
    }
    return true;
}


bool FileExists(const std::string& sFilepath)
{
    struct stat st;
    return stat(sFilepath.c_str(), &st) == 0;
}


bool RemoveFile(const std::string& sFilepath){
    return remove(sFilepath.c_str()) == 0;
}

} // namespace gamepath
