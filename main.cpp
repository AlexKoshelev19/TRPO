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
namespace fs = std::filesystem;

map<string, set<string>> rhymes;
map<string, vector<int>> VERBS_and_indexes;
bool is_running = true;

enum class ACTIONS : int
{
    NO_ACTION,
    MOVE_UP,
    MOVE_DOWN,
    EXIT,
    ENTER,
    BACK,
};

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

fs::path get_dir(string mode) {
    fs::path words = mode;
    error_code ec;
    fs::create_directories(words, ec);

    // Проверяем именно факт ОШИБКИ, а не то, существовала ли папка раньше
    if (ec) {
        cout << "Критическая ошибка создания директории " << words
             << ": " << ec.message() << " (Код: " << ec.value() << ")\n";
        return fs::path(); // Возвращаем пустой путь в случае сбоя
    }
    string file_name = mode + "_" + generateFileName();
    fs::path filePath = words / file_name;
    return filePath;
}

void save_verbs()
{
    fs::path filePath = get_dir("all_verbs");
    if (filePath.empty()) {
        cout << "Сохранение отменено: не удалось перенаправить путь.\n";
        return;
    }
    ofstream fos(filePath, ios::out | ios::trunc);

    fos << "unic verbs count: " << VERBS_and_indexes.size() << "\n\n";
    
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
    fos.close();
    cout << "Статистика по каждому из глаголов сохранена в файл: " << filePath.string() << '\n';
}

void save_rhymes()
{
    fs::path filePath = get_dir("unic_rhymes");
    if (filePath.empty()) {
        cout << "Сохранение отменено: не удалось перенаправить путь.\n";
        return;
    }
    ofstream fos(filePath, ios::out | ios::trunc);

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
    fos.close();

    cout << "Рифмующиеся глаголы сохранены в файл: " << filePath.string() << '\n';
} 

// Получение нажатой клавиши
ACTIONS getPressedKey()
{
    unsigned int key = _getch();

    switch (key)
    {
    case 0x0D: // Enter
        return ACTIONS::ENTER;

    case 0x08: // Backspace
        return ACTIONS::BACK;
        
    case 0x1B: // ESC
        return ACTIONS::EXIT;

    case 0xE0: // Первый байт кода стрелок в Windows
        {
            if (!_kbhit())
                return ACTIONS::NO_ACTION;

            key = _getch();

            if (key == 0x48) // Стрелка вверх
            {
                cout << "up" << endl;
                return ACTIONS::MOVE_UP;
            }
            else if (key == 0x50) // Стрелка вниз
            {
                cout << "down" << endl;
                return ACTIONS::MOVE_DOWN;
            }
        }

    default:
        return ACTIONS::NO_ACTION;
    }
}

void draw_menu(int index, string page_choice[], int size) {
    for (int i = 0; i < size; i++) {
        if (i == index)
            cout << "> " << page_choice[i] << endl;
        else
            cout << "  " << page_choice[i] << endl;
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

// функция управления меню выбора файла
string choose_data_file_menu()
{
    const int MAX_FILES = 20;
    string files[MAX_FILES];
    int count = collect_valid_files(files, MAX_FILES);

    if (count == 0) {
        cout << "Подходящих файлов не найдено.\n";
        getch();
        return "";
    }

    int index = 0;
    ACTIONS action;
    string message = "Выберите файл для анализа. Для перемещения курсора по пунктам меню используйте клавиши стрелок вверх и вниз.\nДля выбора файла нажмите клавишу Enter, для завершения работы с программой нажмите клавишу ESC.";
    do
    {
        system("cls");
        cout << message << "\n\n";
        draw_menu(index, files, count);

        action = getPressedKey();

        if (action == ACTIONS::MOVE_UP)
        {
            index = (index - 1 + count) % count;
        }
        else if (action == ACTIONS::MOVE_DOWN)
        {
            index = (index + 1) % count;
        }
        else if (action == ACTIONS::EXIT)
        {
            is_running = false;
            return "";
        }

    } while (action != ACTIONS::ENTER);

    return files[index];
}

void extract_verbs(string input_file)
{
    if (input_file.empty())
    {
        return;
    }

    PyObject* pModule = PyImport_ImportModule("verb_analiser");
    if (!pModule) {
        PyErr_Print();
        Py_Finalize();
        cerr << "Не удалось загрузить модуль" << endl;
        return;
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, "analyze");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        Py_XDECREF(pFunc);
        Py_XDECREF(pModule);
        Py_Finalize();
        cerr << "Функция analyze не найдена" << endl;
        return;
    }

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
    } else {
        PyErr_Print();
    }

    Py_XDECREF(pResult);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);

    return;
}

int main() {
    Py_Initialize();
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    cout << "Здравствуйте!\nДанная программа предназначена для поиска рифмующихся пар глаголов.\nДля завершения работы с программой нажмите клавишу ESC, а для продолжения - любую другую клавишу.";
    
    if (getPressedKey() == ACTIONS::EXIT)
    {
        is_running = false;
        Py_Finalize();
    }

    while (is_running)
    {
        string input_file = choose_data_file_menu();
        
        if (is_running)
        {
            system("cls");
            cout << "Загрузка глаголов.";
  
            extract_verbs(input_file);

            cout << "Глаголы загружены.\n";
            cout << "Глаголы загружены.\n";
            search_rhyme();
            cout << "Анализ проведён.\n";
            save_rhymes();
            save_verbs();

            cout << "Для завершения работы с программой нажмите клавишу ESC, для продолжения - любую другую клавишу";
            if (getPressedKey() == ACTIONS::EXIT)
            {
                Py_Finalize();
                is_running = false;
            }
        }
    }

    return 0;
}