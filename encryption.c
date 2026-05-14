// encrypt given message with the given substitution cipher

#include "MKL46Z4.h"
#include <stdint.h>
#include <ctype.h>
#include <string.h>

static const char original[] = "abcdefghijklmnopqrstuvwxyz";
static const char key[] = "zxcvbnmlkjpoiuyhgtrqfaewds";

void encryption(char *message, char *encoded){
  for(int i = 0; i < strlen(message); i++){
    char c = message[i];
    if(isalpha(c)){
      if(isupper(c)){
        encoded[i] = toupper(key[tolower(c) - 'a']);
      }else{
        encoded[i] = key[c - 'a'];
      }
    }else{
      encoded[i] = message[i];
    }
  }
  return;
}
