#include <Python.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <vector>
#include <cctype>

using namespace std;

typedef struct {
    string word;
    vector<string> rhymed;
} VERB;

vector<VERB> verbs;

wstring utf8_to_wstring(const string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size);
    return result;
}

string wstring_to_utf8(const wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size, nullptr, nullptr);
    return result;
}


int accent_index(const string& str) {
    wstring wstr = utf8_to_wstring(str);
    for (size_t i = 0; i < wstr.size(); ++i) {
        if (iswupper(wstr[i])) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

string get_ending(const string& str) {
    int idx = accent_index(str);
    if (idx < 0) return "";

    wstring wstr = utf8_to_wstring(str);
    return wstring_to_utf8(wstr.substr(idx));
}

bool is_duplicate(const VERB& verb, const string& str) {
    for (const auto& r : verb.rhymed) {
        if (r == str) return true;
    }
    return false;
}

void search_rhyme() {
    for (size_t i = 0; i < verbs.size(); ++i) {
        string curr_ending = get_ending(verbs[i].word);
        if (curr_ending.empty()) continue;

        for (size_t j = i + 1; j < verbs.size(); ++j) {
            string next_ending = get_ending(verbs[j].word);
            if (next_ending.empty()) continue;

            if (next_ending == curr_ending) {
                if (!is_duplicate(verbs[i], verbs[j].word)) {
                    verbs[i].rhymed.push_back(verbs[j].word);
                    verbs[j].rhymed.push_back(verbs[i].word);
                }
            }
        }
    }
}
int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // Инициализируем интерпретатор Python
    Py_Initialize();

    // Добавляем пути в sys.path
    PyRun_SimpleString("import sys; sys.path.append('.')");
    PyRun_SimpleString("sys.path.append('D:/study/TRPO/test_accent/.venv/Lib/site-packages')");

    // Импортируем модуль
    PyObject* pModule = PyImport_ImportModule("verb_analiser");
    if (!pModule) {
        PyErr_Print();
        cerr << "Не удалось загрузить модуль" << endl;
        Py_Finalize();
        return 1;
    }

    // Получаем функцию analyze
    PyObject* pFunc = PyObject_GetAttrString(pModule, "analyze");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        cerr << "Функция analyze не найдена" << endl;
        Py_XDECREF(pModule);
        Py_Finalize();
        return 1;
    }

    string input_file;
    cout << "Введите название файла: ";
    getline(cin, input_file);

    // Создаём аргументы и вызываем функцию
    PyObject* pArgs = PyTuple_New(1);
    PyObject* pValue = PyUnicode_FromString(input_file.c_str());
    PyTuple_SetItem(pArgs, 0, pValue); // владение передаётся кортежу

    PyObject* pResult = PyObject_CallObject(pFunc, pArgs);

    if (pResult && PyList_Check(pResult)) {
        Py_ssize_t size = PyList_Size(pResult);
        cout << "Найдено глаголов: " << size << endl;

        for (Py_ssize_t i = 0; i < size; ++i) {
            PyObject* item = PyList_GetItem(pResult, i); // borrowed reference
            const char* verb = PyUnicode_AsUTF8(item);
            if (verb) {
                VERB temp;
                temp.word = verb;
                verbs.push_back(temp);
            }
        }


        search_rhyme();

        // Вывод результатов
        for (size_t i = 0; i < verbs.size(); ++i) {
            cout << " - " << verbs[i].word << endl;
            for (size_t j = 0; j < verbs[i].rhymed.size(); ++j) { // было i++ — ошибка
                cout << '\t' << verbs[i].rhymed[j] << endl;
            }
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