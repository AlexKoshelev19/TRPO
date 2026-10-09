#include <Python.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cctype>

using namespace std;

typedef struct {
    string word;
    vector<string> rhymed;
} VERB;

// словарь глагол и множество рифмующихся глаголов
unordered_map<string, unordered_set<string>> VERBS;

// множество выведенных глаголов (для фильтрации дубликатов при выводе)
unordered_set<string> hasUsed;

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

        for (const auto& [current_verb, _] : VERBS) {
            string next_ending = get_ending(current_verb);
            if (next_ending.empty()) continue;

            if ((next_ending == curr_ending)&&(verb!=current_verb)) {
                VERBS[verb].insert(current_verb);
                VERBS[current_verb].insert(verb);
            }
        }
    }
}
int main() {
    int unic_pairs = 0;
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
                cout << (i + 1) << ") " << verb << '\n';
                if (!VERBS.contains(verb)) {
                    VERBS[verb];
                }
                else continue;
            }
        }

        search_rhyme();

        cout << "\nРифмующиеся комбинации:\n";
        // Вывод результатов
        for (const auto& [verb, rhymed_list] : VERBS) {
            if(!rhymed_list.empty())
            {
                string out = " - " + verb + '\n';
                bool hasPair = false;
                for (const auto& elm : rhymed_list)
                {
                    if(!hasUsed.count(elm))
                    {
                        out += '\t' + elm + '\n';
                        hasPair = true;
                        unic_pairs ++;
                    }
                }
                if(hasPair)
                    cout << out << '\n';
                hasUsed.insert(verb);
            }
        }
    } else {
        PyErr_Print();
    }

    cout << "Итого уникальных пар: " << unic_pairs << '\n';

    // Очистка
    Py_XDECREF(pResult);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
    Py_Finalize();

    return 0;
}