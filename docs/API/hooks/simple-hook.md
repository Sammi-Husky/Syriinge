# sySimpleHook
!!! warning
    Hooks registered with this function will not create a stack frame. You must be careful about register usage to avoid crashes.
#### Signature
``` cpp
virtual void sySimpleHook(const u32 address, const void* replacement);
```

#### Paramters

| Parameter      | Type     | Description                       |
| ---------------| -------- | --------------------------------- |
| `address`      | u32      | The address to hook               |
| `replacement`  | void*    | function pointer to the hook body |

#### Description
Injects a hook at the target address. Hooks injected via this method will ***NOT*** return execution to the original function and no stack frame is created.

This hook type is mostly used for raw asm functions.

#### Example Usage

``` cpp

asm void myFunc() {
    // We're using simple hook don't create a stack frame 
    // otherwise it would overwrite the caller's frame
    nofralloc

    // Return 0
    li r3, 0
    blr
}

void Init(CoreAPI* api) {
    // NOTE: Execution does not return to hook location
    api->sySimpleHook(0x8000c8b8, reinterpret_cast<void*>(myFunc));
}
```