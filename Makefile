BUILD_DIR := build

.PHONY: all run clean

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)

$(BUILD_DIR)/Makefile: CMakeLists.txt
	cmake -S . -B $(BUILD_DIR) -G "Unix Makefiles"

run: all
	./$(BUILD_DIR)/wyag --help

clean:
	rm -rf $(BUILD_DIR)
