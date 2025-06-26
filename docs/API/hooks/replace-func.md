# syReplaceFunc
#### Signature
``` cpp
virtual void syReplaceFunc(const u32 address, const void* replacement, void** original);
```

#### Parameters

| Parameter      | Type     | Description                                           |
| ---------------| -------- | ----------------------------------------------------- |
| `address`      | u32      | The address to hook                                   |
| `replacement`  | void*    | Pointer to hook code to inject                        |
| `original`     | void**   | Pointer to a function pointer that will get set to the original unhooked function  |

#### Description
Injects a hook over the first instruction of a function. The `original` parameter will get set to a pointer to the unmodified original function. This is useful for calling the original behavior from within your replacement.

Retuning from your replacement will return to the function caller, not the replaced function.

#### Example Usage

``` cpp

int (*originalFunc)(void);
int myFunc() {
    OSReport("Hello World\n");
    return originalFunc();
}

void Init(CoreAPI* api) {
    // NOTE: Execution does not return to hook location.
    api->syReplaceFunc(0x8000c8b8, 
                       reinterpret_cast<void*>(myFunc), 
                       reinterpret_cast<void**>(originalFunc));
}
```
