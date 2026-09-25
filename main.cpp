#include <Python.h>
#include <iostream>
#include <string>
#include <windows.h>
using namespace std;
int main() {

    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // Инициализируем интерпретатор Python
    Py_Initialize();

    // Добавляем путь к скрипту в sys.path
    PyRun_SimpleString("import sys; sys.path.append('.')");

    // Импортируем модуль
    PyObject* pModule = PyImport_ImportModule("test_file");
    if (!pModule) {
        PyErr_Print();
        std::cerr << "Не удалось загрузить модуль" << std::endl;
        return 1;
    }

    // Получаем функцию analyze
    PyObject* pFunc = PyObject_GetAttrString(pModule, "analyze");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        std::cerr << "Функция analyze не найдена" << std::endl;
        return 1;
    }
    string input_file;
    cout << "Введите название файла: ";
    getline(cin, input_file);


    // Вызываем функцию с аргументом
    PyObject* pArgs = PyTuple_New(1);
    PyObject* pValue = PyUnicode_FromString(
        input_file.c_str()
    );
    PyTuple_SetItem(pArgs, 0, pValue);  // владение передаётся кортежу

    PyObject* pResult = PyObject_CallObject(pFunc, pArgs);

    if (pResult && PyList_Check(pResult)) {
        Py_ssize_t size = PyList_Size(pResult);
        std::cout << "Найдено глаголов: " << size << std::endl;

        for (Py_ssize_t i = 0; i < size; ++i) {
            PyObject* item = PyList_GetItem(pResult, i);
            const char* verb = PyUnicode_AsUTF8(item);
            std::cout << "  - " << verb << std::endl;
        }
    } else {
        PyErr_Print();
    }

    // Очистка
    Py_XDECREF(pResult);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
    Py_Finalize();

    return 0;
}