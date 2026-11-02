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
    rhymes.clear();
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

fs::path get_dir(const string& mode, const string&  base_name) {
    fs::path words = mode;
    error_code ec;
    fs::create_directories(words, ec);

    if (ec) {
        cout << "Критическая ошибка создания директории " << words.string()
             << ": " << ec.message() << " (Код: " << ec.value() << ")\n";
        return fs::path();
    }
    string file_name = base_name + "_" + mode + "_" +  generateFileName();
    return words / file_name; // вместо fs::u8path
}

void save_verbs(const string& base_name)
{
    fs::path filePath = get_dir("all_verbs", base_name);
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
    cout << "Статистика по каждому из глаголов сохранена в файл: " << fs::absolute(filePath).string() << '\n';
}

void save_rhymes(const string& base_name)
{
    fs::path filePath = get_dir("unic_rhymes", base_name);
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

    cout << "Рифмующиеся глаголы сохранены в файл: " << fs::absolute(filePath).string() << '\n';
}

ACTIONS getPressedKey()
{
    unsigned int key = _getch();

    switch (key)
    {
    case 0x0D:
        return ACTIONS::ENTER;

    case 0x08:
        return ACTIONS::BACK;
        
    case 0x1B:
        return ACTIONS::EXIT;

    case 0xE0:
        {
            if (!_kbhit())
                return ACTIONS::NO_ACTION;

            key = _getch();

            if (key == 0x48)
                return ACTIONS::MOVE_UP;
            else if (key == 0x50)
                return ACTIONS::MOVE_DOWN;
        }

    default:
        return ACTIONS::NO_ACTION;
    }
}

void draw_menu(int index, const vector<string>& page_choice) {
    for (size_t i = 0; i < page_choice.size(); ++i) {
        if (static_cast<int>(i) == index)
            cout << "> " << page_choice[i] << "\n";
        else
            cout << "  " << page_choice[i] << "\n";
    }
}

bool is_valid_data_file(const fs::path& file_path)
{
    ifstream fin(file_path);
    if (!fin.is_open()) return false;
    fin.close();
    return true;
}

vector<string> collect_valid_files()
{
    vector<string> files;
    fs::path input_dir = "input";

    error_code ec;
    if (!fs::exists(input_dir, ec) || !fs::is_directory(input_dir, ec)) {
        return files;
    }

    for (const auto& entry : fs::directory_iterator(input_dir, ec)) {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".txt") {
            if (is_valid_data_file(entry.path())) {
                // Заменяем u8string() на string()
                files.push_back(entry.path().filename().string());
            }
        }
    }
    return files;
}

string choose_data_file_menu()
{
    vector<string> files = collect_valid_files();

    if (files.empty()) {
        system("cls");
        cout << "В папке input подходящих файлов не найдено.\nДобавьте файлы в директорию app\\input и перезапустите программу.";
        _getch();
        return "";
    }

    int index = 0;
    int count = static_cast<int>(files.size());
    ACTIONS action;
    string message = "Выберите файл для анализа. Для перемещения курсора по пунктам меню используйте клавиши стрелок вверх и вниз.\n"
                     "Для выбора файла нажмите клавишу Enter, для завершения работы с программой нажмите клавишу ESC.";
    do
    {
        system("cls");
        cout << message << "\n\n";
        draw_menu(index, files);

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

void extract_verbs(const string& input_file)
{
    if (input_file.empty()) return;

    VERBS_and_indexes.clear();

    PyObject* pModule = PyImport_ImportModule("verb_analiser");
    if (!pModule) {
        PyErr_Print();
        cerr << "Не удалось загрузить модуль verb_analiser" << endl;
        return;
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, "analyze");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        Py_XDECREF(pFunc);
        Py_XDECREF(pModule);
        cerr << "Функция analyze не найдена" << endl;
        return;
    }

    PyObject* pArgs = PyTuple_New(1);
    // Передаем путь в Python как полноценный UTF-8
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
}

int main() {
    
    Py_Initialize();
    
    // Включаем UTF-8 для консоли Windows
    setlocale(LC_ALL, ".UTF-8");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    cout << "Здравствуйте!\nДанная программа предназначена для поиска рифмующихся пар глаголов.\n"
         << "Для завершения работы с программой нажмите клавишу ESC, а для продолжения - любую другую клавишу.\n";
    
    if (getPressedKey() == ACTIONS::EXIT)
    {
        is_running = false;
        Py_Finalize();
        return 0;
    }

    while (is_running)
    {
        string chosen_file = choose_data_file_menu();
        
        if (is_running && !chosen_file.empty())
        {
            system("cls");
            cout << "Загрузка глаголов...\n";
  
            // Прямая конкатенация путей через fs::path
            fs::path relative_file_path = fs::path("input") / chosen_file;
            extract_verbs(relative_file_path.string()); // передаем обычный std::string
            system("cls");
            cout << "Глаголы загружены.\n";
            search_rhyme();
            // Получаем имя файла без расширения (например, "стих" из "стих.txt")
            string base_input_name = fs::path(chosen_file).stem().string();

            cout << "Анализ проведён.\n";
            cout << "Разбитый по предложениям текст сохранён в директорию \\app\\edited.\n";
            save_rhymes(base_input_name);
            save_verbs(base_input_name);

            cout << "\nДля завершения работы с программой нажмите клавишу ESC, для продолжения - любую другую клавишу: ";
            if (getPressedKey() == ACTIONS::EXIT)
            {
                is_running = false;
                Py_Finalize();
            }
        }
        else
        {
            is_running = false;
            Py_Finalize();
        }
    }

    Py_Finalize();
    return 0;
}