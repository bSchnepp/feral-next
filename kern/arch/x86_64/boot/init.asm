BITS  32
section .multiboot2
align 4

MULTIBOOT_MAGIC EQU 0xE85250D6
MULTIBOOT_ARCH_ EQU 0x00000000
MULTIBOOT_SIZE_ EQU (header_end - header_start)

extern p2_table
extern p3_table
extern p4_table

; FFFFFFFFC0000000, which is the full P4, but entry 0 for P3, P2.
KERN_VIRT_OFFSET EQU 0xFFFFFF8000000000

header_start:

	dd MULTIBOOT_MAGIC			; We're multiboot2.
	dd MULTIBOOT_ARCH_			; We're on the x86 arch.
	dd MULTIBOOT_SIZE_			; Size of the header.

	; Try to avoid negative numbers in general (they can cause compiler issues in rare, specific cases.)
	dd 0x100000000 - (MULTIBOOT_MAGIC + MULTIBOOT_ARCH_ + MULTIBOOT_SIZE_)

	; Optional Multiboot2 tags should show up here, like so.
	

	
	dw 5	; Type 5
	dw 0	; Flags? Not sure what for here.
	dd 20	; Size of the structure
	dd 80	; 80 chars
	dd 25	; 25 chars
	dd 0	; Ensure we're in text mode.
	dw 0
	dd 8
	
	; Boot services tag (we have a multiboot2-->bare uefi trampoline)
	dw 7
	dw 0
	dd 8

	; This end structure is required by Multiboot.
	dw 0
	dw 0
	dd 8

header_end:


GLOBAL _start
BITS 32

; This is the blue box of death. We'll worry about a useful panic() later.
; The real panic() (KeInternalWarn() for warning, KeStopError() for actual 'panic()') later.
boot_panic:
	mov dword [0xB8000], 0x1F001F00		; Address b8000 is VGA memory. As such, by dumping some data there (1F001F00), we create the "blue box of death".
	mov dword [0xB8004], 0x1F001F00
	mov dword [0xB80A0], 0x1F001F00
	mov dword [0xB80A4], 0x1F001F00
	cli
	hlt
	jmp $
	
boot_panic_invalid_arch:
	mov dword [0xB8000], 0x4F004F00		; If attempting to boot on an IA-32, we'll show the red box of death using the same thing above.
	mov dword [0xB8004], 0x4F004F00
	mov dword [0xB80A0], 0x4F004F00
	mov dword [0xB80A4], 0x4F004F00
	cli
	hlt
	jmp $
	

section .earlydata
gdt_64:
	dq 0	; The zero entry, which is needed for the GDT.

.code: equ $ - gdt_64
	dq (1 << 40) | (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; The GDT needs to be done like this.

.gdtpointer:
	dw $ - gdt_64 - 1
	dq gdt_64	; The pointer that the GDT wants.

section .earlytext

global _start
_start:
	mov esp, stack_top - KERN_VIRT_OFFSET

	; Check for multiboot 2 compliance.
	; This number is the multiboot2 magic number: if this is present, then the kernel was booted correctly.
	; If it was not, then something went wrong, and we'll panic.
	cmp eax, 0x36D76289	; use sub over cmp when we have an excuse for it.
	jne boot_panic
	
	; Push ebx for when we need it later. (holds multiboot struct)
	mov eax, multiboot_value - KERN_VIRT_OFFSET
	mov [eax], ebx

	; Check CPUID...
	pushfd 
	pop eax

	; Use ECX to check the CPUID value.
	mov ecx, eax
	
	; Now we flip the CPUID bit.
	xor eax, 1 << 21

	; Push flags back.
	push eax
	popfd

	; Now pop flags back into eax.
	pushfd
	pop eax

	; And check if the bit was actually flipped.
	cmp eax, ecx
	je boot_panic	; If these are the same, the bit was not flipped.
	
	push ecx
	popfd

	; If we've reached here, that means we were booted by multiboot, and that the hardware supports CPUID.
	; We *could* be lazy and instead assume that the CPU does support CPUID (why would you want this on an 8086???), but we can't assume what CPU this runs on.

	mov eax, 0x80000000	; Long mode argument for CPUID.
	cpuid
	
	cmp eax, 0x80000001	; If this isn't true, we don't support 64-bit long mode, and thus should blue box of death.
	jb boot_panic

	; We now use extended CPUID to check for long mode. The earlier one just checks for if it's too old to possibly support long mode.
	mov eax, 0x80000001
	cpuid

	test edx, 1 << 29      ; Check if we do support long mode (bit 30)
	; If zero, we panic.
	je boot_panic_invalid_arch
	
	; Before moving on, make sure the whole BSS section
	; is zeroed out.
	xor eax, eax
	mov edi, (bss_start - KERN_VIRT_OFFSET)
	mov ecx, (bss_end - KERN_VIRT_OFFSET)
	sub ecx, edi
	cld
	rep stosb


create_page_tables:	
	; Now we need to work on P2.
	; P2 needs to map the first 6MB (should be all we need) with identity mapping, for now.

	; As such, we need 3 entries, and utilize a loop to do so.
	; We'll use ECX for this (we already clobbered it earlier from the checking for CPUID.)
	xor ecx, ecx

	push ebx
.map_page_tables:
	; Use huge pages (2MB), map at 2MB * ecx.
	mov eax, 0x200000
	mul ecx
	or eax, 10000011b	; Present, writable, and huge.
	mov ebx, (p2_table - KERN_VIRT_OFFSET)
	mov [ebx + ecx * 8], eax	; And now write it to the table.

	inc ecx	; Increment it...
	cmp ecx, 512	; P2 needs 512 entries. (one gigabyte of identity mapping.)
	jne .map_page_tables
	pop ebx


; OK, now we need to enable paging.
; Let's start...

enable_paging:
	mov eax, (p4_table - KERN_VIRT_OFFSET)	; Put the address for the p4 table in EAX.
	mov cr3, eax		; Then clobber whatever is in CR3 and move it there.
	
	; We need to enable PAE. (Long mode is a superset of PAE which requires PAE to be enabled.)
	mov eax, cr4
	or eax, (1 << 5)		; Enable the PAE flag in control register 4.
	mov cr4, eax		; And put it back in CR4.

	; We assume EFER is here, because x86_64 just doesn't work without it.
	; Now, we actually enable long mode.
	mov ecx, 0xC0000080
	rdmsr			; Read model specific register (x86-64 EFER)
	or eax, (1 << 8)	; Flip bit 8.
	wrmsr			; And write back to the MSR.

	; Now we finish enable paging.
	mov eax, cr0
	or eax, (1 << 31)	; Just flip bit 31, the bit in binary is LOOONNNGGG.
	or eax, 1
	mov cr0, eax		; And write to CR0.

; In the future, we need to refactor the
; above code to call into these nice small utility
; functions.


; We still need to create a GDT so we can run 64-bit code.
; Let's start...
create_gdt:
	lgdt [gdt_64.gdtpointer]
	; Hold up just a second here. We need to pass to kern_start the multiboot header before it runs off into KiSystemStartup.
	; kern_start is multiboot-only.
	; Get around to that later.


begin_trampoline:
	; invoke far return to go to 64-bit mode.
	push 0x08
	; do a short little dance to ensure we load the right address.
	; We do a call in a sort of odd way: return is just a funny "pop %rip".
	; at the end of the day, so take advantage of that.
	mov eax, dword (kern_start - KERN_VIRT_OFFSET)
	push eax
	retf

	jmp $


SECTION .text
BITS 64
	
kern_start:
	; For now, we need to terminate any other descriptors, as cs is the only one we care about.

	mov ax, 0
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	mov rax, multiboot_value - KERN_VIRT_OFFSET
	mov rdi, [rax]			; Give us the multiboot info we want.
	mov rsp, qword (stack_top)
	and rsp, -16	; Guarantee that we're in fact, aligned correctly.	

	; Force TLB flush (again)
	mov rcx, cr3
	mov cr3, rcx

	extern kern_init
	mov rax, kern_init
	jmp rax
endloop:
	sti ; Force enable interrupts, or else bad things happen!
	hlt
	jmp endloop

section .bss
bss_start:

; Stack overflow will cause a problem in the p4 table.
; FIXME

ALIGN 16
stack_bottom:
	resb 16384	; Nice and big (16KiB) stack.
stack_top:
bss_end:

section .data
; Store our multiboot pointer.
multiboot_value dd 0

; For if loaded in multiboot 2 efi.
efi_image dq 0
efi_systab dq 0
