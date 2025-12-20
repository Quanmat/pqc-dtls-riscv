INCLUDE generated/output_format.ld
ENTRY(_start)

__DYNAMIC = 0;

INCLUDE generated/regions.ld

/* Default sizes if not defined externally */
_stack_size = 1M;
_heap_size  = 1M;

SECTIONS
{
	/* Text/code section */
	.text :
	{
		_ftext = .;

		/* Startup and ISRs */
		*crt0*(.text)
		KEEP(*crt0*(.text))
		KEEP(*(.text.isr))

		/* General code */
		*(.text .stub .text.* .gnu.linkonce.t.*)

		/* C++ constructors */
		KEEP(*(.init))
		KEEP(*(.init_array .ctors .ctors.*))

		_etext = .;
	} > main_ram

	/* Read-only data section */
	.rodata :
	{
		. = ALIGN(8);
		_frodata = .;
		*(.rodata .rodata.* .srodata .srodata.* .gnu.linkonce.r.*)
		*(.rodata1)
		*(.got .got.*)
		*(.toc .toc.*)
		. = ALIGN(8);
		_erodata = .;
	} > main_ram

	/* Initialized data section */
	.data :
	{
		. = ALIGN(8);
		_fdata = .;
		*(.data .data.* .gnu.linkonce.d.*)
		*(.data1)
		_gp = ALIGN(16);
		*(.sdata .sdata.* .gnu.linkonce.s.*)
		. = ALIGN(8);
		_edata = .;
	} > main_ram

	/* Uninitialized data (BSS) section */
	.bss :
	{
		. = ALIGN(8);
		_fbss = .;
		*(.dynsbss)
		*(.sbss .sbss.* .gnu.linkonce.sb.*)
		*(.scommon)
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(8);
		_ebss = .;
		__end = .;  /* safer symbol than `_end` */
	} > main_ram

	/* Heap (grows upward from end of BSS) */
	._user_heap (NOLOAD) :
	{
		. = ALIGN(8);
		__heap_start = .;
		. = . + _heap_size;
		. = ALIGN(8);
		__heap_end = .;
	} > main_ram

	/* Stack (placed at top of main_ram, growing down) */
	__stack_end = ORIGIN(main_ram) + LENGTH(main_ram);
	__stack_start = __stack_end - _stack_size;

	/* Optional dummy section for debugger symbols */
	._stack (NOLOAD) :
	{
		. = ALIGN(16);
		/* This does not allocate space, but helps for debugger mapping */
	} > main_ram

}

/* Legacy symbols if needed */
PROVIDE(_fstack = __stack_end);
PROVIDE(_fdata_rom = LOADADDR(.data));
PROVIDE(_edata_rom = LOADADDR(.data) + SIZEOF(.data));
