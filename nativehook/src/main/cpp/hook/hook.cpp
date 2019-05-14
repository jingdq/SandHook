//
// Created by swift on 2019/5/14.
//

#include "hook.h"

#include "../buffer/code_buffer.h"
#include "../../../../../hooklib/src/main/cpp/utils/lock.h"

using namespace SandHook::Hook;
using namespace SandHook::Decoder;
using namespace SandHook::Asm;
using namespace SandHook::Assembler;


#include "assembler_a64.h"
#include "../archs/arm64/relocate/code_relocate_a64.h"
AndroidCodeBuffer* backupBuffer = new AndroidCodeBuffer();
void *InlineHookArm64Android::inlineHook(void *origin, void *replace) {
    AutoLock lock(hookLock);

    void* backup = nullptr;
    AssemblerA64 assemblerBackup(backupBuffer);
    CodeContainer* codeContainerBackup = &assemblerBackup.codeContainer;

    StaticCodeBuffer inlineBuffer = StaticCodeBuffer(reinterpret_cast<Addr>(origin));
    AssemblerA64 assemblerInline(&inlineBuffer);
    CodeContainer* codeContainerInline = &assemblerInline.codeContainer;

    //build inline trampoline
#define __ assemblerInline.
    Label* target_addr_label = new Label();
    __ Ldr(IP1, *target_addr_label);
    __ Br(IP1);
    __ Emit(target_addr_label);
    __ Emit(reinterpret_cast<Addr>(replace));
#undef __

    //build backup method
    CodeRelocateA64 relocate = CodeRelocateA64(assemblerBackup);
    backup = relocate.relocate(origin, codeContainerInline->size(), nullptr);
#define __ assemblerBackup.
    Label* origin_addr_label = new Label();
    __ Ldr(IP1, *origin_addr_label);
    __ Br(IP1);
    __ Emit(origin_addr_label);
    __ Emit(reinterpret_cast<Addr>(origin) + codeContainerInline->size());
    __ finish();
#undef __

    //commit inline trampoline
    assemblerInline.finish();
    return backup;
}
