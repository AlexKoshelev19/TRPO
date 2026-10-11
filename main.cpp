#include <Python.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cctype>
#include <chrono>

using namespace std;

typedef struct {
    string word;
    vector<string> rhymed;
} VERB;

// словарь глагол и множество рифмующихся глаголов
unordered_map<string, unordered_set<string>> VERBS;
unordered_map<string, unordered_set<string>> rhymes;

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
    for (const auto& [verb, _] : VERBS) {
        string curr_ending = get_ending(verb);
        if (curr_ending.empty()) continue;

        if(rhymes.contains(curr_ending))
        {
            unordered_set<string> v = rhymes[curr_ending];
            v.insert(verb);
            rhymes[curr_ending] = v;
        }
        else
        {
            unordered_set<string> v {verb};
            rhymes[curr_ending] = v;
        }
    }
}
int main() {
    int unic_pairs = 0;
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // Инициализируем интерпретатор Python
    Py_Initialize();

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

    auto start = std::chrono::high_resolution_clock::now();

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
                cout << (i + 1) << ") " << verb << '\n';
                if (!VERBS.contains(verb)) {
                    VERBS[verb];
                }
                else continue;
            }
        }

        search_rhyme();

        cout << "\nРифмующиеся комбинации:\n";

        for(const auto& [end, v] : rhymes)
        {
            if(v.size() > 1)
            {
                for(const auto& verb: v)
                {
                    cout << verb << ", ";
                }
                cout << '\n';
                unic_pairs += (2 + (v.size() - 1) - 1) * (v.size() - 1) / 2;
            }
        }
    } else {
        PyErr_Print();
    }

    auto end = std::chrono::high_resolution_clock::now();

    cout << "Итого уникальных пар: " << unic_pairs << '\n';

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Время выполнения: " << duration.count() << " мс\n";

    // Очистка
    Py_XDECREF(pResult);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
    Py_Finalize();

    return 0;
}