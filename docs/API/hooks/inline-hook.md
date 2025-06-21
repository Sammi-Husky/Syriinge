# syInlineHook
#### Signature
``` cpp
virtual void syInlineHook(const u32 address, const void* replacement);
```

#### Paramters

| Parameter      | Type     | Description                       |
| ---------------| -------- | --------------------------------- |
| `address`      | u32      | The address to hook               |
| `replacement`  | void*    | function pointer to the hook body |

#### Description
Injects a hook at the target address. Hooks injected via this method ***WILL*** return execution to the original function.

#### Example Usage

``` cpp

void sayHello() {
    OSReport("Hello World!\n");
}

void Init(CoreAPI* api) {
    api->syInlineHook(0x8000c8b8, reinterpret_cast<void*>(sayHello));
}
```