#ifndef Base_H__
#define Base_H__

int Base64decode_len(const char *bufcoded);
int Base64decode(char *bufplain,const char *bufcoded);
int Base64encode_len(int len);
int Base64encode(char *encoded, const char *string, int len);
#endif // Base64_H__
