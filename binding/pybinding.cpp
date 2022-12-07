#include "pybinding.h"

namespace gamebinding
{

/* ********** Python 模块初始化开始 ********** */
static PyMethodDef CGameMethods[] = {
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef CGameModule = {
    PyModuleDef_HEAD_INIT,
    "cgame",
    "cgame",
    -1,
    CGameMethods,
};
/* ********** Python 模块初始化结束 ********** */

PyMODINIT_FUNC PyInit_cgame(void){
    return PyModule_Create(&CGameModule);
};

inline void PythonBinding::_CheckPythonCallback(PyObject* pstResult)
{
	if (NULL == pstResult)
    {
		PyErr_Print();
	    return;
    }
	Py_DECREF(pstResult);
}

inline void PythonBinding::_CallPythonWithoutArgs(const char* pchModuleName, const char* pchFuncName)
{
    PyObject* pstModule = PyImport_ImportModule(pchModuleName);
    assert(pstModule);

    PyObject* pstFunc = PyObject_GetAttrString(pstModule, pchFuncName);
    PyObject* pstResult = PyObject_CallObject(pstFunc, NULL);
    _CheckPythonCallback(pstResult);

    Py_DECREF(pstModule);
    Py_DECREF(pstFunc);
}

void PythonBinding::InitPythonBinding()
{
    LOGINFO("init python binding");

    /* Add a built-in module, before Py_Initialize */
    PyImport_AppendInittab("cgame", PyInit_cgame);
    BindPythonModules();

    /* Initialize the Python interpreter.  Required. */
    Py_Initialize();

    /* Optionally import the module; alternatively,
       import can be deferred until the embedded script
       imports it. */
    PyImport_ImportModule("cgame");

    /* 脚本模块初始化 */
    PyRun_SimpleString("import sys");
    char buf[256] = {0};
    sprintf(buf, "sys.path.append('%s')", PYTHON_BASIC_SCRIPT_DIR);
    PyRun_SimpleString(buf);
    PyObject* pModule = PyImport_ImportModule("init");
    if (!pModule)
    {
        LOGERROR("Python load module init failed.");
        PyErr_Print();
        exit(1);
    }

    /* 调用初始化函数 Init */
    _CallPythonWithoutArgs("init", "Init");
}

void PythonBinding::QuitPythonBinding()
{
    _CallPythonWithoutArgs("init", "Quit");
}

} // namespace gamebinding
