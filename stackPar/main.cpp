#include <iostream>
#include <windows.h>
#include <vector>
#include <thread>
#include <chrono>

using namespace std;

typedef void* StackHandle;
typedef void* ThreadedStackHandle;

extern "C" {
 
    typedef StackHandle(__cdecl* CreateIntStackFunc)();
    typedef void(__cdecl* DeleteIntStackFunc)(StackHandle);
    typedef void(__cdecl* PushIntFunc)(StackHandle, int);
    typedef int(__cdecl* PopIntFunc)(StackHandle);
    typedef int(__cdecl* PeekIntFunc)(StackHandle);
    typedef bool(__cdecl* IsEmptyIntFunc)(StackHandle);
    typedef size_t(__cdecl* GetSizeIntFunc)(StackHandle);
    typedef bool(__cdecl* SaveIntToFileFunc)(StackHandle, const wchar_t*);
    typedef bool(__cdecl* LoadIntFromFileFunc)(StackHandle, const wchar_t*);

    typedef ThreadedStackHandle(__cdecl* CreateThreadedStackFunc)();
    typedef void(__cdecl* DeleteThreadedStackFunc)(ThreadedStackHandle);
    typedef void(__cdecl* ThreadedPushFunc)(ThreadedStackHandle, int);
    typedef void(__cdecl* StopThreadedStackFunc)(ThreadedStackHandle);
    typedef bool(__cdecl* IsThreadedStackRunningFunc)(ThreadedStackHandle);
    typedef size_t(__cdecl* GetThreadedStackSizeFunc)(ThreadedStackHandle);
    typedef bool(__cdecl* SaveThreadedStackToFileFunc)(ThreadedStackHandle, const wchar_t*);
}

void testExplicitLinking() {
    cout << "Явное подключение DLL" << endl;

    HMODULE hDll = LoadLibraryW(L"stackDLL.dll");
    if (!hDll) {
        cout << "Ошибка загрузки DLL!" << endl;
        return;
    }

    CreateThreadedStackFunc createThreadedStack = (CreateThreadedStackFunc)GetProcAddress(hDll, "CreateThreadedStack");
    DeleteThreadedStackFunc deleteThreadedStack = (DeleteThreadedStackFunc)GetProcAddress(hDll, "DeleteThreadedStack");
    ThreadedPushFunc threadedPush = (ThreadedPushFunc)GetProcAddress(hDll, "ThreadedPush");
    StopThreadedStackFunc stopThreadedStack = (StopThreadedStackFunc)GetProcAddress(hDll, "StopThreadedStack");
    IsThreadedStackRunningFunc isThreadedStackRunning = (IsThreadedStackRunningFunc)GetProcAddress(hDll, "IsThreadedStackRunning");
    GetThreadedStackSizeFunc getThreadedStackSize = (GetThreadedStackSizeFunc)GetProcAddress(hDll, "GetThreadedStackSize");
    SaveThreadedStackToFileFunc saveThreadedStackToFile = (SaveThreadedStackToFileFunc)GetProcAddress(hDll, "SaveThreadedStackToFile");

    if (!createThreadedStack || !deleteThreadedStack || !threadedPush ||
        !stopThreadedStack || !isThreadedStackRunning || !getThreadedStackSize || !saveThreadedStackToFile) {
        cout << "Ошибка получения многопоточных функций из DLL!" << endl;
        FreeLibrary(hDll);
        return;
    }

    cout << "\nТестирование многопоточного стека" << endl;

    ThreadedStackHandle threadedStack = createThreadedStack();
    if (!threadedStack) {
        cout << "Ошибка создания многопоточного стека!" << endl;
        FreeLibrary(hDll);
        return;
    }

    cout << "Многопоточный стек создан" << endl;
    cout << "Поток-потребитель активен: " << (isThreadedStackRunning(threadedStack) ? "Да" : "Нет") << endl;

    cout << "\nДобавляем элементы через многопоточный интерфейс:" << endl;
    for (int i = 1; i <= 5; i++) {
        cout << "[Main] Отправка значения: " << i * 10 << endl;
        threadedPush(threadedStack, i * 10);
        this_thread::sleep_for(chrono::milliseconds(100)); 
    }

    cout << "\nРазмер стека после добавления: " << getThreadedStackSize(threadedStack) << endl;

    if (saveThreadedStackToFile(threadedStack, L"final_threaded_stack.bin")) {
        cout << "Финальное состояние сохранено в 'final_threaded_stack.bin'" << endl;
    }

    cout << "\nОстанавливаем многопоточный стек..." << endl;
    stopThreadedStack(threadedStack);

    int waitCount = 0;
    while (isThreadedStackRunning(threadedStack) && waitCount < 10) {
        cout << "Ожидание завершения потока-потребителя..." << endl;
        this_thread::sleep_for(chrono::milliseconds(100));
        waitCount++;
    }

    cout << "Поток-потребитель завершен: " << (!isThreadedStackRunning(threadedStack) ? "Да" : "Нет") << endl;

    deleteThreadedStack(threadedStack);
    cout << "Многопоточный стек удален" << endl;

    cout << "\nПроверка сохраненных данных" << endl;

    CreateIntStackFunc createCheckStack = (CreateIntStackFunc)GetProcAddress(hDll, "CreateIntStack");
    LoadIntFromFileFunc loadFromFile = (LoadIntFromFileFunc)GetProcAddress(hDll, "LoadIntStackFromFile");
    GetSizeIntFunc getCheckSize = (GetSizeIntFunc)GetProcAddress(hDll, "GetIntStackSize");
    PopIntFunc popCheck = (PopIntFunc)GetProcAddress(hDll, "PopInt");
    DeleteIntStackFunc deleteCheckStack = (DeleteIntStackFunc)GetProcAddress(hDll, "DeleteIntStack");
    IsEmptyIntFunc isEmptyCheck = (IsEmptyIntFunc)GetProcAddress(hDll, "IsIntStackEmpty");

    if (createCheckStack && loadFromFile && getCheckSize && popCheck && deleteCheckStack && isEmptyCheck) {
        StackHandle checkStack = createCheckStack();
        if (loadFromFile(checkStack, L"final_threaded_stack.bin")) {
            cout << "Данные успешно загружены из файла" << endl;
            cout << "Количество элементов: " << getCheckSize(checkStack) << endl;

            cout << "Элементы в стеке (в порядке извлечения):" << endl;
            while (!isEmptyCheck(checkStack)) {
                int value = popCheck(checkStack);
                cout << " - " << value << endl;
            }
        }
        else {
            cout << "Ошибка загрузки данных из файла!" << endl;
        }
        deleteCheckStack(checkStack);
    }

    FreeLibrary(hDll);
    cout << "Тестирование завершено" << endl;
}

void testBasicStack() {
    cout << "\nТестирование базового стека" << endl;

    HMODULE hDll = LoadLibraryW(L"stackDLL.dll");
    if (!hDll) {
        cout << "Ошибка загрузки DLL!" << endl;
        return;
    }

    CreateIntStackFunc createStack = (CreateIntStackFunc)GetProcAddress(hDll, "CreateIntStack");
    DeleteIntStackFunc deleteStack = (DeleteIntStackFunc)GetProcAddress(hDll, "DeleteIntStack");
    PushIntFunc pushFunc = (PushIntFunc)GetProcAddress(hDll, "PushInt");
    PopIntFunc popFunc = (PopIntFunc)GetProcAddress(hDll, "PopInt");
    PeekIntFunc peekFunc = (PeekIntFunc)GetProcAddress(hDll, "PeekInt");
    GetSizeIntFunc getSizeFunc = (GetSizeIntFunc)GetProcAddress(hDll, "GetIntStackSize");
    SaveIntToFileFunc saveFunc = (SaveIntToFileFunc)GetProcAddress(hDll, "SaveIntStackToFile");
    IsEmptyIntFunc isEmptyFunc = (IsEmptyIntFunc)GetProcAddress(hDll, "IsIntStackEmpty");

    if (!createStack || !deleteStack || !pushFunc || !popFunc || !peekFunc || !getSizeFunc || !saveFunc || !isEmptyFunc) {
        cout << "Ошибка получения функций из DLL!" << endl;
        FreeLibrary(hDll);
        return;
    }

    StackHandle stack = createStack();

    cout << "Добавляем элементы: 1, 2, 3" << endl;
    pushFunc(stack, 1);
    pushFunc(stack, 2);
    pushFunc(stack, 3);

    cout << "Размер стека: " << getSizeFunc(stack) << endl;
    cout << "Верхний элемент: " << peekFunc(stack) << endl;

    cout << "Извлечение элементов: ";
    while (!isEmptyFunc(stack)) {
        cout << popFunc(stack) << " ";
    }
    cout << endl;

    cout << "Стек пустой: " << (isEmptyFunc(stack) ? "ДА" : "НЕТ") << endl;

    deleteStack(stack);
    FreeLibrary(hDll);

    cout << "Тестирование завершено" << endl;
}


int main() {
    setlocale(LC_ALL, "Russian");
    cout << "Лабораторная работа по дисциплине \"Операционные системы\"" << endl;
    cout << "Выполнила студентка группы 6401-020302D Гусева Мария" << endl;
    cout << "Вариант №7: реализовать стек на основе связного списка" << endl;
    cout << "Многопоточная версия с производителем-потребителем" << endl;

    //базовый стек
    testBasicStack();

    //многопоточный стек
    testExplicitLinking();

    cout << "\nНажмите Enter для выхода..." << endl;
    cin.get();

    return 0;
}