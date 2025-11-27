#include "pch.h"

#include "stack_dll.h"


#include <stdexcept>
#include <iostream>
#include <queue>

using namespace std;

struct ThreadedStackData {
    Stack<int>* stack;
    HANDLE hConsumerThread;
    HANDLE hDataReadyEvent;    // Событие "данные готовы"
    HANDLE hDataProcessedEvent; // Событие "данные обработаны"
    HANDLE hStopEvent;         // Событие "остановка"
    int pendingValue;          // Буфер для передаваемого значения
    bool hasPendingData;       // Флаг наличия данных в буфере
    CRITICAL_SECTION cs;       // Критическая секция для синхронизации
};

DWORD WINAPI ConsumerThread(LPVOID lpParam) {
    ThreadedStackData* data = static_cast<ThreadedStackData*>(lpParam);

    while (true) {
        HANDLE handles[2] = { data->hDataReadyEvent, data->hStopEvent };
        DWORD result = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

        if (result == WAIT_OBJECT_0 + 1) { // Остановка
            break;
        }

        if (result == WAIT_OBJECT_0) { // Данные готовы
           
            EnterCriticalSection(&data->cs);

            if (data->hasPendingData) {
               
                data->stack->push(data->pendingValue);
                data->hasPendingData = false;

               
                wchar_t filename[256];
                swprintf(filename, 256, L"threaded_stack_%d.bin", GetCurrentThreadId());
                data->stack->saveToFile(filename);

                cout << "[Consumer] Added value: " << data->pendingValue
                    << ", Stack size: " << data->stack->getSize()
                    << ", Saved to: " << filename << endl;
            }

            LeaveCriticalSection(&data->cs);

           
            SetEvent(data->hDataProcessedEvent);
        }
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule); // Оптимизация
        break;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

extern "C" {
    STACK_API StackHandle CreateIntStack() {
        return new Stack<int>();
    }

    STACK_API void DeleteIntStack(StackHandle handle) {
        delete static_cast<Stack<int>*>(handle);
    }

    STACK_API void PushInt(StackHandle handle, int value) {
        static_cast<Stack<int>*>(handle)->push(value);
    }

    STACK_API int PopInt(StackHandle handle) {
        return static_cast<Stack<int>*>(handle)->pop();
    }

    STACK_API int PeekInt(StackHandle handle) {
        return static_cast<Stack<int>*>(handle)->peek();
    }

    STACK_API bool IsIntStackEmpty(StackHandle handle) {
        return static_cast<Stack<int>*>(handle)->isEmpty();
    }

    STACK_API size_t GetIntStackSize(StackHandle handle) {
        return static_cast<Stack<int>*>(handle)->getSize();
    }

    STACK_API bool SaveIntStackToFile(StackHandle handle, const wchar_t* filename) {
        return static_cast<Stack<int>*>(handle)->saveToFile(filename);
    }

    STACK_API bool LoadIntStackFromFile(StackHandle handle, const wchar_t* filename) {
        return static_cast<Stack<int>*>(handle)->loadFromFile(filename);
    }
}

extern "C" {
    STACK_API ThreadedStackHandle CreateThreadedStack() {
        ThreadedStackData* data = new ThreadedStackData();

        
        data->stack = new Stack<int>();
        data->hasPendingData = false;

        // Создание событий
        data->hDataReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);    
        data->hDataProcessedEvent = CreateEvent(NULL, FALSE, FALSE, NULL); 
        data->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);          

        // Инициализация критической секции
        InitializeCriticalSection(&data->cs);

        // Создание потока-потребителя
        data->hConsumerThread = CreateThread(NULL, 0, ConsumerThread, data, 0, NULL);

        if (!data->hConsumerThread) {
            DeleteThreadedStack(data);
            return nullptr;
        }

        return data;
    }

    STACK_API void DeleteThreadedStack(ThreadedStackHandle handle) {
        ThreadedStackData* data = static_cast<ThreadedStackData*>(handle);

        if (data) {
            StopThreadedStack(handle);

            if (data->hConsumerThread) {
                WaitForSingleObject(data->hConsumerThread, INFINITE);
                CloseHandle(data->hConsumerThread);
            }

            if (data->hDataReadyEvent) CloseHandle(data->hDataReadyEvent);
            if (data->hDataProcessedEvent) CloseHandle(data->hDataProcessedEvent);
            if (data->hStopEvent) CloseHandle(data->hStopEvent);

            DeleteCriticalSection(&data->cs);

            if (data->stack) delete data->stack;

            delete data;
        }
    }

    STACK_API void ThreadedPush(ThreadedStackHandle handle, int value) {
        ThreadedStackData* data = static_cast<ThreadedStackData*>(handle);

        if (!data || !data->hConsumerThread) return;

        EnterCriticalSection(&data->cs);

        data->pendingValue = value;
        data->hasPendingData = true;

        LeaveCriticalSection(&data->cs);

        SetEvent(data->hDataReadyEvent);

        WaitForSingleObject(data->hDataProcessedEvent, INFINITE);
    }

    STACK_API void StopThreadedStack(ThreadedStackHandle handle) {
        ThreadedStackData* data = static_cast<ThreadedStackData*>(handle);

        if (data && data->hStopEvent) {
            SetEvent(data->hStopEvent);
        }
    }

    STACK_API bool IsThreadedStackRunning(ThreadedStackHandle handle) {
        ThreadedStackData* data = static_cast<ThreadedStackData*>(handle);

        if (!data || !data->hConsumerThread) return false;

        DWORD exitCode;
        GetExitCodeThread(data->hConsumerThread, &exitCode);
        return exitCode == STILL_ACTIVE;
    }

    STACK_API size_t GetThreadedStackSize(ThreadedStackHandle handle) {
        ThreadedStackData* data = static_cast<ThreadedStackData*>(handle);

        if (!data || !data->stack) return 0;

        EnterCriticalSection(&data->cs);
        size_t size = data->stack->getSize();
        LeaveCriticalSection(&data->cs);

        return size;
    }

    STACK_API bool SaveThreadedStackToFile(ThreadedStackHandle handle, const wchar_t* filename) {
        ThreadedStackData* data = static_cast<ThreadedStackData*>(handle);

        if (!data || !data->stack) return false;

        EnterCriticalSection(&data->cs);
        bool result = data->stack->saveToFile(filename);
        LeaveCriticalSection(&data->cs);

        return result;
    }
}

template <typename T>
Stack<T>::Stack() : top(nullptr), size(0) {}

template <typename T>
Stack<T>::~Stack() {
    clear();
}

template <typename T>
Stack<T>::Stack(const Stack& other) : top(nullptr), size(0) {
    copyFrom(other);
}

template <typename T>
Stack<T>& Stack<T>::operator=(const Stack& other) {
    if (this != &other) {
        clear();
        copyFrom(other);
    }
    return *this;
}

template <typename T>
void Stack<T>::copyFrom(const Stack& other) {
    if (other.isEmpty()) {
        return;
    }

    Stack<T> temp;
    Node* current = other.top;
    while (current != nullptr) {
        temp.push(current->data);
        current = current->next;
    }

    while (!temp.isEmpty()) {
        push(temp.pop());
    }
}

template <typename T>
void Stack<T>::push(const T& value) {
    Node* newNode = new Node(value);
    newNode->next = top;
    top = newNode;
    size++;
}

template <typename T>
T Stack<T>::pop() {
    if (isEmpty()) {
        throw runtime_error("Стэк пустой");
    }

    Node* temp = top;
    T value = top->data;
    top = top->next;
    delete temp;
    size--;

    return value;
}

template <typename T>
T& Stack<T>::peek() const {
    if (isEmpty()) {
        throw runtime_error("Стэк пустой");
    }
    return top->data;
}

template <typename T>
bool Stack<T>::isEmpty() const {
    return top == nullptr;
}

template <typename T>
size_t Stack<T>::getSize() const {
    return size;
}

template <typename T>
void Stack<T>::clear() {
    while (!isEmpty()) {
        pop();
    }
}

template <typename T>
bool Stack<T>::saveToFile(const wchar_t* filename) const {
    // Используем CreateFileW для Unicode строк
    HANDLE hFile = CreateFileW(
        filename,           // LPCWSTR - wide string
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesWritten;
    if (!WriteFile(hFile, &size, sizeof(size_t), &bytesWritten, NULL)) {
        CloseHandle(hFile);
        return false;
    }

    Stack<T> tempStack = *this;
    Stack<T> reverseStack;

    while (!tempStack.isEmpty()) {
        reverseStack.push(tempStack.pop());
    }

    while (!reverseStack.isEmpty()) {
        T value = reverseStack.pop();
        if (!WriteFile(hFile, &value, sizeof(T), &bytesWritten, NULL)) {
            CloseHandle(hFile);
            return false;
        }
    }

    CloseHandle(hFile);
    return true;
}

template <typename T>
bool Stack<T>::loadFromFile(const wchar_t* filename) {
    HANDLE hFile = CreateFileW(
        filename,        
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    clear();

    DWORD bytesRead;
    size_t fileSize;
    if (!ReadFile(hFile, &fileSize, sizeof(size_t), &bytesRead, NULL)) {
        CloseHandle(hFile);
        return false;
    }

    for (size_t i = 0; i < fileSize; i++) {
        T value;
        if (!ReadFile(hFile, &value, sizeof(T), &bytesRead, NULL)) {
            CloseHandle(hFile);
            clear();
            return false;
        }
        push(value);
    }

    CloseHandle(hFile);
    return true;
}

template class Stack<int>;
template class Stack<double>;