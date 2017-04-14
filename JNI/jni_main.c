#include "HelloJni.h"
#include <stdio.h>

JNIEXPORT void JNICALL Java_HelloJni_printHelloWolrd(JNIEnv *env, jobject thisObj){

    printf("Hello World!\n");
    return ;

}
