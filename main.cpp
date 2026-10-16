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
#include <conio.h>

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

void draw_menu(int index, string page_choice[], int size) {
    // Очищаем экран перед каждым выводом, чтобы меню не дублировалось
    for (int i = 0; i < size; i++) {
        if (i == index)
            cout << "> " << page_choice[i] << endl;
        else
            cout << "  " << page_choice[i] << endl; // Добавили пробелы для выравнивания
    }
}

int choice_menu(string page_choice[], string message, int size)
{

    int index = 0;
    cout << message << endl << endl;
    draw_menu(index, page_choice, size);
    int c;
    do{

        c = getch();
        system("cls");
        if (c == 224 || c == 0) {
            cout << message << endl << endl;
            c = getch();

            if (c == 72) { // Стрелка ВВЕРХ
                index = (index - 1 + size) % size;
                draw_menu(index, page_choice, size);
            }
            else if (c == 80) {
                index = (index + 1) % size;
                draw_menu(index, page_choice, size);
            }
            else {
                draw_menu(index, page_choice, size); }
        }
        else if(c == 13)
        {
            return index;
        }
        else {
            cout << message <<endl<<endl;
            draw_menu(index, page_choice, size); }
    }
    while (c != 27);
    return -1;
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



bool is_valid_data_file(const string& file_name)
{
    ifstream fin(file_name);
    if (!fin.is_open()) return false;

    fin.close();
    return true;
}


// возвращает число найденных файлов, имена пишет в files[]
int collect_valid_files(string files[], int max_files)
{
    int count = 0;

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("*.txt", &fd);
    if (h == INVALID_HANDLE_VALUE)
        return 0;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        string name = fd.cFileName;
        if (is_valid_data_file(name)) {
            if (count < max_files) {
                files[count] = name;
                count++;
            }
        }
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    return count;
}

string choose_data_file()
{
    const int MAX_FILES = 50;
    string files[MAX_FILES];
    int n = collect_valid_files(files, MAX_FILES);

    if (n == 0) {
        cout << "Подходящих файлов не найдено.\n";
        getch();
        return "";
    }

    // последний пункт — отмена
    files[n] = "Отмена";
    int size = n + 1;

    string message = "Выберите файл для загрузки:";
    int index = choice_menu(files, message, size);

    if (index < 0 || index >= n)  // Esc или «Отмена»
        return "";

    return files[index];
}

int main_algorytm() {
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

    string input_file = choose_data_file();

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
    cout << "Анализ проведен. Данные сохранены в файлы. \n";
    cout << "Время выполнения: " << duration.count() << " мс\n";
    cout << "нажмите любую клавишу";
    getch();
    system("cls");
    Py_XDECREF(pResult);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
    Py_Finalize();
    return 0;
}

void greeting_menu() {
    string page_choice[] = {
        "Начать работу",
        "Выйти из программы",
        "Инструкция"
        };

    int menu_size = size(page_choice);
    string message = "Здравствуйте!\n Данная программа позволяет находить рифмы среди глаголов в текстовом файле. Результатом работы программы являются файлы с именами: all_verbs_<id> и unic_rhymes_<id>. ";

    int index=0;
    do{
        index = choice_menu(page_choice, message, menu_size);
        switch (index) {
            case 0: main_algorytm();  break;
            case 1:  break;
            case 2:  break;
            case -1: break;
            default: break;
        }
    } while(index != -1);
    return;
}

int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    greeting_menu();
    return 0;
}