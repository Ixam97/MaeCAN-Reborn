# ----------------------------------------------------------------------------
# Makefile angepasst für den MäCAN Bootloader
# github.com/Ixam97
# ----------------------------------------------------------------------------
# Makefile based on WinAVR Makefile Template written by Eric B. Weddington,
# Jörg Wunsch, et al.
#
#----------------------------------------------------------------------------
# On command line:
# 
# make all:
# Erstellt den Bootloader für alle aktuellen MäCAN-Boards
# 
# make [Board]:
# Kompilliert den Bootloader nur für ein spezielles Board.
# Möglich sind derzeit: BAK-01, MP5x16_V1_0
# ----------------------------------------------------------------------------

# Output format. (can be srec, ihex, binary)
FORMAT = ihex

# Target file name (without extension).
TARGET = Bootloader

# Version of the Bootloader (Legacy = 0).
VERSION = 1

# List C source files here. (C dependencies are automatically generated.)
SRC = $(TARGET).c 				\
	  ../Common/mcan.c			\
	  ../Common/uart.c 			

# List Assembler source files here.
#     Make them always end in a capital .S.  Files ending in a lowercase .s
#     will not be considered source files but generated files (assembler
#     output from the compiler), and will be deleted upon "make clean"!
#     Even though the DOS/Win* filesystem matches both .s and .S the same,
#     it will preserve the spelling of the filenames, and gcc itself does
#     care about how the name is spelled on its command-line.
ASRC =


# Optimization level, can be [0, 1, 2, 3, s].
#     0 = turn off optimization. s = optimize for size.
#     (Note: 3 is not always the best optimization level. See avr-libc FAQ.)
OPT = s


# Debugging format.
#     Native formats for AVR-GCC's -g are dwarf-2 [default] or stabs.
#     AVR Studio 4.10 requires dwarf-2.
#     AVR [Extended] COFF format requires stabs, plus an avr-objcopy run.
DEBUG = dwarf-2


# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
#     Use forward slashes for directory separators.
#     For a directory that has spaces, enclose it in quotes.
EXTRAINCDIRS =


# Compiler flag to set the C Standard level.
#     c89   = "ANSI" C
#     gnu89 = c89 plus GCC extensions
#     c99   = ISO C99 standard (not yet fully implemented)
#     gnu99 = c99 plus GCC extensions
CSTANDARD = -std=gnu99


# Place -D or -U options here
CDEFS = -DF_CPU=$(F_CPU)UL
CDEFS += -DVERSION=$(VERSION)

# uncomment and adapt these line if you want different UART library buffer size
#CDEFS += -DUART_RX_BUFFER_SIZE=128
#CDEFS += -DUART_TX_BUFFER_SIZE=128


# Place -I options here
CINCS =



#---------------- Compiler Options ----------------
#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
CFLAGS = -g$(DEBUG)
CFLAGS += $(CDEFS) $(CINCS)
CFLAGS += -O$(OPT)
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -Wall -Wstrict-prototypes
CFLAGS += -Wa,-adhlns=$(<:.c=.lst)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += $(CSTANDARD)
# Prepare functions for garbage collection
CFLAGS += -ffunction-sections
# Prepare data for garbage collection	
CFLAGS += -fdata-sections
# Pack Structure members together		
CFLAGS += -fpack-struct
# Allocate only as many bytes as needed by enum types	
CFLAGS += -fshort-enums
CFLAGS += -funsigned-char
CFLAGS += -funsigned-bitfields
CFLAGS += -std=gnu99


#---------------- Assembler Options ----------------
#  -Wa,...:   tell GCC to pass this to the assembler.
#  -ahlms:    create listing
#  -gstabs:   have the assembler create line number information; note that
#             for use in COFF files, additional information about filenames
#             and function names needs to be present in the assembler source
#             files -- see avr-libc docs [FIXME: not yet described there]
ASFLAGS = -Wa,-adhlns=$(<:.S=.lst),-gstabs


#---------------- Library Options ----------------
# Minimalistic printf version
PRINTF_LIB_MIN = -Wl,-u,vfprintf -lprintf_min

# Floating point printf version (requires MATH_LIB = -lm below)
PRINTF_LIB_FLOAT = -Wl,-u,vfprintf -lprintf_flt

# If this is left blank, then it will use the Standard printf version.
PRINTF_LIB =
#PRINTF_LIB = $(PRINTF_LIB_MIN)
#PRINTF_LIB = $(PRINTF_LIB_FLOAT)


# Minimalistic scanf version
SCANF_LIB_MIN = -Wl,-u,vfscanf -lscanf_min

# Floating point + %[ scanf version (requires MATH_LIB = -lm below)
SCANF_LIB_FLOAT = -Wl,-u,vfscanf -lscanf_flt

# If this is left blank, then it will use the Standard scanf version.
SCANF_LIB =
#SCANF_LIB = $(SCANF_LIB_MIN)
#SCANF_LIB = $(SCANF_LIB_FLOAT)


MATH_LIB = -lm



#---------------- External Memory Options ----------------

# 64 KB of external RAM, starting after internal RAM (ATmega128!),
# used for variables (.data/.bss) and heap (malloc()).
#EXTMEMOPTS = -Wl,-Tdata=0x801100,--defsym=__heap_end=0x80ffff

# 64 KB of external RAM, starting after internal RAM (ATmega128!),
# only used for heap (malloc()).
#EXTMEMOPTS = -Wl,--defsym=__heap_start=0x801100,--defsym=__heap_end=0x80ffff

EXTMEMOPTS =



#---------------- Linker Options ----------------
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDFLAGS = -Wl,-Map=$(TARGET).map,--cref
# Garbage collect unused sections
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(EXTMEMOPTS)
LDFLAGS += $(PRINTF_LIB) $(SCANF_LIB) $(MATH_LIB)

#============================================================================


# Define programs and commands.
SHELL = sh
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
NM = avr-nm
AVRDUDE = avrdude
REMOVE = rm -f
COPY = cp
WINSHELL = cmd
MAKE = make


# Define Messages
# English
MSG_ERRORS_NONE = Errors: none
MSG_BEGIN = -------- begin --------
MSG_END = --------  end  --------
MSG_SIZE_BEFORE = Size before:
MSG_SIZE_AFTER = Size after:
MSG_COFF = Converting to AVR COFF:
MSG_EXTENDED_COFF = Converting to AVR Extended COFF:
MSG_FLASH = Creating load file for Flash:
MSG_EEPROM = Creating load file for EEPROM:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling:
MSG_ASSEMBLING = Assembling:
MSG_CLEANING = Cleaning up.




# Define all object files.
OBJ = $(SRC:.c=.o) $(ASRC:.S=.o)

# Define all listing files.
LST = $(SRC:.c=.lst) $(ASRC:.S=.lst)


# Compiler flags to generate dependency files.
GENDEPFLAGS = -MD -MP -MF .dep/$(@F).d


# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS) $(GENDEPFLAGS)
ALL_ASFLAGS = -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)





# Default target.
# all: begin gccversion sizebefore build sizeafter end
all: begin gccversion build_all end

compile_msg:
	@echo ------------------------------------------
	@echo Compiling Bootloader for $(BOARD)
	@echo ------------------------------------------
	@echo


build_all:
	@$(MAKE) build_BAK-01
	@$(MAKE) build_MP5x16_V1_0
	@$(MAKE) build_Dx32_1280_V1_0
	@$(MAKE) build_NSGM


# Boardspezifische Werte für MP5x16_V1_0:
build_MP5x16_V1_0: MCU = atmega2560
build_MP5x16_V1_0: F_CPU = 16000000
build_MP5x16_V1_0: BOARD = MP5x16_V1_0
build_MP5x16_V1_0: CDEFS += -DTYPE=0x52
build_MP5x16_V1_0: LDFLAGS += -Wl,--section-start=.text=0x3f000
build_MP5x16_V1_0: compile_msg build clean_list

MP5x16_V1_0: begin gccversion build_MP5x16_V1_0 end

# Boardspezifische Werte BAK-01:
build_BAK-01: MCU = atmega328p
build_BAK-01: F_CPU = 16000000
build_BAK-01: BOARD = BAK-01
build_BAK-01: CDEFS += -DTYPE=0x51
build_BAK-01: LDFLAGS += -Wl,--section-start=.text=0x7000
build_BAK-01: compile_msg build clean_list

BAK-01: begin gccversion build_BAK-01 end

# Boardspezifische Werte NScale Grundmodul (NSGM):
build_NSGM: MCU = atmega328p
build_NSGM: F_CPU = 16000000
build_NSGM: BOARD = NSGM
build_NSGM: CDEFS += -DTYPE=0x70
build_NSGM: LDFLAGS += -Wl,--section-start=.text=0x7000
build_NSGM: compile_msg build clean_list

NSGM: begin gccversion build_NSGM end

# Boardspezifische Werte Dx32(1280):
build_Dx32_1280_V1_0: MCU = atmega1280
build_Dx32_1280_V1_0: F_CPU = 16000000
build_Dx32_1280_V1_0: BOARD = Dx32_1280_V1_0
build_Dx32_1280_V1_0: CDEFS += -DTYPE=0x53
build_Dx32_1280_V1_0: CDEFS += -DSLCAN
build_Dx32_1280_V1_0: LDFLAGS += -Wl,--section-start=.text=0x1e000
build_Dx32_1280_V1_0: compile_msg build clean_list

Dx32_1280_V1_0: begin gccversion build_Dx32_1280_V1_0 end

# build: elf hex eep lss sym
build: hex
	@$(AVRSIZE)
	@echo Copying hex-file to ./hex/$(TARGET)_v$(VERSION)_$(BOARD)_$(MCU).hex
	@$(COPY) $(TARGET).hex ./hex/$(TARGET)_v$(VERSION)_$(BOARD)_$(MCU).hex

elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss
sym: $(TARGET).sym



# Eye candy.
# AVR Studio 3.x does not check make's exit code but relies on
# the following magic strings to be generated by the compile job.
begin:
	@echo
	@echo $(MSG_BEGIN)

end:
	@echo $(MSG_END)
	@echo


# Display size of file.
AVRSIZE = $(SIZE) --format=avr --target=$(FORMAT) --mcu=$(MCU) $(TARGET).elf

# Display compiler version information.
gccversion :
	@$(CC) --version

# Generate avr-gdb config/init file which does the following:
#     define the reset signal, load the target file, connect to target, and set
#     a breakpoint at main().
gdb-config:
	@$(REMOVE) $(GDBINIT_FILE)
	@echo define reset >> $(GDBINIT_FILE)
	@echo SIGNAL SIGHUP >> $(GDBINIT_FILE)
	@echo end >> $(GDBINIT_FILE)
	@echo file $(TARGET).elf >> $(GDBINIT_FILE)
	@echo target remote $(DEBUG_HOST):$(DEBUG_PORT)  >> $(GDBINIT_FILE)
ifeq ($(DEBUG_BACKEND),simulavr)
	@echo load  >> $(GDBINIT_FILE)
endif
	@echo break main >> $(GDBINIT_FILE)

coff: $(TARGET).elf
	@echo
	@echo $(MSG_COFF) $(TARGET).cof
	@$(COFFCONVERT) -O coff-avr $< $(TARGET).cof


extcoff: $(TARGET).elf
	@echo
	@echo $(MSG_EXTENDED_COFF) $(TARGET).cof
	@$(COFFCONVERT) -O coff-ext-avr $< $(TARGET).cof



# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
	@echo $(MSG_FLASH) $@
	@$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

%.eep: %.elf
	@echo $(MSG_EEPROM) $@
	@-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Create extended listing file from ELF output file.
%.lss: %.elf
	@echo $(MSG_EXTENDED_LISTING) $@
	@$(OBJDUMP) -h -S $< > $@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo $(MSG_SYMBOL_TABLE) $@
	@$(NM) -n $< > $@



# Link: create ELF output file from object files.
.SECONDARY : $(TARGET).elf
.PRECIOUS : $(OBJ)
%.elf: $(OBJ)
	@echo $(MSG_LINKING) $@
	@$(CC) $(ALL_CFLAGS) $^ --output $@ $(LDFLAGS)


# Compile: create object files from C source files.
%.o : %.c
	@echo $(MSG_COMPILING) $<
	@$(CC) -c $(ALL_CFLAGS) $< -o $@


# Compile: create assembler files from C source files.
%.s : %.c
	@$(CC) -S $(ALL_CFLAGS) $< -o $@


# Assemble: create object files from assembler source files.
%.o : %.S
	@echo $(MSG_ASSEMBLING) $<
	@$(CC) -c $(ALL_ASFLAGS) $< -o $@

# Create preprocessed source for use in sending a bug report.
%.i : %.c
	@$(CC) -E -mmcu=$(MCU) -I. $(CFLAGS) $< -o $@


# Target: clean project.
clean: begin clean_list end

clean_list :
	@echo $(MSG_CLEANING)
	@$(REMOVE) $(TARGET).hex
	@$(REMOVE) $(TARGET).eep
	@$(REMOVE) $(TARGET).cof
	@$(REMOVE) $(TARGET).elf
	@$(REMOVE) $(TARGET).map
	@$(REMOVE) $(TARGET).sym
	@$(REMOVE) $(TARGET).lss
	@$(REMOVE) $(OBJ)
	@$(REMOVE) $(LST)
	@$(REMOVE) $(SRC:.c=.s)
	@$(REMOVE) $(SRC:.c=.d)
	@$(REMOVE) .dep/*



# Include the dependency files.
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)


# Listing of phony targets.
.PHONY : all begin finish end sizebefore sizeafter gccversion \
build elf hex eep lss sym coff extcoff \
clean clean_list program debug gdb-config