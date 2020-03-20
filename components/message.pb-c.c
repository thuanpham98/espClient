/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: message.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "include/message.pb-c.h"
void   sensor__init
                     (Sensor         *message)
{
  static Sensor init_value = SENSOR__INIT;
  *message = init_value;
}
size_t sensor__get_packed_size
                     (const Sensor *message)
{
  assert(message->base.descriptor == &sensor__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t sensor__pack
                     (const Sensor *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &sensor__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t sensor__pack_to_buffer
                     (const Sensor *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &sensor__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Sensor *
       sensor__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Sensor *)
     protobuf_c_message_unpack (&sensor__descriptor,
                                allocator, len, data);
}
void   sensor__free_unpacked
                     (Sensor *message,
                      ProtobufCAllocator *allocator)
{
  assert(message->base.descriptor == &sensor__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   sensors__init
                     (Sensors         *message)
{
  static Sensors init_value = SENSORS__INIT;
  *message = init_value;
}
size_t sensors__get_packed_size
                     (const Sensors *message)
{
  assert(message->base.descriptor == &sensors__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t sensors__pack
                     (const Sensors *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &sensors__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t sensors__pack_to_buffer
                     (const Sensors *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &sensors__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Sensors *
       sensors__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Sensors *)
     protobuf_c_message_unpack (&sensors__descriptor,
                                allocator, len, data);
}
void   sensors__free_unpacked
                     (Sensors *message,
                      ProtobufCAllocator *allocator)
{
  assert(message->base.descriptor == &sensors__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor sensor__field_descriptors[4] =
{
  {
    "ID",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(Sensor, id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "device",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Sensor, has_device),
    offsetof(Sensor, device),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "io",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Sensor, has_io),
    offsetof(Sensor, io),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "value",
    4,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_FLOAT,
    offsetof(Sensor, has_value),
    offsetof(Sensor, value),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned sensor__field_indices_by_name[] = {
  0,   /* field[0] = ID */
  1,   /* field[1] = device */
  2,   /* field[2] = io */
  3,   /* field[3] = value */
};
static const ProtobufCIntRange sensor__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor sensor__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "Sensor",
  "Sensor",
  "Sensor",
  "",
  sizeof(Sensor),
  4,
  sensor__field_descriptors,
  sensor__field_indices_by_name,
  1,  sensor__number_ranges,
  (ProtobufCMessageInit) sensor__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor sensors__field_descriptors[1] =
{
  {
    "sensors",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Sensors, n_sensors),
    offsetof(Sensors, sensors),
    &sensor__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned sensors__field_indices_by_name[] = {
  0,   /* field[0] = sensors */
};
static const ProtobufCIntRange sensors__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor sensors__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "Sensors",
  "Sensors",
  "Sensors",
  "",
  sizeof(Sensors),
  1,
  sensors__field_descriptors,
  sensors__field_indices_by_name,
  1,  sensors__number_ranges,
  (ProtobufCMessageInit) sensors__init,
  NULL,NULL,NULL    /* reserved[123] */
};
