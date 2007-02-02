#ifndef DATE_H_
#define DATE_H_

extern char*  getDateStr(struct tm* ctime);
extern time_t getDate(const char* date);
extern time_t getFileDate(const char* filename);
extern int    getFileSize(const char* filename);
extern char*  getDateStr(time_t time);
extern const char* basename(const char* filepath);
extern char*  encodeURL(const char* url);
extern char*  decodeURL(const char* url);


#endif /*DATE_H_*/
