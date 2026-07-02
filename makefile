#	Universal Tiny Project Makefile - avr
#	
#	Copyright (C) 2026 Marcos Rubiano
#		email: markusianito@proton.me
#
#	This file is licensed under
#	BSD-2-clause
#
#


CC = avr-gcc
CXX = avr-g++
AR = avr-ar
OBJCOPY = avr-objcopy
SIZE = avr-size
AVRDUDE = avrdude


GREEN  = \033[32m
CYAN   = \033[36m
YELLOW = \033[33m
RED    = \033[31m
RESET  = \033[0m
BUILD_MSG = Compiling
LINK_MSG = Linking

ifeq (, $(shell which $(CC)))
$(error "$(CC) is not installed or not @ PATH")
endif
ifeq (, $(shell which $(CXX)))
$(error "$(CXX) is not installed or not @ PATH")
endif


# =========================
# Selectable settings
# =========================

mcu ?= atmega32u4
fcpu ?= 16000000UL

method ?= 0
port ?= 0
baud ?= 0



# Selección automática de variant
ifeq ($(mcu),atmega328p)
VARIANT = standard
method_def = arduino
baud_def = 115200
else ifeq ($(mcu),atmega32u4)
VARIANT = leonardo
method_def = avr109
baud_def = 57600
else
$(error That is not a valid "mcu=")
endif


# =========================
# Tools
# =========================

TARGET = firmware
SOURCE_DIR = src
BUILD_DIR = build
TARGET_DIR = $(BUILD_DIR)/binaries
CORE_BUILD_DIR = $(BUILD_DIR)/core
CORE_DIR = core/cores/arduino
CORE_LIBRARIES_DIR = core/libraries
VARIANT_DIR = core/variants/$(VARIANT)
TOOLS_DIR = tools

STD1_TARGET = standalone

# =========================
# Fuentes principales
# =========================


CORE_SRC_C = $(wildcard $(CORE_DIR)/*.c)
CORE_SRC_CPP = $(wildcard $(CORE_DIR)/*.cpp)
CORE_SRC_S = $(wildcard $(CORE_DIR)/*.S)

CORE_OBJ_C = $(patsubst $(CORE_DIR)/%.c,$(CORE_BUILD_DIR)/%.o,$(CORE_SRC_C))
CORE_OBJ_CPP = $(patsubst $(CORE_DIR)/%.cpp,$(CORE_BUILD_DIR)/%++.o,$(CORE_SRC_CPP))
CORE_OBJ_S = $(patsubst $(CORE_DIR)/%.S,$(CORE_BUILD_DIR)/%.o,$(CORE_SRC_S))

CORE_OBJS = $(CORE_OBJ_C) $(CORE_OBJ_CPP) $(CORE_OBJ_S)
CORE_LIB = $(BUILD_DIR)/libcore.a

SRC_C = $(wildcard $(SOURCE_DIR)/*.c) $(wildcard $(SOURCE_DIR)/**/*.c)
SRC_CPP = $(wildcard $(SOURCE_DIR)/*.cpp) $(wildcard $(SOURCE_DIR)/**/*.cpp)

OBJ_C = $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_C))
OBJ_CPP = $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_DIR)/%++.o,$(SRC_CPP))

MAIN_OBJS = $(OBJ_C) $(OBJ_CPP)


# =========================
# Librerías
# =========================

CORE_LIB_DIRS = $(wildcard $(CORE_LIBRARIES_DIR)/*)
LIB_DIRS = $(wildcard lib/*)

CORE_LIB_NAMES = $(notdir $(CORE_LIB_DIRS))
LIB_NAMES = $(notdir $(LIB_DIRS))

CORE_LIB_ARCHIVES = $(patsubst %,$(CORE_BUILD_DIR)/libraries/lib%.a,$(CORE_LIB_NAMES))
LIB_ARCHIVES = $(patsubst %,$(BUILD_DIR)/lib%.a,$(LIB_NAMES))

CORE_LIB_INCLUDE_FLAGS = $(foreach clib,$(CORE_LIB_NAMES),-Icore/libraries/$(clib)/src)
LIB_INCLUDE_FLAGS = $(foreach lib,$(LIB_NAMES),-Ilib/$(lib)/src)



# =========================
# Flags
# =========================

CORE_FLAGS = -mmcu=$(mcu) \
			-DF_CPU=$(fcpu) \
			-ffunction-sections \
			-fdata-sections \
			-DARDUINO_ARCH_AVR \
			-DARDUINO=10819 \
			-I$(CORE_DIR) \
			-I$(VARIANT_DIR) \
			  $(CORE_LIB_INCLUDE_FLAGS) \
			  $(LIB_INCLUDE_FLAGS) \
			-Iinclude \
			-Os \
			-flto



#  -Wold-style-cast -Wconversion -Wsign-conversion -Woverloaded-virtual -Wswitch-default -Wnon-virtual-dtor
COMMON_FLAGS = $(CORE_FLAGS) \
-Wall \
 -Wextra \
  -Wpedantic \
   -Wnull-dereference \
    -Wshadow \
	 -Wstrict-aliasing \
	  -Wcast-align \
	   -Winit-self \
	    -Wuninitialized \
		 -Wunreachable-code \
		  -Wcast-qual \
		   -Wdisabled-optimization \
		    -Wlogical-op \
			 -Wmissing-include-dirs \
			  -Wstrict-overflow=5 \
			   -Wundef \
			    -Wno-unused \
				 -Wno-variadic-macros \
				  -Wno-parentheses \
				   -Wmissing-declarations \
					-Wno-error=unused-parameter \
					 -Wno-unused-parameter




CFLAGS = $(COMMON_FLAGS) -std=c23

CXXFLAGS = $(COMMON_FLAGS) \
-std=c++23 \
  -fno-rtti \
   -fno-exceptions \
    -Wsign-promo \
	 -Wnoexcept \
	  -Wctor-dtor-privacy \
	   -Wstrict-null-sentinel \
	    -fno-threadsafe-statics

LDFLAGS = -mmcu=$(mcu) -Wl,--gc-sections -flto


ifeq ($(mcu),atmega32u4)

USB_VID ?= 0x2341
USB_PID ?= 0x8036
USB_MANUFACTURER ?= '"Arduino LLC"'
USB_PRODUCT ?= '"Arduino Leonardo"'

CORE_FLAGS += \
-DUSB_VID=$(USB_VID) \
-DUSB_PID=$(USB_PID) \
-DUSB_MANUFACTURER=$(USB_MANUFACTURER) \
-DUSB_PRODUCT=$(USB_PRODUCT)

endif

# =========================
# Regla principal
# =========================


$(VERBOSE).SILENT:
all: intro prospect $(BUILD_DIR) $(CORE_LIB) libs $(TARGET_DIR)/$(TARGET).hex $(TARGET_DIR)/$(TARGET).eep


intro:
	@printf "\n\n> Universal Tiny Project Makefile - AVR Edition\n"
	@printf "> Written by Marcos R. See /NOTICE\n\n"

prospect:
	@printf "$(CYAN)Building Owake*\n"
	@printf "Copyright (C) 2026 Marcos Rubiano\n"
	@printf "$(RED)The Owake project is licensed under GPLv3.0, see /LICENSE $(RESET)\n\n\n"

# standalone:
# 	$(CXX) -E $(CXXFLAGS) $(SOURCE_DIR)/$(STD1_TARGET).cpp -o $(TOOLS_DIR)/$(STD1_TARGET).ipp
# 	@printf "\n$(GREEN) Generated stand-alone program file: $(TOOLS_DIR)/$(STD1_TARGET).ipp $(RESET)\n"



$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(CORE_BUILD_DIR)
	mkdir -p $(CORE_BUILD_DIR)/libraries
	mkdir -p $(TARGET_DIR)

# =========================
# Compilación src principal
# =========================
$(CORE_BUILD_DIR)/%.o: $(CORE_DIR)/%.c
	@printf "📦 $(BUILD_MSG) $<\n"
	$(CC) $(CORE_FLAGS) -c $< -o $@

$(CORE_BUILD_DIR)/%++.o: $(CORE_DIR)/%.cpp
	@printf "📦 $(BUILD_MSG) $<\n"
	$(CXX) $(CORE_FLAGS) -fno-threadsafe-statics -c $< -o $@

$(CORE_BUILD_DIR)/%.o: $(CORE_DIR)/%.S
	@printf "⚙ $(BUILD_MSG) $<\n"
	$(CC) $(CORE_FLAGS) -c $< -o $@


$(BUILD_DIR)/%.o:
	mkdir -p $(dir $@)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	@printf "🧱 $(BUILD_MSG) $<\n"
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%++.o: $(SOURCE_DIR)/%.cpp
	@printf "🧱 $(BUILD_MSG) $<\n"
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# =========================
# Compilación librerías
# =========================


$(CORE_LIB): $(CORE_OBJS)
	$(AR) rcs $@ $^

libs: $(CORE_LIB_ARCHIVES) $(LIB_ARCHIVES)

define LIB_template
LIB$(1)_SRC_C := $$(wildcard lib/$(1)/src/*.c)
LIB$(1)_SRC_CPP := $$(wildcard lib/$(1)/src/*.cpp)

LIB$(1)_OBJ_C := $$(patsubst lib/$(1)/src/%.c,$(BUILD_DIR)/$(1)_%.o,$$(LIB$(1)_SRC_C))
LIB$(1)_OBJ_CPP := $$(patsubst lib/$(1)/src/%.cpp,$(BUILD_DIR)/$(1)_%++.o,$$(LIB$(1)_SRC_CPP))

$$(LIB$(1)_OBJ_C): $(BUILD_DIR)/$(1)_%.o: lib/$(1)/src/%.c
	@printf "📕 $(BUILD_MSG) library $$<\n"
	$(CC) $(CFLAGS) -Ilib/$(1)/src -c $$< -o $$@

$$(LIB$(1)_OBJ_CPP): $(BUILD_DIR)/$(1)_%++.o: lib/$(1)/src/%.cpp
	@printf "📕 $(BUILD_MSG) library $$<\n"
	$(CXX) $(CXXFLAGS) -Ilib/$(1)/src -c $$< -o $$@

$(BUILD_DIR)/lib$(1).a: $$(LIB$(1)_OBJ_C) $$(LIB$(1)_OBJ_CPP)
	@printf "📚 $(LINK_MSG) library $$@\n"
	$(AR) rcs $$@ $$^

endef
$(foreach lib,$(LIB_NAMES),$(eval $(call LIB_template,$(lib))))

define CORE_LIB_template
CORE_LIB$(1)_SRC_C := $$(wildcard $(CORE_LIBRARIES_DIR)/$(1)/src/*.c)
CORE_LIB$(1)_SRC_CPP := $$(wildcard $(CORE_LIBRARIES_DIR)/$(1)/src/*.cpp)

CORE_LIB$(1)_OBJ_C := $$(patsubst $(CORE_LIBRARIES_DIR)/$(1)/src/%.c,$(CORE_BUILD_DIR)/libraries/$(1)_%.o,$$(CORE_LIB$(1)_SRC_C))
CORE_LIB$(1)_OBJ_CPP := $$(patsubst $(CORE_LIBRARIES_DIR)/$(1)/src/%.cpp,$(CORE_BUILD_DIR)/libraries/$(1)_%++.o,$$(CORE_LIB$(1)_SRC_CPP))

$$(CORE_LIB$(1)_OBJ_C): $(CORE_BUILD_DIR)/libraries/$(1)_%.o: $(CORE_LIBRARIES_DIR)/$(1)/src/%.c
	@printf "📕 $(BUILD_MSG) library $$<\n"
	$(CC) $(CORE_FLAGS) -I$(CORE_LIBRARIES_DIR)/$(1)/src -c $$< -o $$@

$$(CORE_LIB$(1)_OBJ_CPP): $(CORE_BUILD_DIR)/libraries/$(1)_%++.o: $(CORE_LIBRARIES_DIR)/$(1)/src/%.cpp
	@printf "📕 $(BUILD_MSG) library $$<\n"
	$(CXX) $(CORE_FLAGS) -fno-threadsafe-statics -I$(CORE_LIBRARIES_DIR)/$(1)/src -c $$< -o $$@

$(CORE_BUILD_DIR)/libraries/lib$(1).a: $$(CORE_LIB$(1)_OBJ_C) $$(CORE_LIB$(1)_OBJ_CPP)
	@printf "📚 $(LINK_MSG) library $$@\n"
	$(AR) rcs $$@ $$^

endef

$(foreach clib,$(CORE_LIB_NAMES),$(eval $(call CORE_LIB_template,$(clib))))

# =========================
# Link final
# =========================

$(TARGET_DIR)/$(TARGET).elf: $(MAIN_OBJS) $(LIB_ARCHIVES) $(CORE_LIB_ARCHIVES) $(CORE_LIB)
	@printf "🔗 Linking firmware\n"
	$(CXX) $(LDFLAGS) $^ -o $@
	@printf "💾 avr-size Result: \n"
	$(SIZE) $@

$(TARGET_DIR)/$(TARGET).hex: $(TARGET_DIR)/$(TARGET).elf
	@printf "🔋 Generating firmware\n"
	$(OBJCOPY) -O ihex -R .eeprom $< $@


$(TARGET_DIR)/$(TARGET).eep: $(TARGET_DIR)/$(TARGET).elf
	@printf "💽 Generating EEPROM file\n"
	$(OBJCOPY) -O ihex -j .eeprom \
	--set-section-flags=.eeprom=alloc,load \
	--no-change-warnings \
	$< $@
	@printf "$(GREEN)\n✅ Building done. got firmware $(TARGET_DIR)/$(TARGET).hex <============\n\n$(RESET)"

# =========================
# Upload
# =========================

flash: $(TARGET_DIR)/$(TARGET).hex
ifeq ($(port), 0)
	$(error specify a valid "port=")
endif
ifeq ($(method), usbasp)
	$(AVRDUDE) -c usbasp -p $(mcu) -U flash:w:$<
else
	@printf "Uploading $<\n"
	$(AVRDUDE) -c avr109 -p $(mcu) -P $(port) -b $(baud) -U flash:w:$<
endif

flash_eep: $(TARGET_DIR)/$(TARGET).eep
	$(AVRDUDE) -c $(method) -p $(mcu) \
	-U eeprom:w:$<

# =========================
# Limpieza
# =========================

clean:
	rm -rf $(BUILD_DIR)
	@printf "🧼 Clean routine done.\n"