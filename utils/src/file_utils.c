#include "../include/file_utils.h"
#include "../include/file_utils.h"

/* Returns true or false whether directory exists */
bool directory_exists(const char* _path) {
  struct stat buffer;
  if (stat(_path, &buffer) == 0) {
    return S_ISDIR(buffer.st_mode);
  }
  return false;
}

/* Tries to create directory if it doesn't already exist */
int create_directory_if_not_exists(const char* _path) {
  if (directory_exists(_path)) {
    return 0;
  }
  #if defined _WIN32
  bool success = CreateDirectory(_Path, NULL);
  if(success == false)
  {
    DWORD err = GetLastError();
    if(err == ERROR_ALREADY_EXISTS)
      return 1;
    else
      return -1;
  }
	#else
  if (mkdir(_path, 0755) == -1) {
    if (errno != EEXIST) {
      printf("Failed to create directory '%s': %s\n", _path, strerror(errno));
      return -1;
    }
  }
  #endif
  return 0;
}

/* Writes string to given file */
int write_string_to_file(const char* _str, const char* _filename)
{
  FILE *f = fopen(_filename, "w");
  if (f == NULL) {
   printf("Error: Unable to open the file.\n");
   return -1;
  }

  fputs(_str, f);
  fclose(f);

  return 0;
}
