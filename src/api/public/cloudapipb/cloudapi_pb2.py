# Copyright 2018- The Pixie Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: src/api/public/cloudapipb/cloudapi.proto
"""Generated protocol buffer code."""
from google.protobuf.internal import enum_type_wrapper
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


from github.com.gogo.protobuf.gogoproto import gogo_pb2 as github_dot_com_dot_gogo_dot_protobuf_dot_gogoproto_dot_gogo__pb2
from google.protobuf import wrappers_pb2 as google_dot_protobuf_dot_wrappers__pb2
from src.api.public.uuidpb import uuid_pb2 as src_dot_api_dot_public_dot_uuidpb_dot_uuid__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='src/api/public/cloudapipb/cloudapi.proto',
  package='pl.public.cloudapi',
  syntax='proto3',
  serialized_options=b'ZApixielabs.ai/pixielabs/src/api/public/cloudapipb;publiccloudapipb',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n(src/api/public/cloudapipb/cloudapi.proto\x12\x12pl.public.cloudapi\x1a-github.com/gogo/protobuf/gogoproto/gogo.proto\x1a\x1egoogle/protobuf/wrappers.proto\x1a src/api/public/uuidpb/uuid.proto\",\n\rClusterConfig\x12\x1b\n\x13passthrough_enabled\x18\x01 \x01(\x08\"N\n\x13\x43lusterConfigUpdate\x12\x37\n\x13passthrough_enabled\x18\x01 \x01(\x0b\x32\x1a.google.protobuf.BoolValue\"8\n\x11GetClusterRequest\x12#\n\x02id\x18\x01 \x01(\x0b\x32\x0f.pl.uuidpb.UUIDB\x06\xe2\xde\x1f\x02ID\"B\n\x1bGetClusterConnectionRequest\x12#\n\x02id\x18\x01 \x01(\x0b\x32\x0f.pl.uuidpb.UUIDB\x06\xe2\xde\x1f\x02ID\"O\n\x1cGetClusterConnectionResponse\x12 \n\tipAddress\x18\x01 \x01(\tB\r\xe2\xde\x1f\tIPAddress\x12\r\n\x05token\x18\x02 \x01(\t\"\xd0\x02\n\x0b\x43lusterInfo\x12#\n\x02id\x18\x01 \x01(\x0b\x32\x0f.pl.uuidpb.UUIDB\x06\xe2\xde\x1f\x02ID\x12\x31\n\x06status\x18\x02 \x01(\x0e\x32!.pl.public.cloudapi.ClusterStatus\x12\x17\n\x0flastHeartbeatNs\x18\x03 \x01(\x03\x12\x31\n\x06\x63onfig\x18\x04 \x01(\x0b\x32!.pl.public.cloudapi.ClusterConfig\x12#\n\x0b\x63luster_uid\x18\x05 \x01(\tB\x0e\xe2\xde\x1f\nClusterUID\x12\x14\n\x0c\x63luster_name\x18\x06 \x01(\t\x12\x17\n\x0f\x63luster_version\x18\x07 \x01(\t\x12\x16\n\x0evizier_version\x18\x08 \x01(\t\x12\x11\n\tnum_nodes\x18\t \x01(\x05\x12\x1e\n\x16num_instrumented_nodes\x18\n \x01(\x05\"G\n\x12GetClusterResponse\x12\x31\n\x08\x63lusters\x18\x01 \x03(\x0b\x32\x1f.pl.public.cloudapi.ClusterInfo\"\x81\x01\n\x1aUpdateClusterConfigRequest\x12#\n\x02id\x18\x01 \x01(\x0b\x32\x0f.pl.uuidpb.UUIDB\x06\xe2\xde\x1f\x02ID\x12>\n\rconfig_update\x18\x02 \x01(\x0b\x32\'.pl.public.cloudapi.ClusterConfigUpdate\"\x1d\n\x1bUpdateClusterConfigResponse*\x8f\x01\n\rClusterStatus\x12\x0e\n\nCS_UNKNOWN\x10\x00\x12\x0e\n\nCS_HEALTHY\x10\x01\x12\x10\n\x0c\x43S_UNHEALTHY\x10\x02\x12\x13\n\x0f\x43S_DISCONNECTED\x10\x03\x12\x0f\n\x0b\x43S_UPDATING\x10\x04\x12\x10\n\x0c\x43S_CONNECTED\x10\x05\x12\x14\n\x10\x43S_UPDATE_FAILED\x10\x06\x32\xe6\x02\n\x0e\x43lusterManager\x12]\n\nGetCluster\x12%.pl.public.cloudapi.GetClusterRequest\x1a&.pl.public.cloudapi.GetClusterResponse\"\x00\x12x\n\x13UpdateClusterConfig\x12..pl.public.cloudapi.UpdateClusterConfigRequest\x1a/.pl.public.cloudapi.UpdateClusterConfigResponse\"\x00\x12{\n\x14GetClusterConnection\x12/.pl.public.cloudapi.GetClusterConnectionRequest\x1a\x30.pl.public.cloudapi.GetClusterConnectionResponse\"\x00\x42\x43ZApixielabs.ai/pixielabs/src/api/public/cloudapipb;publiccloudapipbb\x06proto3'
  ,
  dependencies=[github_dot_com_dot_gogo_dot_protobuf_dot_gogoproto_dot_gogo__pb2.DESCRIPTOR,google_dot_protobuf_dot_wrappers__pb2.DESCRIPTOR,src_dot_api_dot_public_dot_uuidpb_dot_uuid__pb2.DESCRIPTOR,])

_CLUSTERSTATUS = _descriptor.EnumDescriptor(
  name='ClusterStatus',
  full_name='pl.public.cloudapi.ClusterStatus',
  filename=None,
  file=DESCRIPTOR,
  create_key=_descriptor._internal_create_key,
  values=[
    _descriptor.EnumValueDescriptor(
      name='CS_UNKNOWN', index=0, number=0,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='CS_HEALTHY', index=1, number=1,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='CS_UNHEALTHY', index=2, number=2,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='CS_DISCONNECTED', index=3, number=3,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='CS_UPDATING', index=4, number=4,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='CS_CONNECTED', index=5, number=5,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='CS_UPDATE_FAILED', index=6, number=6,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
  ],
  containing_type=None,
  serialized_options=None,
  serialized_start=1086,
  serialized_end=1229,
)
_sym_db.RegisterEnumDescriptor(_CLUSTERSTATUS)

ClusterStatus = enum_type_wrapper.EnumTypeWrapper(_CLUSTERSTATUS)
CS_UNKNOWN = 0
CS_HEALTHY = 1
CS_UNHEALTHY = 2
CS_DISCONNECTED = 3
CS_UPDATING = 4
CS_CONNECTED = 5
CS_UPDATE_FAILED = 6



_CLUSTERCONFIG = _descriptor.Descriptor(
  name='ClusterConfig',
  full_name='pl.public.cloudapi.ClusterConfig',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='passthrough_enabled', full_name='pl.public.cloudapi.ClusterConfig.passthrough_enabled', index=0,
      number=1, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=177,
  serialized_end=221,
)


_CLUSTERCONFIGUPDATE = _descriptor.Descriptor(
  name='ClusterConfigUpdate',
  full_name='pl.public.cloudapi.ClusterConfigUpdate',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='passthrough_enabled', full_name='pl.public.cloudapi.ClusterConfigUpdate.passthrough_enabled', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=223,
  serialized_end=301,
)


_GETCLUSTERREQUEST = _descriptor.Descriptor(
  name='GetClusterRequest',
  full_name='pl.public.cloudapi.GetClusterRequest',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='id', full_name='pl.public.cloudapi.GetClusterRequest.id', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\002ID', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=303,
  serialized_end=359,
)


_GETCLUSTERCONNECTIONREQUEST = _descriptor.Descriptor(
  name='GetClusterConnectionRequest',
  full_name='pl.public.cloudapi.GetClusterConnectionRequest',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='id', full_name='pl.public.cloudapi.GetClusterConnectionRequest.id', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\002ID', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=361,
  serialized_end=427,
)


_GETCLUSTERCONNECTIONRESPONSE = _descriptor.Descriptor(
  name='GetClusterConnectionResponse',
  full_name='pl.public.cloudapi.GetClusterConnectionResponse',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='ipAddress', full_name='pl.public.cloudapi.GetClusterConnectionResponse.ipAddress', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\tIPAddress', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='token', full_name='pl.public.cloudapi.GetClusterConnectionResponse.token', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=429,
  serialized_end=508,
)


_CLUSTERINFO = _descriptor.Descriptor(
  name='ClusterInfo',
  full_name='pl.public.cloudapi.ClusterInfo',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='id', full_name='pl.public.cloudapi.ClusterInfo.id', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\002ID', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='status', full_name='pl.public.cloudapi.ClusterInfo.status', index=1,
      number=2, type=14, cpp_type=8, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='lastHeartbeatNs', full_name='pl.public.cloudapi.ClusterInfo.lastHeartbeatNs', index=2,
      number=3, type=3, cpp_type=2, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='config', full_name='pl.public.cloudapi.ClusterInfo.config', index=3,
      number=4, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='cluster_uid', full_name='pl.public.cloudapi.ClusterInfo.cluster_uid', index=4,
      number=5, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\nClusterUID', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='cluster_name', full_name='pl.public.cloudapi.ClusterInfo.cluster_name', index=5,
      number=6, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='cluster_version', full_name='pl.public.cloudapi.ClusterInfo.cluster_version', index=6,
      number=7, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='vizier_version', full_name='pl.public.cloudapi.ClusterInfo.vizier_version', index=7,
      number=8, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='num_nodes', full_name='pl.public.cloudapi.ClusterInfo.num_nodes', index=8,
      number=9, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='num_instrumented_nodes', full_name='pl.public.cloudapi.ClusterInfo.num_instrumented_nodes', index=9,
      number=10, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=511,
  serialized_end=847,
)


_GETCLUSTERRESPONSE = _descriptor.Descriptor(
  name='GetClusterResponse',
  full_name='pl.public.cloudapi.GetClusterResponse',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='clusters', full_name='pl.public.cloudapi.GetClusterResponse.clusters', index=0,
      number=1, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=849,
  serialized_end=920,
)


_UPDATECLUSTERCONFIGREQUEST = _descriptor.Descriptor(
  name='UpdateClusterConfigRequest',
  full_name='pl.public.cloudapi.UpdateClusterConfigRequest',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='id', full_name='pl.public.cloudapi.UpdateClusterConfigRequest.id', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\002ID', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='config_update', full_name='pl.public.cloudapi.UpdateClusterConfigRequest.config_update', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=923,
  serialized_end=1052,
)


_UPDATECLUSTERCONFIGRESPONSE = _descriptor.Descriptor(
  name='UpdateClusterConfigResponse',
  full_name='pl.public.cloudapi.UpdateClusterConfigResponse',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=1054,
  serialized_end=1083,
)

_CLUSTERCONFIGUPDATE.fields_by_name['passthrough_enabled'].message_type = google_dot_protobuf_dot_wrappers__pb2._BOOLVALUE
_GETCLUSTERREQUEST.fields_by_name['id'].message_type = src_dot_api_dot_public_dot_uuidpb_dot_uuid__pb2._UUID
_GETCLUSTERCONNECTIONREQUEST.fields_by_name['id'].message_type = src_dot_api_dot_public_dot_uuidpb_dot_uuid__pb2._UUID
_CLUSTERINFO.fields_by_name['id'].message_type = src_dot_api_dot_public_dot_uuidpb_dot_uuid__pb2._UUID
_CLUSTERINFO.fields_by_name['status'].enum_type = _CLUSTERSTATUS
_CLUSTERINFO.fields_by_name['config'].message_type = _CLUSTERCONFIG
_GETCLUSTERRESPONSE.fields_by_name['clusters'].message_type = _CLUSTERINFO
_UPDATECLUSTERCONFIGREQUEST.fields_by_name['id'].message_type = src_dot_api_dot_public_dot_uuidpb_dot_uuid__pb2._UUID
_UPDATECLUSTERCONFIGREQUEST.fields_by_name['config_update'].message_type = _CLUSTERCONFIGUPDATE
DESCRIPTOR.message_types_by_name['ClusterConfig'] = _CLUSTERCONFIG
DESCRIPTOR.message_types_by_name['ClusterConfigUpdate'] = _CLUSTERCONFIGUPDATE
DESCRIPTOR.message_types_by_name['GetClusterRequest'] = _GETCLUSTERREQUEST
DESCRIPTOR.message_types_by_name['GetClusterConnectionRequest'] = _GETCLUSTERCONNECTIONREQUEST
DESCRIPTOR.message_types_by_name['GetClusterConnectionResponse'] = _GETCLUSTERCONNECTIONRESPONSE
DESCRIPTOR.message_types_by_name['ClusterInfo'] = _CLUSTERINFO
DESCRIPTOR.message_types_by_name['GetClusterResponse'] = _GETCLUSTERRESPONSE
DESCRIPTOR.message_types_by_name['UpdateClusterConfigRequest'] = _UPDATECLUSTERCONFIGREQUEST
DESCRIPTOR.message_types_by_name['UpdateClusterConfigResponse'] = _UPDATECLUSTERCONFIGRESPONSE
DESCRIPTOR.enum_types_by_name['ClusterStatus'] = _CLUSTERSTATUS
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

ClusterConfig = _reflection.GeneratedProtocolMessageType('ClusterConfig', (_message.Message,), {
  'DESCRIPTOR' : _CLUSTERCONFIG,
  '__module__' : 'src.api.public.cloudapipb.cloudapi_pb2'
  # @@protoc_insertion_point(class_scope:pl.public.cloudapi.ClusterConfig)
  })
_sym_db.RegisterMessage(ClusterConfig)

ClusterConfigUpdate = _reflection.GeneratedProtocolMessageType('ClusterConfigUpdate', (_message.Message,), {
  'DESCRIPTOR' : _CLUSTERCONFIGUPDATE,
  '__module__' : 'src.api.public.cloudapipb.cloudapi_pb2'
  # @@protoc_insertion_point(class_scope:pl.public.cloudapi.ClusterConfigUpdate)
  })
_sym_db.RegisterMessage(ClusterConfigUpdate)

GetClusterRequest = _reflection.GeneratedProtocolMessageType('GetClusterRequest', (_message.Message,), {
  'DESCRIPTOR' : _GETCLUSTERREQUEST,
  '__module__' : 'src.api.public.cloudapipb.cloudapi_pb2'
  # @@protoc_insertion_point(class_scope:pl.public.cloudapi.GetClusterRequest)
  })
_sym_db.RegisterMessage(GetClusterRequest)

GetClusterConnectionRequest = _reflection.GeneratedProtocolMessageType('GetClusterConnectionRequest', (_message.Message,), {
  'DESCRIPTOR' : _GETCLUSTERCONNECTIONREQUEST,
  '__module__' : 'src.api.public.cloudapipb.cloudapi_pb2'
  # @@protoc_insertion_point(class_scope:pl.public.cloudapi.GetClusterConnectionRequest)
  })
_sym_db.RegisterMessage(GetClusterConnectionRequest)

GetClusterConnectionResponse = _reflection.GeneratedProtocolMessageType('GetClusterConnectionResponse', (_message.Message,), {
  'DESCRIPTOR' : _GETCLUSTERCONNECTIONRESPONSE,
  '__module__' : 'src.api.public.cloudapipb.cloudapi_pb2'
  # @@protoc_insertion_point(class_scope:pl.public.cloudapi.GetClusterConnectionResponse)
  })
_sym_db.RegisterMessage(GetClusterConnectionResponse)

ClusterInfo = _reflection.GeneratedProtocolMessageType('ClusterInfo', (_message.Message,), {
  'DESCRIPTOR' : _CLUSTERINFO,
  '__module__' : 'src.api.public.cloudapipb.cloudapi_pb2'
  # @@protoc_insertion_point(class_scope:pl.public.cloudapi.ClusterInfo)
  })
_sym_db.RegisterMessage(ClusterInfo)

GetClusterResponse = _reflection.GeneratedProtocolMessageType('GetClusterResponse', (_message.Message,), {
  'DESCRIPTOR' : _GETCLUSTERRESPONSE,
  '__module__' : 'src.api.public.cloudapipb.cloudapi_pb2'
  # @@protoc_insertion_point(class_scope:pl.public.cloudapi.GetClusterResponse)
  })
_sym_db.RegisterMessage(GetClusterResponse)

UpdateClusterConfigRequest = _reflection.GeneratedProtocolMessageType('UpdateClusterConfigRequest', (_message.Message,), {
  'DESCRIPTOR' : _UPDATECLUSTERCONFIGREQUEST,
  '__module__' : 'src.api.public.cloudapipb.cloudapi_pb2'
  # @@protoc_insertion_point(class_scope:pl.public.cloudapi.UpdateClusterConfigRequest)
  })
_sym_db.RegisterMessage(UpdateClusterConfigRequest)

UpdateClusterConfigResponse = _reflection.GeneratedProtocolMessageType('UpdateClusterConfigResponse', (_message.Message,), {
  'DESCRIPTOR' : _UPDATECLUSTERCONFIGRESPONSE,
  '__module__' : 'src.api.public.cloudapipb.cloudapi_pb2'
  # @@protoc_insertion_point(class_scope:pl.public.cloudapi.UpdateClusterConfigResponse)
  })
_sym_db.RegisterMessage(UpdateClusterConfigResponse)


DESCRIPTOR._options = None
_GETCLUSTERREQUEST.fields_by_name['id']._options = None
_GETCLUSTERCONNECTIONREQUEST.fields_by_name['id']._options = None
_GETCLUSTERCONNECTIONRESPONSE.fields_by_name['ipAddress']._options = None
_CLUSTERINFO.fields_by_name['id']._options = None
_CLUSTERINFO.fields_by_name['cluster_uid']._options = None
_UPDATECLUSTERCONFIGREQUEST.fields_by_name['id']._options = None

_CLUSTERMANAGER = _descriptor.ServiceDescriptor(
  name='ClusterManager',
  full_name='pl.public.cloudapi.ClusterManager',
  file=DESCRIPTOR,
  index=0,
  serialized_options=None,
  create_key=_descriptor._internal_create_key,
  serialized_start=1232,
  serialized_end=1590,
  methods=[
  _descriptor.MethodDescriptor(
    name='GetCluster',
    full_name='pl.public.cloudapi.ClusterManager.GetCluster',
    index=0,
    containing_service=None,
    input_type=_GETCLUSTERREQUEST,
    output_type=_GETCLUSTERRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='UpdateClusterConfig',
    full_name='pl.public.cloudapi.ClusterManager.UpdateClusterConfig',
    index=1,
    containing_service=None,
    input_type=_UPDATECLUSTERCONFIGREQUEST,
    output_type=_UPDATECLUSTERCONFIGRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='GetClusterConnection',
    full_name='pl.public.cloudapi.ClusterManager.GetClusterConnection',
    index=2,
    containing_service=None,
    input_type=_GETCLUSTERCONNECTIONREQUEST,
    output_type=_GETCLUSTERCONNECTIONRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
])
_sym_db.RegisterServiceDescriptor(_CLUSTERMANAGER)

DESCRIPTOR.services_by_name['ClusterManager'] = _CLUSTERMANAGER

# @@protoc_insertion_point(module_scope)
