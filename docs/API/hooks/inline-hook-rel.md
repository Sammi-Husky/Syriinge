# syInlineHook
#### Signature
``` cpp
virtual void syInlineHookRel(const u32 offset, const void* replacement, int moduleId);
```

#### Paramters

| Parameter      | Type     | Description                       |
| ---------------| -------- | --------------------------------- |
| `address`      | u32      | The address to hook               |
| `replacement`  | void*    | function pointer to the hook body |
| `moduleId`     | int      | The ID of the rel to inject into  |

#### Description
[Relative Hooking](../CoreAPI.md/#static-hooking-vs-rel-hooking) version of [syInlineHook](./inline-hook.md)

#### Example Usage

``` cpp

void sayHello() {
    OSReport("Hello World!\n");
}

void Init(CoreAPI* api) {
    // Inject our hook into the module with ID 0x1b at offset 0x1E00
    api->syInlineHookRel(0x1E00, reinterpret_cast<void*>(sayHello), 0x1b);
}
```