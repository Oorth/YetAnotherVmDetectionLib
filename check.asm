.code

PUBLIC RunVMwareBackdoor


RunVMwareBackdoor PROC

; Save registers
push rbx
push rcx
push rdx


mov eax, 564D5868h			; Load (VMXh) into EAX
mov ecx, 0Ah				; Load Command (Get Version) into ECX
mov dx, 5658h				; Load Port (VX) into DX (Lower 16 bits of RDX)
in eax, dx					; Reads from Port DX into EAX.

; Restore registers
pop rdx
pop rcx
pop rbx

ret
RunVMwareBackdoor ENDP



PUBLIC GetLDT
PUBLIC GetIDT
PUBLIC GetGDT

; Returns the LDT selector (returns in EAX)
GetLDT PROC
    xor rax, rax        ; Clear RAX
    sldt ax             ; Store Local Descriptor Table Register into AX
    ret
GetLDT ENDP

; Stores IDT into the buffer passed in RCX
GetIDT PROC
    sidt [rcx]          ; Store Interrupt Descriptor Table to memory at RCX
    ret
GetIDT ENDP

; Stores GDT into the buffer passed in RCX
GetGDT PROC
    sgdt [rcx]          ; Store Global Descriptor Table to memory at RCX
    ret
GetGDT ENDP


GetAsmInertia PROC

    push rbx

    ; Ensure instruction cache is ready
    xor eax, eax
    cpuid

    ; Start Timer
    lfence              ; Serialize
    rdtsc
    shl rdx, 32
    or rax, rdx         ; RAX = Timestamp
    mov r8, rax         ; Save Start Time in R8

    
    ; Trigger 1: CPUID
    xor eax, eax
    cpuid               ; This modifies EAX, EBX, ECX, EDX

    ; Trigger 2: STR (Store Task Register)
    str ax  

    ; ==========================================================

    rdtscp              ; Wait for triggers to finish
    shl rdx, 32
    or rax, rdx         ; RAX = End Timestamp
    
    sub rax, r8         ; RAX = End - Start

    pop rbx

    ret
GetAsmInertia ENDP


END