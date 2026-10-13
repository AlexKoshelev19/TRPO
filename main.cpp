#include <Python.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <windows.h>

using namespace std;

map<string, set<string>> rhymes;
map<string, vector<int>> VERBS_and_indexes;

// Поиск начала гласной буквы, на которой стоит ударение \u0301
int accent_index(const string& str) {
    size_t pos = str.find("\u0301");
    if (pos == string::npos || pos == 0) return -1;

    size_t i = pos - 1;
    // Пропускаем хвостовые байты UTF-8 (10xxxxxx), чтобы встать на заголовочный байт буквы
    while (i > 0 && (static_cast<unsigned char>(str[i]) & 0xC0) == 0x80) {
        --i;
    }
    return static_cast<int>(i);
}

string get_ending(const string& str) {
    int idx = accent_index(str);
    if (idx < 0) return "";
    return str.substr(idx);
}

void search_rhyme()
{
    for (const auto& [verb, _] : VERBS_and_indexes)
    {
        string curr_ending = get_ending(verb);
        if (curr_ending.empty()) continue;

        rhymes[curr_ending].insert(verb);
    }
}

string generateFileName()
{
    auto now = chrono::system_clock::now();
    auto time_t_now = chrono::system_clock::to_time_t(now);
    tm tm_now{};
    localtime_s(&tm_now, &time_t_now);
    
    return to_string(tm_now.tm_hour) + "_" + 
           to_string(tm_now.tm_min) + "_" + 
           to_string(tm_now.tm_sec) + ".txt";
}

void save_verbs()
{
    string file_name = "all_verbs_" + generateFileName();
    ofstream fos(file_name, ios::out | ios::trunc);

    fos << "Найдено уникальных глаголов: " << VERBS_and_indexes.size() << "\n\n";

    int ind = 1;
    for (const auto& [verb, sentences] : VERBS_and_indexes)
    {
        fos << ind << ") " << verb << " : ";
        for (const auto& sentence : sentences)
        {
            fos << sentence << ' ';
        }
        fos << '\n';
        ind++;
    }
}

void save_rhymes()
{
    string file_name = "unic_rhymes_" + generateFileName();
    ofstream fos(file_name, ios::out | ios::trunc);

    int ind = 1;
    for (const auto& [end, v] : rhymes)
    {
        if (v.size() > 1)
        {
            for (auto it1 = v.begin(); it1 != v.end(); ++it1)
            {
                auto it2 = next(it1);
                for (; it2 != v.end(); ++it2)
                {
                    fos << ind++ << ") " << *it1 << " - " << *it2 << '\n';
                }
            }
        }
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    Py_Initialize();

    PyObject* pModule = PyImport_ImportModule("verb_analiser");
    if (!pModule) {
        PyErr_Print();
        cerr << "Не удалось загрузить модуль" << endl;
        Py_Finalize();
        return 1;
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, "analyze");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        cerr << "Функция analyze не найдена" << endl;
        Py_XDECREF(pFunc);
        Py_XDECREF(pModule);
        Py_Finalize();
        return 1;
    }

    string input_file;
    cout << "Введите название файла: ";
    getline(cin, input_file);

    auto start = chrono::high_resolution_clock::now();

    PyObject* pArgs = PyTuple_New(1);
    PyObject* pValue = PyUnicode_FromString(input_file.c_str());
    PyTuple_SetItem(pArgs, 0, pValue);

    PyObject* pResult = PyObject_CallObject(pFunc, pArgs);

    if (pResult && PyDict_Check(pResult)) {
        PyObject *key = nullptr;
        PyObject *value = nullptr;
        Py_ssize_t pos = 0;

        while (PyDict_Next(pResult, &pos, &key, &value)) {
            const char* verb_utf8 = PyUnicode_AsUTF8(key);
            
            if (verb_utf8 && PyList_Check(value)) {
                Py_ssize_t list_size = PyList_Size(value);
                vector<int> sentence_indices;
                sentence_indices.reserve(list_size);

                for (Py_ssize_t j = 0; j < list_size; ++j) {
                    PyObject* item = PyList_GetItem(value, j);
                    if (PyLong_Check(item)) {
                        sentence_indices.push_back(static_cast<int>(PyLong_AsLong(item)));
                    }
                }

                // Прямая запись без вызова MultiByteToWideChar
                VERBS_and_indexes[string(verb_utf8)] = move(sentence_indices);
            }
        }

        search_rhyme();
        save_verbs();
        save_rhymes();
    } else {
        PyErr_Print();
    }

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Время выполнения: " << duration.count() << " мс\n";

    Py_XDECREF(pResult);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
    Py_Finalize();

    return 0;
}