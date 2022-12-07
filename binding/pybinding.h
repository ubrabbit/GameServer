#pragma once

#include <Python.h>
#include "common/singleton.h"
#include "common/logger.h"

#define PYTHON_BASIC_SCRIPT_DIR "script"

namespace gamebinding
{

class PythonBinding : public Singleton<PythonBinding>
{
public:

    void InitPythonBinding();

    void QuitPythonBinding();

    virtual void BindPythonModules() {};

private:

    inline void _CallPythonWithoutArgs(const char* pchModuleName, const char* pchFuncName);
    inline void _CheckPythonCallback(PyObject* pstResult);

};

} // namespace gamebinding
