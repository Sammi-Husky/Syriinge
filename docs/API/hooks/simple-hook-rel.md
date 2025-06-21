# sySimpleHookRel
!!! warning
    Register context of the hooked function is not preserved with this method. Hooks will clobber the volatile registers.

!!! warning
    The overwritten instruction at `address` is not run or preserved.

#### Signature
``` cpp
virtual void sySimpleHookRel(const u32 offset, const void* replacement, int moduleId);
```

#### Parameters

| Parameter      | Type     | Description                       |
| ---------------| -------- | --------------------------------- |
| `address`      | u32      | The address to hook               |
| `replacement`  | void*    | function pointer to the hook body |
| `moduleId`     | int      | The ID of the rel to inject into  |

#### Description
[Relative Hooking](../CoreAPI.md/#static-hooking-vs-rel-hooking) version of [sySimpleHook](./simple-hook.md)

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
    // Inject our hook at offset 0x1fec into the module whose ID matches 0x1b
    api->sySimpleHook(0x1fec, reinterpret_cast<void*>(myFunc), 0x1b);
}
```