#include "test2.h"


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

void greeting_menu(){
    string page_choice[] = {
        "Создать аэропорт", 
        "Редактировать аэропорт",
        "Удалить аэропорт",
        "Создать маршрут",
        "Редактировать маршрут", // редактирование маршрута
        "Удалить маршрут", 
        "Найти прямой маршрут между аэропортами",
        "Найти маршрут с пересадками",
        "Вывод одного аэропорта с маршрутами",
        "Вывести все аэропорты",
        "Вывести все маршруты",
        "Работать с файлами" };
        
    int size = sizeof(page_choice) / sizeof(page_choice[0]);
    string message = "Здравствуйте!\n Данная программа позволяет создавать транспортную сеть аэропортов. Вы можете создать отдельно аэропорты и соединять их маршрутами. При первом запуске программе, рекомендую ознакомиться с инструкцией и далее следовать сообщениям на экране.";
    int index=0;
    do{
        index = choice_menu(page_choice, message, size);
        switch (index) {
            case 0: create_airport_mode(); break; // создать маршрут
            case 1: edit_airport_mode(); break;
            case 2: delete_airplane_mode();break;
            case 3: route_mode(3);   break;
            case 4: route_mode(4);   break;          // редактирование маршрута
            case 5: route_mode(5);   break;          // удаление маршрута
            case 6: route_mode(6);   break;          // прямой маршрут
            case 7: route_mode(7);   break;          // маршрут с пересадками
            case 8: route_mode(8);   break;          // вывод одного маршрута
            case 9: print_all_airports(); break;          // вывод аэропортов
            case 10:print_all_routes(); break;
            case 11:file_mode(); break;
            case -1: break;
            default: break;
        }
    } while(index != -1);
    return; 
}


void file_mode()
{
    string page_choice[] = {"Загрузить данные из файла", "Выгрузить данные в файл", "Выйти в главное меню"};
    int size = sizeof(page_choice) / sizeof(page_choice[0]);
    string message = "Для загрузки транспортной сети убедитесь в наличии файла transport_net.txt в директории программы.";
    int index=0;
    do
    {
        index = choice_menu(page_choice, message, size);
        switch (index)
        {
            case  0: system("cls"); download_from_file(); return;;
            case  1: system("cls"); upload_to_file(); return;
            case  2: return;
            case -1: break;
            default: continue;
        }
   } while(index != -1);
    system("cls");
    return; 
}
void create_airport_mode(Airport* &temp)
{   char c;
    do {
        system("cls");
        string message = "Для создания аэропорта вам необходимо ввести данные в следующие поля: Код аэропорта, название, город, страну(exit - для выхода) ";
        cout << message << endl;

        cout << endl << " Введите название аэропорта: ";
        string name_airport = good_scan(is_valid_word);
        if(name_airport == "exit") break;

        cout << endl << " Введите код аэропорта: ";
        string code_airport = good_scan(is_valid_code);
        if(code_airport == "exit") break;

        cout << endl << " Введите город аэропорта: ";
        string city_airport = good_scan(is_valid_word);
        if(city_airport == "exit") break;
        

        cout << endl << " Введите страну аэропорта: ";
        string country_airport = good_scan(is_valid_word);
        if(country_airport == "exit") break;


        create_airport(name_airport, city_airport, country_airport, code_airport);

        temp = find_Airport(name_airport);
        cout << endl;
        print_Airport(temp);
        cout << "нажмите любую клавишу чтобы добавить новый аэропорт(esc-для выхода в главное меню)" << endl;
        c = getch();
    } while (c !=27 );
    system("cls");
}




void create_airport_mode()
{
    Airport* local_temp = nullptr;
    create_airport_mode(local_temp);
}





bool create_route_mode(Airport* dep_airport_ptr, Airport* arr_airport_ptr)
{
    cout << endl << "Введите расстояние между аэропортами: ";
    string distance = good_scan(is_valid_digit);
    if (distance == "exit") return false;

    cout << endl << "Введите продолжительность полета: ";
    string flight_time = good_scan(is_valid_digit);
    if (flight_time == "exit") return false;

    Route* temp = create_route(dep_airport_ptr, arr_airport_ptr, distance, flight_time );
    if (temp == nullptr) {
        cout << "Такой маршрут уже существует. нажмите любую клавишу для выхода";
        getch();
        return false;

    }
    cout << "Маршрут создан!" << endl;

    print_one_route(dep_airport_ptr, arr_airport_ptr);

    cout << "нажмите любую клавишу";

    system("cls");
    return true;
}


void delete_airplane_mode()
{
    char c;
    do {
        system("cls");
        Airport* temp = input_airport("Введите название аэропорта, который вы хотите удалить(exit - для выхода): ");
        if (temp == nullptr){system("cls"); return; }  // пользователь выбрал выход
        delete_airport(temp);
        cout << "Аэропорт удалён.\n";
        cout << "Для продолжения выбранного действия нажмите любую клавишу(esc - выход)";
        c = getch();
    } while (c!=27);
    system("cls");
}

void edit_airport_mode() {

        system("cls");
        Airport* current = input_airport("\nВведите название аэропорта данные которого вы хотите изменить(exit - для выхода): ");
        if (current == nullptr) {system("cls"); return; }
        edit_airport(current);


}
void edit_airport(Airport* current) {
    Airport* temp = current;
    char c;
    string name = current->name;
    string code = current->code;
    string country = current->country;
    string city = current->city;
    do {
        system("cls");
        cout << "Исходный аэропорт: \n";
        print_Airport(current);
        cout << "Для редактирования:" << endl;

        cout << endl << " Введите название аэропорта(exit - для выхода): ";
        string name_airport = good_scan(is_valid_word);
        if(name_airport == "exit") break;
        current->name = name_airport;
        //Проверка на существующий аэропорт
        cout << endl << " Введите код аэропорта(exit - для выхода): ";
        string code_airport = good_scan(is_valid_code);
        if(code_airport == "exit") break;
        current->code = code_airport;

        cout << endl << " Введите город аэропорта(exit - для выхода): ";
        string city_airport = good_scan(is_valid_word);
        if(city_airport == "exit") break;
        current->city = city_airport;

        cout << endl << " Введите страну аэропорта(exit - для выхода): ";
        string country_airport = good_scan(is_valid_word);
        if(country_airport == "exit") break;
        current->country = country_airport;
        cout << endl;
        cout << "Аэропорт изменен: " << endl;
        print_Airport(temp);


        cout << "нажмите любую клавишу чтобы изменить аэропорт(esc - для выхода)" << endl;
        c = getch();

    } while (c !=27);
    system("cls");

}
// Возвращает указатель на аэропорт.
// Если пользователь выбрал "Выйти" — возвращает nullptr.
Airport* input_airport(const string& prompt)
{
    cout << prompt;
    string name = good_scan(is_valid_word);
    if (name == "exit") return nullptr;

    Airport* ptr = find_Airport(name);

    while (ptr == nullptr)
    {
        string options[] = { "Создать аэропорт", "Ввести заново", "Выйти в главное меню" };
        string msg = "Аэропорта \"" + name + "\" нет. Что сделать?";
        int choice = choice_menu(options, msg, 3);

        switch (choice)
        {
            case 0: // Создать
                system("cls");
                create_airport_mode();
                ptr = find_Airport(name);
                if (ptr == nullptr) {
                    // пользователь мог создать под другим именем
                    cout << "Введите название созданного аэропорта(exit - для выхода): ";
                    name = good_scan(is_valid_word);
                    if (name == "exit") {
                        system("cls");
                        return nullptr;
                    }
                    ptr = find_Airport(name);
                }
                break;

            case 1: // Ввести заново
                cout << prompt;
                name = good_scan(is_valid_word);
                if (name == "exit") {
                    system("cls");
                    return nullptr;
                }
                ptr = find_Airport(name);
                break;

            case 2: // Выход
                return nullptr;

            default:
                break;
        }
    }

    return ptr;
}


void route_mode(int code) {
    string message = "";
    char c;
    switch (code) {
        case 3: message = "Для создания маршрута вам необходимо ввести данные об аэропортах отправления и назначения"; break; //удаление маршрута
        case 4: message = "При редактировании маршрута вам необходимо ввести данные об аэропортах отправления и назначения и изменить продолжительность полета и расстояние. "; break;//определение прямого маршрута между аэропортами
        case 5: message = "Для удаления маршрута вам необходимо ввести данные об аэропортах отправления и назначения"; break;//определение маршрута между аэропортами с пересадками
        case 6: message = "Для поиска прямого маршрута вам необходимо ввести данные об аэропортах отправления и назначения"; break;
        case 7: message = "Для поиска маршрута с пересадками вам необходимо ввести данные об аэропортах отправления и назначения"; break;
    }
    do{
        system("cls");
        if (!message.empty()) cout << message << endl;

        Airport* dep_ptr;
        Airport* arr_ptr;
        if(code != 8){
            dep_ptr = input_airport("\nВведите название аэропорта отправления(exit - для выхода): ");
            if (dep_ptr == nullptr) {system("cls"); return;}
            arr_ptr = input_airport("\nВведите название аэропорта назначения(exit - для выхода): ");
            if (arr_ptr == nullptr) {system("cls"); return;}
        }
        else{
            arr_ptr = input_airport("\nВведите название аэропорта, аэропорты которого вы хотите вывести(exit - для выхода): ");
            if (arr_ptr == nullptr) {system("cls"); return;}
        }

        switch (code) {
            case 3: create_route_mode(dep_ptr, arr_ptr); break;

            case 4: edit_route(dep_ptr, arr_ptr); break;

            case 5: delete_route(dep_ptr, arr_ptr); break;
                  //удаление маршрута
            case 6: if(is_direct_route(dep_ptr, arr_ptr) == true) break;
                else{} //определение прямого маршрута между аэропортами
            case 7: search_route(dep_ptr, arr_ptr); break;//определение маршрута между аэропортами с пересадками
            case 8: print_routes(arr_ptr); break;
        }
        cout << "Для продолжения выбранного действия нажмите любую клавишу(esc - выход)";
        c = getch();
    }while (c!=27);
    system("cls"); return;
}


int main() {
   
    SetConsoleOutputCP(65001); 
    SetConsoleCP(65001);
    char c;
    do{
        system("cls");
        cout << "Проба пера " << endl;
        c = getch();
        if(c == 27)  break; 
        greeting_menu();
    } while(c != 27);
    return 0;
}
