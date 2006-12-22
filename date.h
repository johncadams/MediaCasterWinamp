#ifndef DATE_H_
#define DATE_H_

extern char* getDateStr(struct tm* ctime);
extern long  getDate(const char* date);
extern int   getFileDate(const char* filename);
extern int   getFileSize(const char* filename);
extern char* getDateStr(time_t time);


#endif /*DATE_H_*/
