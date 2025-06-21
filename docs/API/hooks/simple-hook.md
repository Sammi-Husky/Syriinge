# sySimpleHook
!!! warning
    Register context of the hooked function is not preserved with this method. Hooks will clobber the volatile registers.

!!! warning
    The overwritten instruction at `address` is not run or preserved.

#### Signature
``` cpp
virtual void sySimpleHook(const u32 address, const void* replacement);
```

#### Parameters

| Parameter      | Type     | Description                       |
| ---------------| -------- | --------------------------------- |
| `address`      | u32      | The address to hook               |
| `replacement`  | void*    | function pointer to the hook body |

#### Description
Injects a hook at the target address. Hooks injected via this method will ***NOT*** return execution to the original function by default. 

This hook type is commonly used for raw asm functions.

#### Example Usage

``` cpp

asm void myFunc() {
    // Don't need a stack frame.
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