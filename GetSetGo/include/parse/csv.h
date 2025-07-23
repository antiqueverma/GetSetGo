
#ifndef CSV_H_
#define CSV_H_

#include "gsg_base.h"

#define CSV_DELIMITER       ','
#define CSV_MAX_LINE_LENGTH 1024

gsg_result_t CSV_openFile(const char *filename, FILE **file);
gsg_result_t CSV_getLine(FILE *file, char *buffer, size_t buffer_size);
gsg_result_t CSV_getCell(FILE *file, char *buffer, size_t buffer_size, size_t cell_index);

#endif //CSV_H_