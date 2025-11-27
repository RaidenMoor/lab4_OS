#pragma once
#pragma once
#ifndef STACK_DLL_H
#define STACK_DLL_H

#include <windows.h>

#ifdef STACKDLL_EXPORTS
#define STACK_API __declspec(dllexport)
#else
#define STACK_API __declspec(dllimport)
#endif

typedef void* StackHandle;
typedef void* ThreadedStackHandle;

extern "C" {
    STACK_API StackHandle CreateIntStack();
    STACK_API void DeleteIntStack(StackHandle handle);
    STACK_API void PushInt(StackHandle handle, int value);
    STACK_API int PopInt(StackHandle handle);
    STACK_API int PeekInt(StackHandle handle);
    STACK_API bool IsIntStackEmpty(StackHandle handle);
    STACK_API size_t GetIntStackSize(StackHandle handle);
    STACK_API bool SaveIntStackToFile(StackHandle handle, const wchar_t* filename);
    STACK_API bool LoadIntStackFromFile(StackHandle handle, const wchar_t* filename);

    STACK_API ThreadedStackHandle CreateThreadedStack();
    STACK_API void DeleteThreadedStack(ThreadedStackHandle handle);
    STACK_API void ThreadedPush(ThreadedStackHandle handle, int value);
    STACK_API void StopThreadedStack(ThreadedStackHandle handle);
    STACK_API bool IsThreadedStackRunning(ThreadedStackHandle handle);
    STACK_API size_t GetThreadedStackSize(ThreadedStackHandle handle);
    STACK_API bool SaveThreadedStackToFile(ThreadedStackHandle handle, const wchar_t* filename);
}

template <typename T>
class STACK_API Stack {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& value) : data(value), next(nullptr) {}
    };

    Node* top;
    size_t size;

public:
    Stack();
    Stack(const Stack& other);
    ~Stack();

    Stack& operator=(const Stack& other);

    void push(const T& value);
    T pop();
    T& peek() const;
    bool isEmpty() const;
    size_t getSize() const;
    void clear();

    bool saveToFile(const wchar_t* filename) const;
    bool loadFromFile(const wchar_t* filename);

private:
    void copyFrom(const Stack& other);
};

// явное инстанцирование дл€ экспорта
template class STACK_API Stack<int>;
template class STACK_API Stack<double>;

#endif