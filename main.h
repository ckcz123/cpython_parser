#ifndef Py_MAIN_H
#define Py_MAIN_H
#ifdef __cplusplus
extern "C" {
#endif

char* tokenize(const char* code, int add_endmarker);
char* predict(const char* tokens);
char* predict2(const char* code);
int isidentifier(const char* token);
void freeme(char* ptr);

#ifdef __cplusplus
}
#endif
#endif /* !Py_MAIN_H */
