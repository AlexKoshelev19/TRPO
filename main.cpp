#include <Python.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace std;

typedef struct {
    string word;
    vector<string> rhymed;
} VERB;

map<string, set<string>> rhymes;
map<string, vector<int>> VERBS_and_indexes;

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
    for (const auto& [verb, _] : VERBS_and_indexes) {
        string curr_ending = get_ending(verb);
        if (curr_ending.empty()) continue;

        if(rhymes.contains(curr_ending))
        {
            set<string> v = rhymes[curr_ending];
            v.insert(verb);
            rhymes[curr_ending] = v;
        }
        else
        {
            set<string> v {verb};
            rhymes[curr_ending] = v;
        }
    }
}

// Функция для генерации имени выходного файла
string generateFileName()
{
    auto now = chrono::system_clock::now();
    auto time_t_now = chrono::system_clock::to_time_t(now);
    tm* tm_now = localtime(&time_t_now);
    
    string data = to_string(tm_now->tm_hour);
    data += "_" + to_string(tm_now->tm_min);
    data += "_" + to_string(tm_now->tm_sec) + ".txt";
    
    return data;
}

// функция вывода в файл всех глаголов и номеров предложений, где они встретились
void save_verbs()
{
    string file_name = "all_verbs_" + generateFileName();
    filesystem::path path(file_name);
    ofstream fos(path, ios::ate);

    fos << "Найдено уникальных глаголов: " << VERBS_and_indexes.size() << "\n\n";

    int ind = 1;
    for(const auto& [verb, sentences] : VERBS_and_indexes)
    {
        fos << ind << ") " << verb << " : ";
        for(const auto & sentence : sentences)
        {
            fos << sentence << ' ';
        }
        fos << '\n';
        ind++;
    }
    fos.flush();
    fos.close();
}

// функция вывода в файл всех рифмующихся пар
void save_rhymes()
{
    int unic_pairs = 0;
    string file_name = "unic_rhymes_" + generateFileName();
    filesystem::path path(file_name);
    ofstream fos(path, ios::ate);

    int ind = 1;
    for(const auto& [end, v] : rhymes)
    {
        if(v.size() > 1)
        {
            for (auto it1 = v.begin(); it1 != v.end(); ++it1) {
                auto it2 = it1;
                ++it2;
                for (; it2 != v.end(); ++it2) {
                    fos << ind++ << ") " << *it1 << " - " << *it2 << '\n';
                }
            }
            unic_pairs += v.size() * (v.size() - 1) / 2;
        }
    }

    fos.flush();
    fos.close();
}

int main() {
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

    auto start = chrono::high_resolution_clock::now();

    // Создаём аргументы и вызываем функцию
    PyObject* pArgs = PyTuple_New(1);
    PyObject* pValue = PyUnicode_FromString(input_file.c_str());
    PyTuple_SetItem(pArgs, 0, pValue); // владение передаётся кортежу

    PyObject* pResult = PyObject_CallObject(pFunc, pArgs);

    if (pResult && PyDict_Check(pResult)) {
        Py_ssize_t size = PyDict_Size(pResult);

        PyObject *key = nullptr;
        PyObject *value = nullptr;
        Py_ssize_t pos = 0;

        // PyDict_Next итерируется по всем парам (key, value)
        while (PyDict_Next(pResult, &pos, &key, &value)) {
            const char* verb = PyUnicode_AsUTF8(key);
            
            if (verb && PyList_Check(value)) {
                Py_ssize_t list_size = PyList_Size(value);
                vector<int> sentence_indices;
                sentence_indices.reserve(list_size);

                for (Py_ssize_t j = 0; j < list_size; ++j) {
                    PyObject* item = PyList_GetItem(value, j);
                    if (PyLong_Check(item)) {
                        sentence_indices.push_back(static_cast<int>(PyLong_AsLong(item)));
                    }
                }

                // Сохраняем в словарь
                VERBS_and_indexes[verb] = move(sentence_indices);
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

    // Очистка
    Py_XDECREF(pResult);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
    Py_Finalize();

    return 0;
}