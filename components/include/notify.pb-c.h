/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: notify.proto */

#ifndef PROTOBUF_C_notify_2eproto__INCLUDED
#define PROTOBUF_C_notify_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003003 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _Notify Notify;
typedef struct _Notifies Notifies;


/* --- enums --- */


/* --- messages --- */

struct  _Notify
{
  ProtobufCMessage base;
  char *id;
  uint32_t device;
  char *header;
  char *content;
  char *comment;
};
#define NOTIFY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&notify__descriptor) \
    , (char *)protobuf_c_empty_string, 0, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string }


struct  _Notifies
{
  ProtobufCMessage base;
  size_t n_notifies;
  Notify **notifies;
};
#define NOTIFIES__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&notifies__descriptor) \
    , 0,NULL }


/* Notify methods */
void   notify__init
                     (Notify         *message);
size_t notify__get_packed_size
                     (const Notify   *message);
size_t notify__pack
                     (const Notify   *message,
                      uint8_t             *out);
size_t notify__pack_to_buffer
                     (const Notify   *message,
                      ProtobufCBuffer     *buffer);
Notify *
       notify__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   notify__free_unpacked
                     (Notify *message,
                      ProtobufCAllocator *allocator);
/* Notifies methods */
void   notifies__init
                     (Notifies         *message);
size_t notifies__get_packed_size
                     (const Notifies   *message);
size_t notifies__pack
                     (const Notifies   *message,
                      uint8_t             *out);
size_t notifies__pack_to_buffer
                     (const Notifies   *message,
                      ProtobufCBuffer     *buffer);
Notifies *
       notifies__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   notifies__free_unpacked
                     (Notifies *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Notify_Closure)
                 (const Notify *message,
                  void *closure_data);
typedef void (*Notifies_Closure)
                 (const Notifies *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor notify__descriptor;
extern const ProtobufCMessageDescriptor notifies__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_notify_2eproto__INCLUDED */
