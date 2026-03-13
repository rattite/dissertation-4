CC = gcc
CFLAGS = -g -MMD -Isrc
LIBS = -lm -lsqlite3 -lspatialite -lgeos_c -lgsl -lgslcblas

# Directory Definitions
SRC_DIR = src
BIN_DIR = bin
DATA_DIR = data

# List of Object Files (prefixed with bin/)
OBJS = $(addprefix $(BIN_DIR)/, curve.o db.o grid.o m1.o m2.o m3.o test_helper.o)

# Targets
TEST_BINS = naivetest indextest m1test m2test m3test reset draw_bboxes draw_3 curvetest query_gen 
DATA_BINS = process getpoints clean_db

# Final paths for targets
ALL_TARGETS = $(addprefix $(BIN_DIR)/, $(TEST_BINS)) $(addprefix $(DATA_DIR)/, $(DATA_BINS))

all: $(BIN_DIR) $(DATA_DIR) $(ALL_TARGETS)

# Create directories if they don't exist
$(BIN_DIR) $(DATA_DIR):
	mkdir -p $@

# Pattern rule for object files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Specific rules for binaries in bin/
$(BIN_DIR)/%: $(SRC_DIR)/%.c $(OBJS)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LIBS)

# Specific rules for binaries in data/
$(DATA_DIR)/%: $(SRC_DIR)/%.c $(OBJS)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LIBS)

# Special case for 'reset' (it only needs test_helper.o based on your original file)
$(BIN_DIR)/reset: $(SRC_DIR)/reset.c $(BIN_DIR)/test_helper.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -rf $(BIN_DIR)/*.o $(BIN_DIR)/*.d $(ALL_TARGETS)

-include $(OBJS:.o=.d)
