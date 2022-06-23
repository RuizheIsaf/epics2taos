
#include "headers.h"



ARCHIVE_ERROR archive_initial(ARCHIVER *archiver);
ARCHIVE_ERROR archive_pv(evargs eha);
void archive_thread(ARCHIVER *parchiver);
ARCHIVE_ERROR start_archive_thread(ARCHIVER *archiver);


