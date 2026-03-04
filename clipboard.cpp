#include "../include/interpreter.h"
#include <vector>
#include "../include/module_registry.h"
#include <cstring>
#include <stdexcept>
#include <string>
#include <windows.h>

namespace clipboard_lib {

std::string get() {
    if (!OpenClipboard(nullptr)) {
        throw std::runtime_error("clipboard.get[] failed to open clipboard");
    }

    HANDLE data = GetClipboardData(CF_TEXT);
    if (data == nullptr) {
        CloseClipboard();
        return "";
    }

    char* text = static_cast<char*>(GlobalLock(data));
    if (text == nullptr) {
        CloseClipboard();
        throw std::runtime_error("clipboard.get[] failed to lock clipboard data");
    }

    std::string out(text);
    GlobalUnlock(data);
    CloseClipboard();
    return out;
}

void set(const std::string& value) {
    if (!OpenClipboard(nullptr)) {
        throw std::runtime_error("clipboard.set[] failed to open clipboard");
    }

    if (!EmptyClipboard()) {
        CloseClipboard();
        throw std::runtime_error("clipboard.set[] failed to clear clipboard");
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, value.size() + 1);
    if (hMem == nullptr) {
        CloseClipboard();
        throw std::runtime_error("clipboard.set[] failed to allocate memory");
    }

    void* ptr = GlobalLock(hMem);
    if (ptr == nullptr) {
        GlobalFree(hMem);
        CloseClipboard();
        throw std::runtime_error("clipboard.set[] failed to lock memory");
    }

    memcpy(ptr, value.c_str(), value.size() + 1);
    GlobalUnlock(hMem);

    if (SetClipboardData(CF_TEXT, hMem) == nullptr) {
        GlobalFree(hMem);
        CloseClipboard();
        throw std::runtime_error("clipboard.set[] failed to set clipboard data");
    }

    CloseClipboard();
}

void clear() {
    if (!OpenClipboard(nullptr)) {
        throw std::runtime_error("clipboard.clear[] failed to open clipboard");
    }
    if (!EmptyClipboard()) {
        CloseClipboard();
        throw std::runtime_error("clipboard.clear[] failed to clear clipboard");
    }
    CloseClipboard();
}

} // namespace clipboard_lib

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("clipboard", [](Interpreter& interp) {
                    interp.registerModuleFunction("clipboard", "get", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "clipboard.get");
                        return Value::fromString(clipboard_lib::get());
                    });
                    interp.registerModuleFunction("clipboard", "set", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "clipboard.set");
                        clipboard_lib::set(interp.expectString(args[0], "clipboard.set expects string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("clipboard", "clear", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "clipboard.clear");
                        clipboard_lib::clear();
                        return Value::fromNumber(0.0);
                    });

    });
}
