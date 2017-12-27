#ifdef SWIGJAVA
#define __ANDROID__
#endif

%module(directors="1") megachat
%{
#include "megachatapi.h"

extern JavaVM *MEGAjvm;

#ifdef __ANDROID__
extern jstring strEncodeUTF8;
extern jclass clsString;
extern jmethodID ctorString;
extern jmethodID getBytes;
extern int sdkVersion;
#endif

#ifndef KARERE_DISABLE_WEBRTC
    namespace webrtc
    {
        class JVM
        {
            public:
                static void Initialize(JavaVM* jvm, jobject context);
        };
    };
#endif

%}
%import "megaapi.h"

#ifdef SWIGJAVA

//Use compilation-time constants in Java
%javaconst(1);

%typemap(out) char*
%{
    if ($1)
    {
        int len = strlen($1);
        jbyteArray $1_array = jenv->NewByteArray(len);
        jenv->SetByteArrayRegion($1_array, 0, len, (const jbyte*)$1);
        $result = (jstring) jenv->NewObject(clsString, ctorString, $1_array, strEncodeUTF8);
        jenv->DeleteLocalRef($1_array);
    }
%}

%typemap(in) char*
%{
    jbyteArray $1_array;
    $1 = 0;
    if ($input)
    {
        $1_array = (jbyteArray) jenv->CallObjectMethod($input, getBytes, strEncodeUTF8);
        jsize $1_size = jenv->GetArrayLength($1_array);
        $1 = new char[$1_size + 1];
        if ($1_size)
        {
            jenv->GetByteArrayRegion($1_array, 0, $1_size, (jbyte*)$1);
        }
        $1[$1_size] = '\0';
    }
%}

%typemap(freearg) char*
%{
    if ($1)
    {
        delete [] $1;
        jenv->DeleteLocalRef($1_array);
    }
%}

%typemap(directorin,descriptor="Ljava/lang/String;") char *
%{
    $input = 0;
    if ($1)
    {
        int len = strlen($1);
        jbyteArray $1_array = jenv->NewByteArray(len);
        jenv->SetByteArrayRegion($1_array, 0, len, (const jbyte*)$1);
        $input = (jstring) jenv->NewObject(clsString, ctorString, $1_array, strEncodeUTF8);
        jenv->DeleteLocalRef($1_array);
    }
    Swig::LocalRefGuard $1_refguard(jenv, $input);
%}

//Make the "delete" method protected
%typemap(javadestruct, methodname="delete", methodmodifiers="protected synchronized") SWIGTYPE 
{   
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        $jnicall;
      }
      swigCPtr = 0;
    }
}

%javamethodmodifiers copy ""

%typemap(check) mega::MegaApi *megaApi
%{

#ifndef KARERE_DISABLE_WEBRTC
    if (!MEGAjvm)
    { 
        jenv->GetJavaVM(&MEGAjvm);
    }
    
    MEGAjvm->AttachCurrentThread(&jenv, NULL);
    jclass appGlobalsClass = jenv->FindClass("android/app/AppGlobals");
    jmethodID getInitialApplicationMID = jenv->GetStaticMethodID(appGlobalsClass,"getInitialApplication","()Landroid/app/Application;");
    jobject context = jenv->CallStaticObjectMethod(appGlobalsClass, getInitialApplicationMID);

    // Initialize the Java environment (currently only used by the audio manager).
    webrtc::JVM::Initialize(MEGAjvm, context);
    //MEGAjvm->DetachCurrentThread();
#endif

%}

#endif

//Generate inheritable wrappers for listener objects
%feature("director") megachat::MegaChatRequestListener;
%feature("director") megachat::MegaChatCallListener;
%feature("director") megachat::MegaChatVideoListener;
%feature("director") megachat::MegaChatListener;
%feature("director") megachat::MegaChatLogger;
%feature("director") megachat::MegaChatRoomListener;

typedef long long time_t;
typedef long long uint64_t;
typedef long long int64_t;

%include "megachatapi.h"

