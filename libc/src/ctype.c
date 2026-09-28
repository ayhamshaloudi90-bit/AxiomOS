#include <ctype.h>
int isupper(int c){return c>='A'&&c<='Z';}
int islower(int c){return c>='a'&&c<='z';}
int isalpha(int c){return isupper(c)||islower(c);}
int isdigit(int c){return c>='0'&&c<='9';}
int isalnum(int c){return isalpha(c)||isdigit(c);}
int isspace(int c){return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\f'||c=='\v';}
int toupper(int c){return islower(c)?c-('a'-'A'):c;}
int tolower(int c){return isupper(c)?c+('a'-'A'):c;}
