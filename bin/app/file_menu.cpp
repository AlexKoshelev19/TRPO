#include "test2.h"

void download_from_file()
{
    cout << "Введите имя входного файла: ";
    string file_name = choose_data_file();

    if (file_name.empty()) {
        system("cls");
        return;
    }


    ifstream fin(file_name);

    if (!fin.is_open()) {
        cout << "Ошибка открытия файла!\n";
        return;
    }

    string line;
    
    while (getline(fin, line)) {
        if (line == "NODES") continue;
        if (line == "EDGES") break;          // разделитель разделов
        if (line.empty()) continue;

        string fields[4];
        size_t start = 0;
        int i = 0;

        // парсим первые 3 поля
        for (; i < 3; ++i) {
            size_t end = line.find(',', start);
            if (end == string::npos) break;
            fields[i] = line.substr(start, end - start);
            start = end + 1;
        }
        fields[i] = line.substr(start);      // последнее поле

        if (i == 3) {
            create_airport(fields[0], fields[1], fields[2], fields[3], true);
        }
    }

    while (getline(fin, line)) {
        if (line.empty()) continue;

        string fields[4];
        size_t start = 0;
        int i = 0;

        for (; i < 3; ++i) {
            size_t end = line.find(',', start);
            if (end == string::npos) break;
            fields[i] = line.substr(start, end - start);
            start = end + 1;
        }
        fields[i] = line.substr(start);

        if (i != 3) continue;                // строка повреждена
        Airport* temp_dep = find_Airport(fields[0]);
        if (temp_dep == nullptr) {
            create_airport(fields[0], "NULL", "NULL", "NULL", true);
            temp_dep = find_Airport(fields[0]);
        }

        Airport* temp_arr = find_Airport(fields[1]);
        if (temp_arr == nullptr) {
            create_airport(fields[1], "NULL", "NULL", "NULL", true);
            temp_arr = find_Airport(fields[1]);
        }
        create_route(temp_dep, temp_arr, fields[2], fields[3], true);
    }
    fin.close();
    cout << "Данные успешно загружены.\n нажмите любую кнопку";
    getch();
    system("cls"); return;
}


void upload_to_file()
{
    cout << "Введите имя выходного файла: ";
    string file_name;
    getline(cin, file_name);

    ofstream fout(file_name);
    if (!fout.is_open()) {
        cout << "Ошибка открытия файла для записи!\n";
        return;
    }


    fout << "NODES\n";

    Airport* temp = main_ptr;
    if (temp == nullptr) {
        cout << "Список аэропортов пуст!\n";
    }

    while (temp != nullptr) {
        fout << temp->name << ","
             << temp->city << ","
             << temp->country << ","
             << temp->code << "\n";
        temp = temp->next_airport;   // ← обязательно!
    }

    fout << "EDGES\n";
    temp = main_ptr;
    while (temp != nullptr) {
        Route* r = temp->routes;
        while (r != nullptr) {
            fout << r->dep_airport_ptr->name << ","
                 << r->arr_airport_ptr->name << ","
                 << r->distance << ","
                 << r->flight_time << "\n";
            r = r->next_route;
        }
        temp = temp->next_airport;
    }

    fout.close();
    cout << "Данные успешно сохранены в \"" << file_name << "\".\n";
}
bool is_valid_data_file(const string& file_name)
{
    ifstream fin(file_name);
    if (!fin.is_open())
        return false;

    string line;
    bool in_edges = false;
    int airports = 0;
    int routes = 0;

    while (getline(fin, line)) {

        if (line.empty())
            continue;

        if (line == "NODES")
            continue;

        if (line == "EDGES") {
            in_edges = true;
            continue;
        }

        // подсчёт полей через запятую
        int commas = 0;
        for (char c : line)
            if (c == ',') commas++;

        // нужно ровно 4 поля → 3 запятые
        if (commas != 3)
            return false;

        if (!in_edges)
            airports++;
        else
            routes++;
    }

    fin.close();

    // минимальные требования к формату
    return in_edges && airports > 0;  // EDGES был, хотя бы один аэропорт
    // если нужны и маршруты: return in_edges && airports > 0 && routes > 0;
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






















