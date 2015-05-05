#ifndef ENV_H_INCLUDED
#define ENV_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

char *env_get(char *key);

void env_free(char *val);

/* returns 0 on success.  calls unsetenv(key) if val is null */
int env_put(const char *key, const char *val);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ENV_H_INCLUDED
