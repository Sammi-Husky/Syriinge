# syReplaceFunc
#### Signature
``` cpp
virtual void syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId);
```

#### Paramters

| Parameter      | Type     | Description                                           |
| ---------------| -------- | ----------------------------------------------------- |
| `address`      | u32      | The address to hook                                   |
| `replacement`  | void*    | Pointer to hook code to inject                        |
| `original`     | void**   | Pointer to a function pointer that will get set to the original unhooked function  |
| `moduleId`     | int      | The ID of the rel to inject into  |

#### Description
[Relative Hooking](../CoreAPI.md/#static-hooking-vs-rel-hooking) version of [syReplaceFunc](./replace-func.md)

#### Example Usage

``` cpp

void (*originalFunc)(void);
int myFunc() {
    OSReport("Hello World\n");
    return originalFunc();
}

void Init(CoreAPI* api) {
    // NOTE: Execution does not return to hook location.
    // Inject our hook at offset 0x1DFC in the rel that's ID is 0x1b
    api->syReplaceFunc(0x1DFC, 
                       reinterpret_cast<void*>(myFunc), 
                       reinterpret_cast<void**>(originalFunc),
                       0x1b);
}
```