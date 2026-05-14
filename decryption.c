// decrypt given message with the given substitution cipher

#include "MKL46Z4.h"
#include <stdint.h>
#include <ctype.h>
#include <string.h>

static const char original[] = "abcdefghijklmnopqrstuvwxyz";
static const char key[] = "zxcvbnmlkjpoiuyhgtrqfaewds";

void decryption(char *encoded, char *message){
  for(int i = 0; i < strlen(encoded); i++){
    char c = encoded[i];
    if(isalpha(c)){
      if(isupper(c)){
        for(int j = 0; j < 26; j++){
          if(key[j] == tolower(c)){
            message[i] = toupper(original[j]);
            break;
          }
        }
      }else{
        for(int j = 0; j < 26; j++){
          if(key[j] == c){
            message[i] = original[j];
            break;
          }
        }
      }
    }else{
      message[i] = c;
    }
  }
  return;
}
